"""
Speech-to-text using OpenAI Whisper.

Accepts a list of raw Opus payload bytes (with the BinaryProtocol3 header
ALREADY stripped), concatenates them, wraps in an OGG container via ffmpeg,
and sends to Whisper.
"""

import io
import asyncio
import struct
from openai import AsyncOpenAI
from app.config import settings

client = AsyncOpenAI(api_key=settings.OPENAI_API_KEY)


async def transcribe_opus(
    opus_payloads: list[bytes],
    sample_rate: int = 16000,
    frame_duration_ms: int = 60,
) -> str:
    """
    Transcribe a list of raw Opus payload buffers using Whisper.

    Approach:
      1. Concatenate all Opus payloads into one blob.
      2. Use ffmpeg to wrap them in a WAV container (16-bit PCM).
         ffmpeg is smart enough to handle raw Opus when given the right input hints.
      3. Send the WAV bytes to Whisper as audio/wav.

    Falls back gracefully if ffmpeg isn't available.
    """
    if not opus_payloads:
        print("[STT] No audio frames to transcribe")
        return ""

    # Concatenate all raw Opus payloads
    raw_opus = b"".join(opus_payloads)
    total_bytes = len(raw_opus)
    print(f"[STT] Got {len(opus_payloads)} frames, {total_bytes} bytes of raw Opus")

    if total_bytes < 100:
        print("[STT] Too little audio data — skipping")
        return ""

    try:
        # Use ffmpeg to decode raw Opus → PCM WAV
        # -f opus tells ffmpeg to treat stdin as raw Opus data
        proc = await asyncio.create_subprocess_exec(
            "ffmpeg",
            "-y",
            "-f", "opus",
            "-ar", str(sample_rate),
            "-ac", "1",
            "-i", "pipe:0",
            "-f", "wav",
            "-ar", str(sample_rate),
            "-ac", "1",
            "-acodec", "pcm_s16le",
            "pipe:1",
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )
        wav_data, stderr = await proc.communicate(input=raw_opus)
        stderr_text = stderr.decode("utf-8", errors="replace") if stderr else ""

        if proc.returncode != 0:
            print(f"[STT] ffmpeg exited with code {proc.returncode}")
            print(f"[STT] ffmpeg stderr: {stderr_text[-500:]}")
            # Fallback: try as raw pcm
            return await _whisper_from_raw_opus(raw_opus, sample_rate)

        if len(wav_data) < 44:
            print(f"[STT] ffmpeg produced too little output ({len(wav_data)} bytes). stderr: {stderr_text[-300:]}")
            return await _whisper_from_raw_opus(raw_opus, sample_rate)

        print(f"[STT] ffmpeg produced {len(wav_data)} byte WAV — sending to Whisper")

        # Send WAV to Whisper
        audio_file = io.BytesIO(wav_data)
        audio_file.name = "audio.wav"

        transcript = await client.audio.transcriptions.create(
            model="whisper-1",
            file=audio_file,
        )
        result = transcript.text.strip()
        print(f"[STT] Whisper result: '{result}'")
        return result

    except FileNotFoundError:
        print("[STT] ffmpeg not found! Please install it: sudo apt install ffmpeg")
        return await _whisper_from_raw_opus(raw_opus, sample_rate)
    except Exception as e:
        print(f"[STT] Error during transcription: {e}")
        import traceback
        traceback.print_exc()
        return ""


async def _whisper_from_raw_opus(raw_opus: bytes, sample_rate: int) -> str:
    """
    Fallback: send raw Opus bytes directly to Whisper wrapped as .ogg.
    OpenAI Whisper can handle ogg/opus files natively.
    """
    try:
        print("[STT] Trying fallback: sending raw opus as .ogg to Whisper...")
        audio_file = io.BytesIO(raw_opus)
        audio_file.name = "audio.ogg"   # Whisper accepts ogg
        transcript = await client.audio.transcriptions.create(
            model="whisper-1",
            file=audio_file,
        )
        result = transcript.text.strip()
        print(f"[STT] Fallback Whisper result: '{result}'")
        return result
    except Exception as e:
        print(f"[STT] Fallback also failed: {e}")
        return ""
