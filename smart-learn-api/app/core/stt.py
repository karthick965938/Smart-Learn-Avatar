import io
import asyncio
import subprocess
import tempfile
import os
from openai import AsyncOpenAI
from app.config import settings

client = AsyncOpenAI(api_key=settings.OPENAI_API_KEY)


async def transcribe_opus(opus_frames: list[bytes], sample_rate: int = 16000) -> str:
    """
    Decode a list of raw Opus frames to PCM WAV using ffmpeg, then transcribe with Whisper.
    ffmpeg is used because opuslib wheel is not always available on all platforms.
    """
    if not opus_frames:
        return ""

    # Concatenate all raw opus frames into one blob
    raw_opus = b"".join(opus_frames)

    try:
        # Use ffmpeg to decode Opus → PCM WAV in memory
        proc = await asyncio.create_subprocess_exec(
            "ffmpeg",
            "-f", "opus",         # input format hint
            "-i", "pipe:0",       # read from stdin
            "-f", "wav",          # output: WAV
            "-ar", str(sample_rate),
            "-ac", "1",
            "pipe:1",             # write to stdout
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.DEVNULL,
        )
        wav_data, _ = await proc.communicate(input=raw_opus)

        if not wav_data or len(wav_data) < 44:
            return ""

        # Send WAV to OpenAI Whisper
        audio_file = io.BytesIO(wav_data)
        audio_file.name = "audio.wav"

        transcript = await client.audio.transcriptions.create(
            model="whisper-1",
            file=audio_file,
            language="en",
        )
        return transcript.text.strip()

    except Exception as e:
        print(f"[STT] Error transcribing audio: {e}")
        return ""
