"""
Speech-to-text using OpenAI Whisper.

The ESP32 sends raw Opus payloads (no OGG container, no header). 
We must encode them into a valid audio file before sending to Whisper.

Pipeline:
  raw Opus bytes ──(ffmpeg)──► PCM WAV ──(Whisper)──► transcript text
"""

import io
import asyncio
from openai import AsyncOpenAI
from app.config import settings

client = AsyncOpenAI(api_key=settings.OPENAI_API_KEY)


async def transcribe_opus(
    opus_payloads: list[bytes],
    sample_rate: int = 16000,
    frame_duration_ms: int = 60,
) -> str:
    """
    Transcribe a list of raw Opus payload bytes using Whisper.

    The raw payloads are fed to ffmpeg which decodes them to PCM WAV.
    The WAV is then sent to OpenAI Whisper for transcription.
    """
    if not opus_payloads:
        print("[STT] No audio frames to transcribe")
        return ""

    raw_opus = b"".join(opus_payloads)
    total_bytes = len(raw_opus)
    print(f"[STT] {len(opus_payloads)} frames, {total_bytes} bytes of raw Opus")

    if total_bytes < 200:
        print("[STT] Too little audio — skipping")
        return ""

    # Strategy 1: ffmpeg raw Opus → WAV
    wav_data = await _opus_to_wav_ffmpeg(raw_opus, sample_rate)

    if wav_data and len(wav_data) > 44:
        print(f"[STT] ffmpeg → {len(wav_data)} byte WAV — sending to Whisper")
        return await _call_whisper(wav_data, filename="audio.wav")

    # Strategy 2: ffmpeg raw PCM → WAV (treat raw bytes as PCM)
    print("[STT] Trying strategy 2: treating as raw PCM s16le...")
    wav_pcm = await _pcm_to_wav_ffmpeg(raw_opus, sample_rate)
    if wav_pcm and len(wav_pcm) > 44:
        print(f"[STT] PCM → {len(wav_pcm)} byte WAV — sending to Whisper")
        return await _call_whisper(wav_pcm, filename="audio.wav")

    print("[STT] All strategies failed — returning empty")
    return ""


async def _opus_to_wav_ffmpeg(raw_opus: bytes, sample_rate: int) -> bytes:
    """
    Use ffmpeg to decode raw Opus bytes to PCM WAV.
    ffmpeg is told to assume the input is Opus audio in a pipe.
    """
    try:
        proc = await asyncio.create_subprocess_exec(
            "ffmpeg",
            "-y",
            "-f", "opus",             # Tell ffmpeg: input is raw Opus
            "-ar", str(sample_rate),
            "-ac", "1",
            "-i", "pipe:0",           # Read from stdin
            "-f", "wav",              # Output WAV
            "-ar", str(sample_rate),
            "-ac", "1",
            "-acodec", "pcm_s16le",
            "pipe:1",                 # Write to stdout
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )
        wav_data, stderr = await proc.communicate(input=raw_opus)
        stderr_str = stderr.decode("utf-8", errors="replace") if stderr else ""

        if proc.returncode == 0 and len(wav_data) > 44:
            print(f"[STT][ffmpeg] raw opus → WAV: {len(wav_data)} bytes")
            return wav_data

        # If raw opus failed, try wrapping in ogg container first
        print(f"[STT][ffmpeg] raw opus failed (code={proc.returncode}), trying ogg container...")
        ogg_data = await _wrap_opus_in_ogg(raw_opus, sample_rate)
        if ogg_data:
            return await _ogg_to_wav_ffmpeg(ogg_data, sample_rate)

        print(f"[STT][ffmpeg] stderr: {stderr_str[-400:]}")
        return b""

    except FileNotFoundError:
        print("[STT] ffmpeg not found! Add 'ffmpeg' to Dockerfile apt-get install")
        return b""
    except Exception as e:
        print(f"[STT][ffmpeg] Unexpected error: {e}")
        return b""


async def _wrap_opus_in_ogg(raw_opus: bytes, sample_rate: int) -> bytes:
    """
    Wrap raw Opus data in an OGG container using ffmpeg.
    Input: raw Opus bytes (no container)
    Output: valid .ogg file bytes
    """
    try:
        proc = await asyncio.create_subprocess_exec(
            "ffmpeg",
            "-y",
            "-f", "opus",
            "-ar", str(sample_rate),
            "-ac", "1",
            "-i", "pipe:0",
            "-f", "ogg",              # Output OGG container
            "-acodec", "copy",        # Copy the Opus stream as-is
            "pipe:1",
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.DEVNULL,
        )
        ogg_data, _ = await proc.communicate(input=raw_opus)
        if proc.returncode == 0 and ogg_data:
            print(f"[STT][ffmpeg] Wrapped in OGG: {len(ogg_data)} bytes")
            return ogg_data
        return b""
    except Exception as e:
        print(f"[STT] OGG wrap error: {e}")
        return b""


async def _ogg_to_wav_ffmpeg(ogg_data: bytes, sample_rate: int) -> bytes:
    """Convert proper OGG container to WAV via ffmpeg."""
    try:
        proc = await asyncio.create_subprocess_exec(
            "ffmpeg",
            "-y",
            "-i", "pipe:0",
            "-f", "wav",
            "-ar", str(sample_rate),
            "-ac", "1",
            "-acodec", "pcm_s16le",
            "pipe:1",
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.DEVNULL,
        )
        wav_data, _ = await proc.communicate(input=ogg_data)
        if proc.returncode == 0 and len(wav_data) > 44:
            print(f"[STT][ffmpeg] OGG → WAV: {len(wav_data)} bytes")
            return wav_data
        return b""
    except Exception as e:
        print(f"[STT] OGG→WAV error: {e}")
        return b""


async def _pcm_to_wav_ffmpeg(raw_bytes: bytes, sample_rate: int) -> bytes:
    """Fallback: treat raw bytes as signed-16-bit PCM and convert to WAV."""
    try:
        proc = await asyncio.create_subprocess_exec(
            "ffmpeg",
            "-y",
            "-f", "s16le",
            "-ar", str(sample_rate),
            "-ac", "1",
            "-i", "pipe:0",
            "-f", "wav",
            "-ar", str(sample_rate),
            "-ac", "1",
            "pipe:1",
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.DEVNULL,
        )
        wav_data, _ = await proc.communicate(input=raw_bytes)
        if proc.returncode == 0 and len(wav_data) > 44:
            return wav_data
        return b""
    except Exception as e:
        print(f"[STT] PCM→WAV error: {e}")
        return b""


async def _call_whisper(audio_bytes: bytes, filename: str = "audio.wav") -> str:
    """Send audio bytes to OpenAI Whisper and return the transcript."""
    try:
        audio_file = io.BytesIO(audio_bytes)
        audio_file.name = filename
        transcript = await client.audio.transcriptions.create(
            model="whisper-1",
            file=audio_file,
        )
        result = transcript.text.strip()
        print(f"[STT] Whisper → '{result}'")
        return result
    except Exception as e:
        print(f"[STT] Whisper API error: {e}")
        return ""
