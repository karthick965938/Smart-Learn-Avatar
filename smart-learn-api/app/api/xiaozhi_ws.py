"""
XiaoZhi-compatible WebSocket server.

Protocol summary (from xiaozhi-esp32 firmware):

CLIENT → SERVER (TEXT):
  {"type":"hello", "version":3, "features":{...}, "transport":"websocket",
   "audio_params":{"format":"opus","sample_rate":16000,"channels":1,"frame_duration":60}}

  {"session_id":"...", "type":"listen", "state":"start", "mode":"auto|manual|realtime"}
  {"session_id":"...", "type":"listen", "state":"stop"}
  {"session_id":"...", "type":"listen", "state":"detect", "text":"<wake word>"}
  {"session_id":"...", "type":"abort"}
  {"session_id":"...", "type":"mcp", "payload":{...}}

CLIENT → SERVER (BINARY):
  Raw Opus audio frames (when device is listening)

SERVER → CLIENT (TEXT):
  {"type":"hello", "transport":"websocket", "session_id":"...",
   "audio_params":{"sample_rate":16000,"frame_duration":60}}

  {"type":"stt",  "text":"<what user said>"}
  {"type":"llm",  "emotion":"neutral"}
  {"type":"tts",  "state":"start"}
  {"type":"tts",  "state":"sentence_start", "text":"<sentence being spoken>"}
  {"type":"tts",  "state":"stop"}

SERVER → CLIENT (BINARY):
  Raw Opus audio frames (TTS reply)
"""

import json
import uuid
import asyncio
import httpx
from fastapi import APIRouter, WebSocket, WebSocketDisconnect

from app.core.stt import transcribe_opus
from app.core.tts import text_to_opus
from app.config import settings

router = APIRouter()

# Sliding chunk size: collect this many Opus frames before running STT
# 60 ms/frame × 50 frames = 3 seconds of audio per chunk
_FRAMES_PER_STT_CHUNK = 50


@router.websocket("/ws/xiaozhi")
async def xiaozhi_websocket(websocket: WebSocket):
    """
    XiaoZhi-compatible WebSocket endpoint.
    The ESP32 device connects here after the OTA endpoint returns this URL.
    """
    await websocket.accept()
    session_id = str(uuid.uuid4())
    print(f"[WS] New connection — session {session_id}")

    # Negotiated audio params (updated from client hello)
    sample_rate = 16000
    frame_duration = 60  # ms

    # State
    is_listening = False
    audio_frames: list[bytes] = []
    abort_flag = False

    try:
        # ── Step 1: wait for client "hello" ──────────────────────────────────
        raw = await websocket.receive()
        if raw.get("type") == "websocket.disconnect":
            return

        hello_text = raw.get("text") or raw.get("bytes", b"").decode()
        try:
            hello = json.loads(hello_text)
        except Exception:
            await websocket.close(code=1008)
            return

        if hello.get("type") != "hello":
            await websocket.close(code=1008)
            return

        ap = hello.get("audio_params", {})
        sample_rate = ap.get("sample_rate", 16000)
        frame_duration = ap.get("frame_duration", 60)
        print(f"[WS] Client hello: sample_rate={sample_rate}, frame_duration={frame_duration}")

        # ── Step 2: send server "hello" ───────────────────────────────────────
        server_hello = {
            "type": "hello",
            "transport": "websocket",
            "session_id": session_id,
            "audio_params": {
                "sample_rate": sample_rate,
                "frame_duration": frame_duration,
            },
        }
        await websocket.send_text(json.dumps(server_hello))
        print(f"[WS] Sent server hello for session {session_id}")

        # ── Step 3: main message loop ─────────────────────────────────────────
        while True:
            raw = await websocket.receive()

            if raw.get("type") == "websocket.disconnect":
                break

            # Binary = Opus audio from mic
            if raw.get("bytes") is not None:
                if is_listening and not abort_flag:
                    audio_frames.append(raw["bytes"])

            # Text = control messages
            elif raw.get("text"):
                msg = {}
                try:
                    msg = json.loads(raw["text"])
                except Exception:
                    continue

                msg_type = msg.get("type", "")

                if msg_type == "listen":
                    state = msg.get("state", "")

                    if state == "start":
                        is_listening = True
                        abort_flag = False
                        audio_frames = []
                        print(f"[WS] Listening started (mode={msg.get('mode','auto')})")

                    elif state == "detect":
                        # Wake word detected — device will start sending audio next
                        is_listening = True
                        abort_flag = False
                        audio_frames = []
                        print(f"[WS] Wake word: {msg.get('text', '')}")

                    elif state == "stop":
                        print(f"[WS] Listening stopped — running STT on {len(audio_frames)} frames")
                        is_listening = False

                        if audio_frames and not abort_flag:
                            # Run the full STT → KB → TTS pipeline
                            frames_copy = audio_frames[:]
                            audio_frames = []
                            asyncio.ensure_future(
                                _handle_query(websocket, session_id, frames_copy, sample_rate)
                            )
                        else:
                            audio_frames = []

                elif msg_type == "abort":
                    print(f"[WS] Abort received (reason={msg.get('reason','')})")
                    abort_flag = True
                    is_listening = False
                    audio_frames = []

    except WebSocketDisconnect:
        print(f"[WS] Client disconnected — session {session_id}")
    except Exception as e:
        print(f"[WS] Unexpected error in session {session_id}: {e}")
    finally:
        print(f"[WS] Session {session_id} closed")


async def _handle_query(
    websocket: WebSocket,
    session_id: str,
    opus_frames: list[bytes],
    sample_rate: int,
):
    """
    Full pipeline: Opus audio → STT → KB query → TTS → stream back to device.
    """
    try:
        # 1. Speech → Text
        print(f"[Pipeline] Running STT...")
        user_text = await transcribe_opus(opus_frames, sample_rate=sample_rate)
        if not user_text:
            print("[Pipeline] STT returned empty text — skipping")
            return

        print(f"[Pipeline] STT result: '{user_text}'")

        # 2. Send STT result to device (shows on display)
        await websocket.send_text(json.dumps({
            "type": "stt",
            "text": user_text,
        }))

        # 3. Send LLM emotion hint (neutral by default)
        await websocket.send_text(json.dumps({
            "type": "llm",
            "emotion": "neutral",
        }))

        # 4. Query the KB endpoint
        print(f"[Pipeline] Querying KB endpoint: {settings.KB_ENDPOINT}")
        answer = await _query_kb(user_text)
        if not answer:
            answer = "Sorry, I could not find an answer."

        print(f"[Pipeline] KB answer: '{answer}'")

        # 5. Signal TTS start
        await websocket.send_text(json.dumps({
            "type": "tts",
            "state": "start",
        }))

        # 6. Send sentence text (appears as subtitle on device)
        await websocket.send_text(json.dumps({
            "type": "tts",
            "state": "sentence_start",
            "text": answer,
        }))

        # 7. Synthesize TTS audio and stream it back as binary Opus frames
        print(f"[Pipeline] Running TTS...")
        opus_audio = await text_to_opus(answer, sample_rate=sample_rate)
        if opus_audio:
            # Send in chunks of 4 KB to avoid large WebSocket frames
            chunk_size = 4096
            for i in range(0, len(opus_audio), chunk_size):
                chunk = opus_audio[i: i + chunk_size]
                await websocket.send_bytes(chunk)

        # 8. Signal TTS stop
        await websocket.send_text(json.dumps({
            "type": "tts",
            "state": "stop",
        }))

        print(f"[Pipeline] Done for session {session_id}")

    except Exception as e:
        print(f"[Pipeline] Error in pipeline: {e}")
        try:
            await websocket.send_text(json.dumps({"type": "tts", "state": "stop"}))
        except Exception:
            pass


async def _query_kb(query: str) -> str:
    """
    POST to the external KB endpoint and return the answer string.
    Endpoint: http://18.234.117.250:5000/api/v1/kb/f9549db9/query
    Expected response: {"answer": "...", "context": [...], "latency": ...}
    """
    try:
        async with httpx.AsyncClient(timeout=60.0) as client:
            response = await client.post(
                settings.KB_ENDPOINT,
                json={"query": query},
            )
            response.raise_for_status()
            data = response.json()
            return data.get("answer", "")
    except Exception as e:
        print(f"[KB] Error querying KB endpoint: {e}")
        return ""
