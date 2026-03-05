import asyncio
from openai import AsyncOpenAI
from app.config import settings
from app.core.ogg_utils import parse_ogg_packets

client = AsyncOpenAI(api_key=settings.OPENAI_API_KEY)

async def text_to_opus_frames(text: str, sample_rate: int = 16000, frame_duration_ms: int = 60) -> list[bytes]:
    """
    Convert text → individual raw Opus frames suitable for sending to ESP32.
    Each returned bytes item = one WebSocket binary message = one Opus frame.
    """
    if not text.strip():
        return []
    try:
        # 1. OpenAI TTS → MP3
        resp = await client.audio.speech.create(
            model="tts-1", voice="alloy", input=text, response_format="mp3"
        )
        mp3 = resp.content
        print(f"[TTS] Got {len(mp3)} bytes of MP3 from OpenAI")

        # 2. MP3 → OGG/Opus at device sample rate, with correct frame duration
        ogg = await _mp3_to_ogg(mp3, sample_rate, frame_duration_ms)
        if not ogg:
            return []
        print(f"[TTS] OGG encoded: {len(ogg)} bytes")

        # 3. Parse OGG → individual Opus frames
        frames = parse_ogg_packets(ogg)
        print(f"[TTS] Parsed {len(frames)} Opus frames ({frame_duration_ms}ms @ {sample_rate}Hz)")
        return frames

    except Exception as e:
        print(f"[TTS] Error: {e}")
        return []

async def _mp3_to_ogg(mp3: bytes, sample_rate: int, frame_duration_ms: int) -> bytes:
    try:
        proc = await asyncio.create_subprocess_exec(
            "ffmpeg", "-y",
            "-i", "pipe:0",
            "-f", "ogg",
            "-acodec", "libopus",
            "-ar", str(sample_rate),
            "-ac", "1",
            "-b:a", "24k",
            "-frame_duration", str(frame_duration_ms),
            "pipe:1",
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )
        ogg, err = await proc.communicate(input=mp3)
        if proc.returncode != 0:
            print(f"[TTS][ffmpeg] code={proc.returncode}: {err.decode(errors='replace')[-300:]}")
            return b""
        return ogg
    except FileNotFoundError:
        print("[TTS] ffmpeg not found")
        return b""
    except Exception as e:
        print(f"[TTS] ffmpeg error: {e}")
        return b""
