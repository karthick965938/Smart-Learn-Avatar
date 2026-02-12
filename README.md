# 🎓 Smart Learn Avatar

**Smart Learn Avatar** is a full-stack, AI-powered educational ecosystem that brings interactive 3D avatars to life on hardware. It combines advanced Retrieval-Augmented Generation (RAG) with edge computing to create a portable, voice-interactive AI tutor.

---

## 🎥 Demo Video

[![Smart Learn Avatar Demo](https://img.youtube.com/vi/sbAEzvDquOA/0.jpg)](https://www.youtube.com/watch?v=sbAEzvDquOA)

*Watch the full demonstration on YouTube: [AI Tutor Running on ESP32?! 🤯](https://www.youtube.com/watch?v=sbAEzvDquOA)*

### 🚀 What's in the Video?
- **Introduction**: Overview of the AI Tutor running on the ESP32-S3-BOX-3.
- **Knowledge Base Creation**: Uploading documents and URLs to the web dashboard.
- **IoT Setup**: Configuring WiFi and OpenAI keys directly from the browser.
- **LVGL Avatar Animation**: Stunning real-time animations and lip-sync on the hardware.
- **Real-time Interaction**: Full voice-to-voice conversation demo with the interactive tutor.

---



## 🏗️ System Architecture

The project is divided into three core components:

### 1. [🧠 Smart Learn API](./smart-learn-api/README.md)
The brain of the system. A **FastAPI** backend that manages:
- **Vector Database**: Multi-tenant knowledge bases using ChromaDB.
- **RAG Pipeline**: Document ingestion (PDF, DOCX, URL) and intelligent context retrieval.
- **LLM Integration**: GPT-4o-powered question answering.

### 2. [🌐 Smart Learn Web](./smart-learn-web/README.md)
The control center. A **React + Vite** dashboard for:
- **Curating Knowledge**: Managing documents and URLs for different subjects.
- **AI Customization**: Setting personalities and voice profiles for each avatar.
- **Hardware Flashing**: A browser-based tool (`esptool-js`) to provision ESP32 devices without installing local drivers.

### 3. [🤖 Smart Learn IoT](./smart-learn-iot/README.md)
The interactive interface. **ESP-IDF** firmware for the **ESP32-S3-BOX-3** featuring:
- **Real-time Voice Chat**: Far-field mic support and low-latency audio playback.
- **Dynamic Visuals**: Animated avatars with lip-sync and live subtitle overlays.
- **Edge Integration**: Securely connects to the API via WiFi to provide answers from the custom knowledge base.

---

## 🛠️ Typical Workflow

1.  **Ingest Content**: Use the Web Dashboard to upload your learning materials (textbooks, research papers, or documentation).
2.  **Configure Personality**: Define your tutor's name, tone, and specific instructions.
3.  **Flash Hardware**: Connect your ESP32-S3-BOX-3 via USB and hit "Initiate Flash" in the web dashboard.
4.  **Learn Interactively**: Place the device on your desk, and start asking questions. The avatar will respond using the context from your uploaded documents.

---

## 📖 Component Documentation

For detailed setup instructions for each part, please refer to their respective README files:
- [API Setup & Features](./smart-learn-api/README.md)
- [Web Dashboard Setup](./smart-learn-web/README.md)
- [IoT Firmware & Build Guide](./smart-learn-iot/README.md)

---

## 📄 License

This project is licensed under the MIT License. See individual modules for specific library licenses (e.g., ESP-IDF, OpenAI).
