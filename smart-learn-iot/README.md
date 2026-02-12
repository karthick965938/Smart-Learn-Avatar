# 🤖 Smart Learn IoT (Firmware)

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.3+-E7352C?style=for-the-badge&logo=espressif&logoColor=white)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/index.html)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32--S3--BOX--3-blue?style=for-the-badge&logo=espressif)](https://github.com/espressif/esp-box)

**Smart Learn IoT** is the firmware powering the Smart Learn Avatar hardware. It transforms an **ESP32-S3-BOX-3** into a voice-interactive AI tutor that can talk to your custom knowledge bases in real-time.

[![Video Demo](https://img.shields.io/badge/Demo-Voice%20Avatar-blue?style=for-the-badge&logo=youtube)](https://www.youtube.com/watch?v=sbAEzvDquOA)

---


## 🚀 Key Features

- **🎙️ Voice Interaction**: Hands-free engagement with the AI using system-level speech recognition (STT).
- **📚 RAG Integration**: Queries the **Smart Learn API** to provide answers grounded in your custom documents.
- **🔊 Natural Voice Synthesis**: Uses OpenAI's TTS engine for expressive, high-quality audio responses.
- **🎭 Animated Avatar**: Dynamic UI with lip-sync animations and personality-driven expressions.
- **💬 Live Subtitles**: Real-time text display of the AI's response for better accessibility and learning.
- **🎨 Personalized Experience**: Syncs with web-configured themes, voices, and AI personalities.
- **📡 WiFi & NVS Provisioning**: Securely stores credentials and API keys in a dedicated NVS partition.

---

## 🛠️ Hardware Requirements

- **Device**: [ESP32-S3-BOX-3](https://github.com/espressif/esp-box)
- **Connectivity**: USB-C for flashing and power.

---

## ⚙️ Getting Started

### Prerequisites

1. **ESP-IDF**: Version 5.3 or higher. [Installation Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html).
2. **Knowledge Base API**: Ensure the [Smart Learn API](../smart-learn-api/README.md) is running and accessible.

### 1. Project Setup

Navigate to the project directory:

```bash
cd smart-learn-iot/smart-learn
```

### 2. Build the Firmware

Set the target and build the project:

```bash
idf.py set-target esp32s3
idf.py build
```

### 3. Flash the Device

Flash the compiled app to the device:

```bash
idf.py flash monitor
```

### 4. Provisioning (NVS)

The device requires a specific configuration at address `0x9000`. You can generate this binary via the **Smart Learn Web** dashboard or the API.

**Required Keys in NVS:**
- `ssid`/`password`: WiFi credentials.
- `ChatGPT_key`: OpenAI API key.
- `KB_url`: The endpoint of your specific Knowledge Base.
- `Base_url`: The base URL for OpenAI-compatible services.
- `tts_voice`: Selected voice (e.g., `alloy`, `nova`).
- `theme_type`: `dark` or `light`.

---

## 📁 Project Structure

```text
smart-learn-iot/
├── smart-learn/
│   ├── main/           # Application logic (STT, TTS, KB flow)
│   │   ├── app/        # WiFi, Audio, and Theme management
│   │   ├── ui/         # LVGL UI components and animations
│   │   └── main.c      # Entry point and OpenAI integration
│   ├── factory_nvs/    # Default NVS configuration templates
│   ├── spiffs/         # Static assets (icons, boot audio)
│   ├── squareline/     # SquareLine Studio UI project files
│   └── partitions.csv  # Flash memory partition table
├── components/         # Custom ESP-IDF components
└── tools/              # Utility scripts for flashing and bin generation
```

---

## 📄 License

This project is part of the Smart Learn Avatar ecosystem. See the root `README.md` for licensing information.
