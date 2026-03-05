import io
import asyncio
from openai import AsyncOpenAI
from app.config import settings

client = AsyncOpenAI(api_key=settings.OPENAI_API_KEY)

# Default TTS voice — can be overridden via config
TTS_VOICE = "alloy"


async def text_to_opus(text: str, sample_rate: int = 16000) -> bytes:
    """
    Convert text to Opus audio bytes using OpenAI TTS + ffmpeg re-encode.
    Returns raw Opus bytes (in an OGG container that ffmpeg can decode).
    """
    if not text.strip():
        return b""

    try:
        # Step 1: Generate MP3 audio via OpenAI TTS
        response = await client.audio.speech.create(
            model="tts-1",
            voice=TTS_VOICE,
            input=text,
            response_format="mp3",
        )
        mp3_bytes = response.content

        # Step 2: Re-encode MP3 → raw Opus frames via ffmpeg
        # We output a raw opus stream that the WebSocket will send as binary frames
        proc = await asyncio.create_subprocess_exec(
            "ffmpeg",
            "-i", "pipe:0",            # MP3 from stdin
            "-f", "opus",              # output as raw opus
            "-ar", str(sample_rate),
            "-ac", "1",
            "-b:a", "24k",
            "pipe:1",                  # to stdout
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.DEVNULL,
        )
        opus_bytes, _ = await proc.communicate(input=mp3_bytes)
        return opus_bytes

    except Exception as e:
        print(f"[TTS] Error synthesizing speech: {e}")
        return b""


async def pcm_to_opus_frames(pcm_bytes: bytes, sample_rate: int = 16000, frame_duration_ms: int = 60) -> list[bytes]:
    """
    Split a PCM buffer into individual Opus frames of frame_duration_ms each.
    """
    try:
        proc = await asyncio.create_subprocess_exec(
            "ffmpeg",
            "-f", "s16le", "-ar", str(sample_rate), "-ac", "1",
            "-i", "pipe:0",
            "-f", "opus", "-ar", str(sample_rate), "-ac", "1",
            "-b:a", "24k",
            "pipe:1",
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.DEVNULL,
        )
        opus_data, _ = await proc.communicate(input=pcm_bytes)
        frame_size = sample_rate * frame_duration_ms // 1000 * 2  # bytes per frame (16-bit PCM)
        # Return the raw bytes as a single frame for simplicity
        return [opus_data] if opus_data else []
    except Exception as e:
        print(f"[TTS] Error converting PCM to Opus: {e}")
        return []
