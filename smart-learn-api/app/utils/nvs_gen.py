"""
Generate NVS partition binary using the official ESP-IDF NVS Partition Generator.
Partition: nvs @ 0x9000, size 0x4000. Namespace: "configuration".

NVS key strings MUST match firmware exactly (factory_nvs/main/main.c, main/settings/settings.c).
Do not rename these — existing .bin firmware will not find them.

  API/request field  →  NVS key (device)
  -----------------     -----------------
  ssid                →  ssid
  password            →  password
  openai_key          →  ChatGPT_key      (device uses this name for OpenAI API key)
  base_url            →  Base_url
  kb_url              →  KB_url
  tts_voice           →  tts_voice
  theme (→ "0"/"1")   →  theme_type

Note: factory_nvs.bin at 0x700000 is the UF2 app; it reads the NVS partition at 0x9000.
"""

import csv
import os
import subprocess
import sys
import tempfile

# Must match partitions.csv: nvs @ 0x9000, size 0x4000
NVS_PARTITION_SIZE = 0x4000

# Device NVS keys — must match factory_nvs/main/main.c and main/settings/settings.c exactly.
# Do not change; firmware and CONFIG.INI (esp_tinyuf2) expect these.
NVS_KEY_SSID = "ssid"
NVS_KEY_PASSWORD = "password"
NVS_KEY_CHATGPT_KEY = "ChatGPT_key"   # OpenAI API key
NVS_KEY_BASE_URL = "Base_url"
NVS_KEY_KB_URL = "KB_url"
NVS_KEY_TTS_VOICE = "tts_voice"
NVS_KEY_THEME_TYPE = "theme_type"


def generate_nvs(
    *,
    ssid: str,
    password: str,
    openai_key: str,
    base_url: str,
    kb_url: str,
    tts_voice: str,
    theme_type: str,
) -> bytes:
    """
    Build a CSV and run esp_idf_nvs_partition_gen. Returns NVS binary.
    openai_key is written under NVS key "ChatGPT_key"; base_url under "Base_url"; kb_url under "KB_url".
    """
    try:
        import esp_idf_nvs_partition_gen  # noqa: F401
    except ImportError as e:
        raise RuntimeError(
            "esp-idf-nvs-partition-gen is not installed. "
            "Run: pip install esp-idf-nvs-partition-gen   (or: pip install -e .)"
        ) from e

    def _s(v): return "" if v is None else str(v).strip()

    rows = [
        ("key", "type", "encoding", "value"),
        ("configuration", "namespace", "", ""),
        (NVS_KEY_SSID, "data", "string", _s(ssid)),
        (NVS_KEY_PASSWORD, "data", "string", _s(password)),
        (NVS_KEY_CHATGPT_KEY, "data", "string", _s(openai_key)),
        (NVS_KEY_BASE_URL, "data", "string", _s(base_url)),
        (NVS_KEY_KB_URL, "data", "string", _s(kb_url)),
        (NVS_KEY_TTS_VOICE, "data", "string", _s(tts_voice)),
        (NVS_KEY_THEME_TYPE, "data", "string", _s(theme_type)),
    ]

    with tempfile.TemporaryDirectory() as tmp:
        csv_path = os.path.join(tmp, "nvs.csv")
        out_path = os.path.join(tmp, "nvs.bin")

        with open(csv_path, "w", newline="", encoding="utf-8") as f:
            w = csv.writer(f, lineterminator="\n")
            for r in rows:
                w.writerow(r)

        size_hex = f"0x{NVS_PARTITION_SIZE:x}"
        cmd = [
            sys.executable, "-m", "esp_idf_nvs_partition_gen",
            "generate", csv_path, out_path, size_hex,
        ]
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.returncode != 0:
            err = (result.stderr or result.stdout or "").strip()
            raise RuntimeError(f"esp_idf_nvs_partition_gen failed: {err}")

        with open(out_path, "rb") as f:
            data = f.read()

    if len(data) != NVS_PARTITION_SIZE:
        # Pad with 0xFF or truncate to match; official tool may use one page
        if len(data) < NVS_PARTITION_SIZE:
            data = data + bytes([0xFF] * (NVS_PARTITION_SIZE - len(data)))
        else:
            data = data[:NVS_PARTITION_SIZE]
    return data
