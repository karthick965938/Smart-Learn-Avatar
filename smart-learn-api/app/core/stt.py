import io, asyncio
from openai import AsyncOpenAI
from app.config import settings
from app.core.ogg_utils import build_ogg_opus

client = AsyncOpenAI(api_key=settings.OPENAI_API_KEY)

async def transcribe_opus(packets: list[bytes], sample_rate: int = 16000, frame_duration_ms: int = 60) -> str:
    if not packets:
        return ""
    total = sum(len(p) for p in packets)
    print(f"[STT] {len(packets)} frames, {total} bytes")
    if total < 200:
        return ""

    # Wrap raw Opus packets in a proper OGG container
    ogg = build_ogg_opus(packets, sample_rate=sample_rate, frame_duration_ms=frame_duration_ms)
    print(f"[STT] Built OGG container: {len(ogg)} bytes")

    # Decode OGG/Opus → WAV via ffmpeg
    wav = await _run_ffmpeg(
        ogg,
        ["-f", "ogg", "-i", "pipe:0",
         "-f", "wav", "-ar", str(sample_rate), "-ac", "1", "-acodec", "pcm_s16le", "pipe:1"]
    )
    if not wav or len(wav) < 44:
        print("[STT] ffmpeg OGG→WAV failed — skipping")
        return ""

    print(f"[STT] Decoded to WAV: {len(wav)} bytes — sending to Whisper")
    try:
        buf = io.BytesIO(wav); buf.name = "audio.wav"
        t = await client.audio.transcriptions.create(model="whisper-1", file=buf)
        result = t.text.strip()
        print(f"[STT] Whisper → '{result}'")
        return result
    except Exception as e:
        print(f"[STT] Whisper error: {e}")
        return ""

async def _run_ffmpeg(data: bytes, args: list[str]) -> bytes:
    try:
        proc = await asyncio.create_subprocess_exec(
            "ffmpeg", "-y", *args,
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )
        out, err = await proc.communicate(input=data)
        if proc.returncode != 0:
            print(f"[ffmpeg] code={proc.returncode}: {err.decode(errors='replace')[-300:]}")
            return b""
        return out
    except FileNotFoundError:
        print("[ffmpeg] Not found! Install: sudo apt install ffmpeg")
        return b""
    except Exception as e:
        print(f"[ffmpeg] Error: {e}")
        return b""
