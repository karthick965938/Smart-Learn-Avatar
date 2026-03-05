"""
XiaoZhi-compatible WebSocket server with server-side end-of-speech detection.

KEY INSIGHT: When the device uses auto mode without AFE/VAD, it sends audio frames
indefinitely and expects the SERVER to detect silence and trigger the pipeline.

Server-side EOS detection strategy:
  - Wait for at least MIN_SPEECH_FRAMES of audio (ensures we captured some speech)
  - After that, monitor frame sizes: small Opus frames = silence/comfort noise
  - If SILENCE_FRAMES_THRESHOLD consecutive small frames detected → trigger pipeline
  - Hard cap at MAX_FRAMES (absolute timeout) to avoid infinite collection

Binary frame format (version=1): raw Opus bytes, no header
Binary frame format (version=3): 4-byte BinaryProtocol3 header + Opus payload

CLIENT → SERVER (TEXT):
  {"type":"hello", "version":1, ...}
  {"session_id":"...", "type":"listen", "state":"detect", "text":"<wake word>"}
  {"session_id":"...", "type":"listen", "state":"start",  "mode":"auto"}
  {"session_id":"...", "type":"listen", "state":"stop"}
  {"session_id":"...", "type":"abort"}

SERVER → CLIENT (TEXT):
  {"type":"hello", "transport":"websocket", "session_id":"...", "audio_params":{...}}
  {"type":"stt",  "text":"<what user said>"}
  {"type":"llm",  "emotion":"neutral"}
  {"type":"tts",  "state":"start"}
  {"type":"tts",  "state":"sentence_start", "text":"<sentence>"}
  {"type":"tts",  "state":"stop"}

SERVER → CLIENT (BINARY):
  Opus audio frames for TTS reply
"""

import json
import uuid
import struct
import asyncio
import traceback
import httpx
from fastapi import APIRouter, WebSocket, WebSocketDisconnect

from app.core.stt import transcribe_opus
from app.core.tts import text_to_opus_frames
from app.config import settings

router = APIRouter()

# ── Server-side Voice Activity Detection (VAD) tuning ───────────────────────
# At 60ms/frame: 17 frames ≈ 1s, 50 ≈ 3s, 200 ≈ 12s
SPEECH_FRAME_BYTES       = 80  # Opus frames >= this byte size = real speech
SILENCE_FRAMES_THRESHOLD = 8   # Consecutive small frames = end of speech (~480ms)
MIN_SPEECH_FRAMES        = 10  # Must detect at least this many SPEECH frames to send to LLM
MAX_FRAMES               = 200 # Hard cap; force trigger or skip after this many frames


def _strip_v3_header(data: bytes) -> bytes:
    """
    Strip the 4-byte BinaryProtocol3 header:
      byte 0:   type      (1 byte)
      byte 1:   reserved  (1 byte)
      bytes 2-3: payload_size (uint16 big-endian)
      bytes 4+:  Opus payload
    """
    if len(data) < 4:
        return data
    try:
        payload_size = struct.unpack_from(">H", data, 2)[0]
        if 4 + payload_size <= len(data):
            return data[4: 4 + payload_size]
    except Exception:
        pass
    return data


def _is_speech_frame(frame: bytes) -> bool:
    """Heuristic: larger Opus frames carry actual speech content."""
    return len(frame) >= SPEECH_FRAME_BYTES


@router.websocket("/ws/xiaozhi")
async def xiaozhi_websocket(websocket: WebSocket):
    await websocket.accept()
    session_id = str(uuid.uuid4())
    print(f"\n[WS] ===== New connection — session {session_id[:8]} =====")

    sample_rate    = 16000
    frame_duration = 60
    proto_version  = 1

    is_listening     = False
    audio_frames: list[bytes] = []
    abort_flag       = False
    frame_count      = 0
    silence_streak   = 0
    speech_frame_count = 0   # counts frames that are actual speech (not silence)
    pipeline_task    = None

    try:
        # ── Hello handshake ──────────────────────────────────────────────────
        raw = await websocket.receive()
        if raw.get("type") == "websocket.disconnect":
            return

        hello_text = raw.get("text") or (raw.get("bytes") or b"").decode("utf-8", errors="replace")
        print(f"[WS] Raw hello: {hello_text[:300]}")

        try:
            hello = json.loads(hello_text)
        except Exception as e:
            print(f"[WS] Bad hello JSON: {e}")
            await websocket.close(code=1008)
            return

        if hello.get("type") != "hello":
            print(f"[WS] Expected hello, got: {hello.get('type')}")
            await websocket.close(code=1008)
            return

        proto_version  = hello.get("version", 1)
        ap             = hello.get("audio_params", {})
        sample_rate    = ap.get("sample_rate", 16000)
        frame_duration = ap.get("frame_duration", 60)
        print(f"[WS] Client hello OK — proto_version={proto_version}, sample_rate={sample_rate}, frame_duration={frame_duration}ms")

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
        print("[WS] Sent server hello OK")

        # ── Main message loop ────────────────────────────────────────────────
        while True:
            raw = await websocket.receive()

            if raw.get("type") == "websocket.disconnect":
                print("[WS] Clean disconnect")
                break

            # ── Binary: Opus audio from mic ──────────────────────────────────
            bdata = raw.get("bytes")
            if bdata is not None:
                if is_listening and not abort_flag:
                    # Strip header for version 3; version 1 is raw Opus
                    frame = _strip_v3_header(bdata) if proto_version == 3 else bdata
                    audio_frames.append(frame)
                    frame_count += 1

                    # Log progress every 1.5 seconds of audio
                    if frame_count % 25 == 0:
                        print(f"[WS] Audio: {frame_count} frames | {sum(len(f) for f in audio_frames)} bytes | last_frame={len(frame)}B")

                    # ──────────────────────────────────────────────────────────
                    # SERVER-SIDE END-OF-SPEECH DETECTION
                    # Strategy:
                    #   - Track speech_frame_count (frames with real audio energy)
                    #   - Track silence_streak (consecutive small/quiet frames)
                    #   - Only trigger the LLM pipeline when:
                    #       (a) We detected real speech (speech_frame_count >= MIN_SPEECH_FRAMES)
                    #       AND (b) Silence followed it (silence_streak >= SILENCE_FRAMES_THRESHOLD)
                    #   - If MAX_FRAMES reached without speech → skip (nobody spoke)
                    # ──────────────────────────────────────────────────────────
                    if _is_speech_frame(frame):
                        speech_frame_count += 1
                        silence_streak = 0       # reset silence streak on any speech
                    else:
                        if speech_frame_count > 0:  # only track silence after speech started
                            silence_streak += 1

                    # EOS condition: enough speech followed by enough silence
                    if speech_frame_count >= MIN_SPEECH_FRAMES and silence_streak >= SILENCE_FRAMES_THRESHOLD:
                        print(f"[VAD] Speech({speech_frame_count} frames) + Silence({silence_streak} frames) detected — triggering pipeline")
                        await _trigger_pipeline(
                            websocket, session_id, audio_frames, sample_rate, frame_duration,
                            pipeline_task=pipeline_task
                        )
                        audio_frames = []
                        frame_count = 0
                        silence_streak = 0
                        speech_frame_count = 0
                        continue

                    # Hard cap reached
                    if frame_count >= MAX_FRAMES:
                        if speech_frame_count >= MIN_SPEECH_FRAMES:
                            print(f"[VAD] MAX_FRAMES reached with speech ({speech_frame_count} frames) — force triggering")
                            await _trigger_pipeline(
                                websocket, session_id, audio_frames, sample_rate, frame_duration,
                                pipeline_task=pipeline_task
                            )
                        else:
                            print(f"[VAD] MAX_FRAMES reached but NO speech detected ({speech_frame_count}/{MIN_SPEECH_FRAMES} frames) — skipping")
                            await _safe_send(websocket, {"type": "tts", "state": "stop"})
                        audio_frames = []
                        frame_count = 0
                        silence_streak = 0
                        speech_frame_count = 0
                        continue
                continue

            # ── Text: control messages ───────────────────────────────────────
            text = raw.get("text")
            if not text:
                continue

            try:
                msg = json.loads(text)
            except Exception as e:
                print(f"[WS] Bad JSON: {e} | raw={text[:80]}")
                continue

            msg_type = msg.get("type", "")
            msg_state = msg.get("state", "")
            print(f"[WS] MSG type={msg_type} state={msg_state} mode={msg.get('mode','')} text={msg.get('text','')[:40]}")

            if msg_type == "listen":
                if msg_state == "detect":
                    # Wake word — reset and prepare to collect speech
                    is_listening = True
                    abort_flag = False
                    audio_frames = []
                    frame_count = 0
                    silence_streak = 0
                    speech_frame_count = 0
                    print(f"[WS] Wake word: '{msg.get('text', '')}'")

                elif msg_state == "start":
                    is_listening = True
                    abort_flag = False
                    audio_frames = []
                    frame_count = 0
                    silence_streak = 0
                    speech_frame_count = 0
                    print(f"[WS] Listening STARTED (mode={msg.get('mode','auto')})")

                elif msg_state == "stop":
                    # Device explicitly signaled end-of-speech (e.g. manual mode or AFE VAD)
                    is_listening = False
                    total_bytes = sum(len(f) for f in audio_frames)
                    print(f"[WS] Device sent listen:stop — {frame_count} frames, {total_bytes} bytes, speech_frames={speech_frame_count}")

                    if audio_frames and not abort_flag and speech_frame_count >= MIN_SPEECH_FRAMES:
                        await _trigger_pipeline(
                            websocket, session_id, audio_frames, sample_rate, frame_duration,
                            pipeline_task=pipeline_task
                        )
                    else:
                        if abort_flag:
                            reason = "abort active"
                        elif speech_frame_count < MIN_SPEECH_FRAMES:
                            reason = f"no real speech ({speech_frame_count}/{MIN_SPEECH_FRAMES} speech frames)"
                        else:
                            reason = "no audio frames"
                        print(f"[WS] Pipeline SKIPPED — {reason}")
                        await _safe_send(websocket, {"type": "tts", "state": "stop"})

                    audio_frames = []
                    frame_count = 0
                    silence_streak = 0
                    speech_frame_count = 0

            elif msg_type == "abort":
                print(f"[WS] ABORT — reason={msg.get('reason','')}")
                abort_flag = True
                is_listening = False
                audio_frames = []
                frame_count = 0
                silence_streak = 0
                speech_frame_count = 0

            else:
                print(f"[WS] Unhandled msg type: {msg_type}")

    except WebSocketDisconnect:
        print(f"[WS] Client disconnected — session {session_id[:8]}")
    except Exception as e:
        print(f"[WS] Unexpected error — session {session_id[:8]}: {e}")
        traceback.print_exc()
    finally:
        print(f"[WS] ===== Session {session_id[:8]} closed =====\n")


async def _trigger_pipeline(
    websocket: WebSocket,
    session_id: str,
    audio_frames: list[bytes],
    sample_rate: int,
    frame_duration: int,
    pipeline_task=None,
):
    """
    Fire the STT→KB→TTS pipeline as a background asyncio task.
    If a previous pipeline task is still running, we skip (device is already speaking).
    """
    if pipeline_task and not pipeline_task.done():
        print("[WS] Previous pipeline still running — skipping")
        return

    frames_copy = list(audio_frames)
    asyncio.ensure_future(
        _run_pipeline(websocket, session_id, frames_copy, sample_rate, frame_duration)
    )


async def _run_pipeline(
    websocket: WebSocket,
    session_id: str,
    opus_frames: list[bytes],
    sample_rate: int,
    frame_duration: int,
):
    """Full pipeline: Opus frames → STT → KB → TTS → stream audio back."""
    total_bytes = sum(len(f) for f in opus_frames)
    print(f"\n[Pipeline] ── Starting for session {session_id[:8]} ──")
    print(f"[Pipeline] Input: {len(opus_frames)} frames, {total_bytes} bytes")

    try:
        # 1. STT ─────────────────────────────────────────────────────────────
        print("[Pipeline] Step 1 ► STT (Whisper)...")
        user_text = await transcribe_opus(opus_frames, sample_rate=sample_rate, frame_duration_ms=frame_duration)
        print(f"[Pipeline] STT → '{user_text}'")

        if not user_text.strip():
            print("[Pipeline] Empty STT result — sending stop")
            await _safe_send(websocket, {"type": "tts", "state": "stop"})
            return

        # 2. Show STT text on device display ─────────────────────────────────
        await _safe_send(websocket, {"type": "stt", "text": user_text})
        await _safe_send(websocket, {"type": "llm", "emotion": "neutral"})

        # 3. Query KB ─────────────────────────────────────────────────────────
        print(f"[Pipeline] Step 2 ► KB query: '{user_text}'")
        answer = await _query_kb(user_text)
        if not answer:
            answer = "Sorry, I could not find an answer to that."
        print(f"[Pipeline] KB → '{answer[:100]}'")

        # 4. TTS start signal ─────────────────────────────────────────────────
        await _safe_send(websocket, {"type": "tts", "state": "start"})
        await _safe_send(websocket, {"type": "tts", "state": "sentence_start", "text": answer})

        # 5. Generate and stream TTS audio ────────────────────────────────────
        print("[Pipeline] Step 3 ► TTS synthesis...")
        frames = await text_to_opus_frames(answer, sample_rate=sample_rate, frame_duration_ms=frame_duration)

        if frames:
            for frame in frames:
                await websocket.send_bytes(frame)
            print(f"[Pipeline] Sent {len(frames)} Opus frames to device")
        else:
            print("[Pipeline] TTS produced no frames")

        # 6. TTS stop ─────────────────────────────────────────────────────────
        await _safe_send(websocket, {"type": "tts", "state": "stop"})
        print(f"[Pipeline] ── Done for session {session_id[:8]} ──\n")

    except Exception as e:
        print(f"[Pipeline] ERROR: {e}")
        traceback.print_exc()
        await _safe_send(websocket, {"type": "tts", "state": "stop"})


async def _safe_send(websocket: WebSocket, payload: dict):
    """Send JSON text, swallow errors if connection is gone."""
    try:
        await websocket.send_text(json.dumps(payload))
    except Exception as e:
        print(f"[WS] send_text failed: {e}")


async def _query_kb(query: str) -> str:
    """POST to KB_ENDPOINT and return the answer string."""
    try:
        async with httpx.AsyncClient(timeout=60.0) as client:
            print(f"[KB] POST {settings.KB_ENDPOINT}")
            resp = await client.post(settings.KB_ENDPOINT, json={"query": query})
            resp.raise_for_status()
            data = resp.json()
            answer = data.get("answer", "")
            print(f"[KB] Got {len(answer)} chars")
            return answer
    except Exception as e:
        print(f"[KB] Error: {e}")
        return ""
