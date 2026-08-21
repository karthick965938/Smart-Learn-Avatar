# 🎓 Smart Learn

**Smart Learn** is a full-stack AI learning platform. **Arduino UNO Q** is the main board with **RC522** RFID and **OLED** display, paired with **Smart Learn Web** for knowledge base selection. **ESP32-S3 Mini** powers hands-free voice with wake word **"Hi Json"**, built-in microphone, amplifier, and speaker output.

---

## ✨ Highlights

| | |
|---|---|
| **📚 Multi-KB RAG** | Isolated knowledge bases with ChromaDB + GPT-4o-mini, grounded answers from your own content |
| **🖼️ Image ingestion** | Upload PNG, JPG, WEBP, GIF — vision extraction then embedding with low-cost models |
| **📡 RFID hub** | Arduino UNO Q + RC522 + OLED — tap a card, assign KB in **Smart Learn Web** |
| **🎙️ Voice assistant** | ESP32-S3 Mini — **"Hi Json"** wake word, STT, KB query, TTS on built-in audio |
| **🌐 Web dashboard** | Manage KBs, documents, AI setup, RFID assignments, and chat preview |
| **🔄 Live RFID updates** | Card → KB mappings in the API; change anytime in **IoT Setup** |

---

## 🎥 Demo

[![Smart Learn Demo](https://img.youtube.com/vi/sbAEzvDquOA/0.jpg)](https://www.youtube.com/watch?v=sbAEzvDquOA)

*[Watch on YouTube](https://www.youtube.com/watch?v=sbAEzvDquOA)* — knowledge base setup, voice Q&A, and IoT interaction.

---

## 🏗️ Overview

Smart Learn has four main parts that work together:

```text
                    ┌─────────────────────────────────┐
                    │      Smart Learn Web            │
                    │  KBs · Docs · AI Setup · RFID   │
                    └───────────────┬─────────────────┘
                                    │ REST
                                    ▼
                    ┌─────────────────────────────────┐
                    │      Smart Learn API            │
                    └───────────────┬─────────────────┘
                          ▲                    ▲
              RFID scan   │                    │  voice KB query
                          │                    │
          ┌───────────────┴───┐      ┌─────────┴──────────────┐
          │  Arduino UNO Q    │      │   ESP32-S3 Mini        │
          │  MAIN · RC522+OLED│      │   VOICE · mic+speaker  │
          │  (smart-learn-uno-q)     │   (smart-learn-iot)    │
          └─────────────────────┘      └────────────────────────┘
```

| Component | Role | Tech |
|-----------|------|------|
| [**Smart Learn API**](./smart-learn-api/README.md) | Backend: knowledge bases, RAG, document/image ingestion, RFID registry | FastAPI, ChromaDB, OpenAI |
| [**Smart Learn Web**](./smart-learn-web/README.md) | Dashboard: upload content, AI personality, RFID card assignment, chat preview | React, Vite, Tailwind |
| [**Smart Learn UNO Q**](./smart-learn-uno-q/README.md) | **Main board**: RC522 RFID, OLED display, API + web integration | Sketch + Python |
| [**Smart Learn IoT**](./smart-learn-iot/README.md) | **Voice module**: **Hi Json**, built-in mic & amplifier, WiFi, RAG queries | ESP-IDF |

---

## 🧩 What Each Part Does

### 🧠 [Smart Learn API](./smart-learn-api/README.md)

The backend brain of the platform.

- Multiple **knowledge bases** with metadata (assistant name, custom instructions, conversation modes)
- **Document ingestion**: PDF, DOCX, CSV, TXT, URLs, and **images** (vision → text → embeddings)
- **RAG querying** with conversation history and concise answers for small screens
- **RFID API**: scan registration, card → KB assignment, event polling for the web dashboard
- Full reference: [`smart-learn-api/API.md`](./smart-learn-api/API.md)

### 🌐 [Smart Learn Web](./smart-learn-web/README.md)

Browser control center for knowledge bases, documents, AI setup, and RFID management.

- Create and manage knowledge bases
- Upload files and URLs (including images)
- **AI Setup** per KB: name, system prompt, Q&A / follow-up / revision modes
- **IoT Setup**: RFID card registry and KB assignment
- **Live scan popups** when a device reads a card; chat opens for assigned cards

### 📟 [Smart Learn UNO Q](./smart-learn-uno-q/README.md) — main board

**Arduino UNO Q** with **RC522** + **OLED** + **Smart Learn Web**.

- Tap card → API → **IoT Setup** / scan popup → assign KB
- OLED: **Smart Learn 2.0**, scan status, **Knowledge Base Selected**

### 🤖 [Smart Learn IoT](./smart-learn-iot/README.md) — voice module

**ESP32-S3 Mini** — built-in microphone, audio amplifier, and external speaker.

- **Hi Json** wake word → STT → KB query (`KB_url` in NVS) → TTS
- WiFi-connected voice answers grounded in your knowledge bases

---

## 🛠️ Typical Workflow

1. **Start the API** — set `OPENAI_API_KEY` and `API_BASE_URL` (LAN IP for IoT devices).
2. **Start the web app** — point `VITE_API_BASE_URL` at the API.
3. **Create a knowledge base** — upload documents, URLs, or images.
4. **Configure AI Setup** — assistant name, instructions, conversation type.
5. **Deploy Arduino UNO Q** — wire **RC522** + **OLED**; flash [`smart-learn-uno-q`](./smart-learn-uno-q/).
6. **Flash ESP32-S3 Mini** — voice firmware + NVS; connect external speaker for **Hi Json** voice Q&A.
7. **Tap a card on the UNO Q** — assign KB in **Smart Learn Web**; OLED shows selected KB; chat opens for assigned cards.
8. **Say "Hi Json" on the ESP32** — voice queries use `KB_url` from NVS (align with the KB you assigned on the UNO Q).

---

## 🚀 Quick Start

```bash
# 1. API
cd smart-learn-api
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
# Create .env with OPENAI_API_KEY and API_BASE_URL
uvicorn app.main:app --reload --host 0.0.0.0 --port 5000

# 2. Web
cd smart-learn-web
npm install
cp .env.example .env   # set VITE_API_BASE_URL
npm run dev

# 3a. Main board — Arduino UNO Q (RC522 + OLED)
# See smart-learn-uno-q/README.md

# 3b. Voice module — ESP32-S3 Mini (ESP-IDF)
cd smart-learn-iot/smart-learn-board
idf.py set-target esp32s3
idf.py build flash monitor
```

See each module’s README for full setup, NVS provisioning, and wiring.

---

## 📖 Documentation

| Module | README | Extra |
|--------|--------|-------|
| API | [smart-learn-api/README.md](./smart-learn-api/README.md) | [API.md](./smart-learn-api/API.md) |
| Web | [smart-learn-web/README.md](./smart-learn-web/README.md) | — |
| IoT | [smart-learn-iot/README.md](./smart-learn-iot/README.md) | [SETUP.md](./smart-learn-iot/smart-learn-board/SETUP.md), [WIRING.md](./smart-learn-iot/smart-learn-board/WIRING.md) |
| UNO Q | [smart-learn-uno-q/README.md](./smart-learn-uno-q/README.md) | — |

---

## 📄 License

This project is licensed under the MIT License. See individual modules for specific library licenses (e.g., ESP-IDF, OpenAI).
