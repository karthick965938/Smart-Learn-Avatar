"""
XiaoZhi-compatible WebSocket server.

Binary frame format (BinaryProtocol3, used when version=3):
  byte 0:   type      (0 = audio)
  byte 1:   reserved  (0)
  bytes 2-3: payload_size  (uint16, big-endian)
  bytes 4+:  actual Opus payload

CLIENT → SERVER (TEXT):
  {"type":"hello", "version":3, "features":{...}, "transport":"websocket",
   "audio_params":{"format":"opus","sample_rate":16000,"channels":1,"frame_duration":60}}
  {"session_id":"...", "type":"listen", "state":"start",  "mode":"auto|manual|realtime"}
  {"session_id":"...", "type":"listen", "state":"stop"}
  {"session_id":"...", "type":"listen", "state":"detect", "text":"<wake word>"}
  {"session_id":"...", "type":"abort"}

CLIENT → SERVER (BINARY):
  BinaryProtocol3 frames when listening (4-byte header + Opus payload)

SERVER → CLIENT (TEXT):
  {"type":"hello", "transport":"websocket", "session_id":"...",
   "audio_params":{"sample_rate":16000,"frame_duration":60}}
  {"type":"stt",  "text":"<what user said>"}
  {"type":"llm",  "emotion":"neutral"}
  {"type":"tts",  "state":"start"}
  {"type":"tts",  "state":"sentence_start", "text":"<sentence>"}
  {"type":"tts",  "state":"stop"}

SERVER → CLIENT (BINARY):
  Opus frames (TTS reply)
"""

import json
import uuid
import struct
import asyncio
import traceback
import httpx
from fastapi import APIRouter, WebSocket, WebSocketDisconnect

from app.core.stt import transcribe_opus
from app.core.tts import text_to_opus
from app.config import settings

router = APIRouter()


def _strip_binary_header(data: bytes) -> bytes:
    """
    Strip the 4-byte BinaryProtocol3 header from an incoming binary WebSocket frame.

    Layout:
        uint8   type          (1 byte)
        uint8   reserved      (1 byte)
        uint16  payload_size  (2 bytes, big-endian)
        uint8[] payload       (payload_size bytes of raw Opus)

    If the data is shorter than 4 bytes or doesn't look like a v3 frame,
    return the data as-is (handles legacy raw-Opus mode).
    """
    if len(data) < 4:
        return data
    try:
        payload_size = struct.unpack_from(">H", data, 2)[0]  # big-endian uint16 at offset 2
        if 4 + payload_size <= len(data):
            return data[4: 4 + payload_size]
    except Exception:
        pass
    return data  # fallback: return as-is


@router.websocket("/ws/xiaozhi")
async def xiaozhi_websocket(websocket: WebSocket):
    """
    XiaoZhi-compatible WebSocket endpoint.
    ESP32 device connects here after the OTA endpoint returns this URL.
    """
    await websocket.accept()
    session_id = str(uuid.uuid4())
    print(f"\n[WS] ===== New connection — session {session_id} =====")

    # Audio params negotiated from client hello
    sample_rate = 16000
    frame_duration = 60   # ms
    proto_version = 3     # default; updated from client hello

    # Session state
    is_listening = False
    audio_frames: list[bytes] = []   # stripped Opus payloads
    abort_flag = False
    frame_count = 0

    try:
        # ── Step 1: Wait for client "hello" ─────────────────────────────────
        raw = await websocket.receive()
        if raw.get("type") == "websocket.disconnect":
            print("[WS] Disconnected before hello")
            return

        hello_text = raw.get("text") or (raw.get("bytes") or b"").decode("utf-8", errors="replace")
        print(f"[WS] Raw hello: {hello_text[:200]}")

        try:
            hello = json.loads(hello_text)
        except Exception as e:
            print(f"[WS] Failed to parse hello JSON: {e}")
            await websocket.close(code=1008)
            return

        if hello.get("type") != "hello":
            print(f"[WS] Expected 'hello' but got type={hello.get('type')}")
            await websocket.close(code=1008)
            return

        proto_version = hello.get("version", 3)
        ap = hello.get("audio_params", {})
        sample_rate = ap.get("sample_rate", 16000)
        frame_duration = ap.get("frame_duration", 60)
        print(f"[WS] Client hello OK — version={proto_version}, sample_rate={sample_rate}, frame_duration={frame_duration}")

        # ── Step 2: Send server "hello" ──────────────────────────────────────
        server_hello = {
            "type": "hello",
            "version": proto_version,
            "transport": "websocket",
            "session_id": session_id,
            "audio_params": {
                "sample_rate": sample_rate,
                "frame_duration": frame_duration,
            },
        }
        await websocket.send_text(json.dumps(server_hello))
        print(f"[WS] Sent server hello OK")

        # ── Step 3: Main message loop ────────────────────────────────────────
        while True:
            raw = await websocket.receive()

            if raw.get("type") == "websocket.disconnect":
                print("[WS] Clean disconnect received")
                break

            # ── Binary frame: Opus audio from device microphone ──────────────
            bdata = raw.get("bytes")
            if bdata is not None:
                if is_listening and not abort_flag:
                    # Strip the BinaryProtocol3 header (4 bytes) to get raw Opus
                    opus_payload = _strip_binary_header(bdata) if proto_version == 3 else bdata
                    audio_frames.append(opus_payload)
                    frame_count += 1
                    if frame_count % 25 == 0:  # Log every ~1.5 sec of audio
                        print(f"[WS] Collecting audio... {frame_count} frames ({sum(len(f) for f in audio_frames)} bytes so far)")
                continue

            # ── Text frame: control messages ─────────────────────────────────
            text = raw.get("text")
            if not text:
                continue

            try:
                msg = json.loads(text)
            except Exception as e:
                print(f"[WS] Cannot parse text message: {e} | raw={text[:100]}")
                continue

            msg_type = msg.get("type", "")
            print(f"[WS] Control message: type={msg_type}, state={msg.get('state','')}, text={msg.get('text','')[:50]}")

            if msg_type == "listen":
                state = msg.get("state", "")

                if state == "detect":
                    # Wake word audio data may arrive just before this message
                    is_listening = True
                    abort_flag = False
                    audio_frames = []
                    frame_count = 0
                    print(f"[WS] Wake word detected: '{msg.get('text', '')}'")

                elif state == "start":
                    is_listening = True
                    abort_flag = False
                    audio_frames = []
                    frame_count = 0
                    print(f"[WS] Listening STARTED — mode={msg.get('mode', 'auto')}")

                elif state == "stop":
                    is_listening = False
                    total_frames = len(audio_frames)
                    total_bytes = sum(len(f) for f in audio_frames)
                    print(f"[WS] Listening STOPPED — collected {total_frames} frames ({total_bytes} bytes of Opus)")

                    if audio_frames and not abort_flag:
                        frames_copy = audio_frames[:]
                        audio_frames = []
                        frame_count = 0
                        print(f"[WS] Launching STT→KB→TTS pipeline...")
                        asyncio.ensure_future(
                            _handle_query(websocket, session_id, frames_copy, sample_rate, frame_duration)
                        )
                    else:
                        audio_frames = []
                        frame_count = 0
                        if abort_flag:
                            print("[WS] Pipeline skipped — abort was active")
                        else:
                            print("[WS] Pipeline skipped — no audio frames collected")
                            # Inform device so it doesn't hang
                            try:
                                await websocket.send_text(json.dumps({"type": "tts", "state": "stop"}))
                            except Exception:
                                pass

            elif msg_type == "abort":
                reason = msg.get("reason", "")
                print(f"[WS] ABORT received — reason={reason}")
                abort_flag = True
                is_listening = False
                audio_frames = []
                frame_count = 0

            else:
                print(f"[WS] Unhandled message type: {msg_type}")

    except WebSocketDisconnect:
        print(f"[WS] Client disconnected — session {session_id}")
    except Exception as e:
        print(f"[WS] Unexpected error in session {session_id}: {e}")
        traceback.print_exc()
    finally:
        print(f"[WS] ===== Session {session_id} closed =====\n")


async def _handle_query(
    websocket: WebSocket,
    session_id: str,
    opus_frames: list[bytes],
    sample_rate: int,
    frame_duration: int,
):
    """
    Full pipeline: Opus payload bytes → STT → KB query → TTS → stream back.
    Always sends tts:stop at the end so the device doesn't hang.
    """
    print(f"[Pipeline] Starting for session {session_id}")
    print(f"[Pipeline] Input: {len(opus_frames)} frames, {sum(len(f) for f in opus_frames)} total bytes")

    try:
        # ─ 1. Speech → Text ─────────────────────────────────────────────────
        print("[Pipeline] Step 1: Running STT (Whisper)...")
        user_text = await transcribe_opus(opus_frames, sample_rate=sample_rate, frame_duration_ms=frame_duration)
        print(f"[Pipeline] STT result: '{user_text}'")

        if not user_text.strip():
            print("[Pipeline] STT returned empty — sending silent stop")
            await _safe_send_text(websocket, {"type": "tts", "state": "stop"})
            return

        # ─ 2. Show STT text on device display ───────────────────────────────
        await _safe_send_text(websocket, {"type": "stt", "text": user_text})

        # ─ 3. Send emotion (shown on display face) ───────────────────────────
        await _safe_send_text(websocket, {"type": "llm", "emotion": "neutral"})

        # ─ 4. Query knowledge base ──────────────────────────────────────────
        print(f"[Pipeline] Step 2: Querying KB at {settings.KB_ENDPOINT}...")
        answer = await _query_kb(user_text)
        if not answer:
            answer = "Sorry, I could not find an answer to that."
        print(f"[Pipeline] KB answer: '{answer}'")

        # ─ 5. Signal TTS start ───────────────────────────────────────────────
        await _safe_send_text(websocket, {"type": "tts", "state": "start"})

        # ─ 6. Send sentence subtitle ────────────────────────────────────────
        await _safe_send_text(websocket, {"type": "tts", "state": "sentence_start", "text": answer})

        # ─ 7. Synthesize reply and stream Opus audio ─────────────────────────
        print("[Pipeline] Step 3: Running TTS...")
        opus_audio = await text_to_opus(answer, sample_rate=sample_rate)

        if opus_audio:
            print(f"[Pipeline] Streaming {len(opus_audio)} bytes of TTS audio in chunks...")
            chunk_size = 960  # ~60ms at 16kHz mono 16-bit
            sent = 0
            for i in range(0, len(opus_audio), chunk_size):
                chunk = opus_audio[i: i + chunk_size]
                await websocket.send_bytes(chunk)
                sent += len(chunk)
            print(f"[Pipeline] Sent {sent} bytes of audio")
        else:
            print("[Pipeline] TTS produced no audio")

        # ─ 8. Signal TTS stop ────────────────────────────────────────────────
        await _safe_send_text(websocket, {"type": "tts", "state": "stop"})
        print(f"[Pipeline] Done for session {session_id}")

    except Exception as e:
        print(f"[Pipeline] ERROR: {e}")
        traceback.print_exc()
        # Always send stop so device doesn't hang in speaking state
        await _safe_send_text(websocket, {"type": "tts", "state": "stop"})


async def _safe_send_text(websocket: WebSocket, payload: dict):
    """Send a JSON text message, swallowing errors if the connection has closed."""
    try:
        await websocket.send_text(json.dumps(payload))
    except Exception as e:
        print(f"[WS] send_text failed: {e}")


async def _query_kb(query: str) -> str:
    """
    POST to KB_ENDPOINT and return the answer string.
    Expected response: {"answer": "...", "context": [...], "latency": ...}
    """
    try:
        async with httpx.AsyncClient(timeout=60.0) as client:
            print(f"[KB] POST {settings.KB_ENDPOINT} query='{query}'")
            response = await client.post(
                settings.KB_ENDPOINT,
                json={"query": query},
            )
            response.raise_for_status()
            data = response.json()
            answer = data.get("answer", "")
            print(f"[KB] Got answer ({len(answer)} chars)")
            return answer
    except Exception as e:
        print(f"[KB] Error: {e}")
        return ""
