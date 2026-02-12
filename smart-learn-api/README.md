# 🧠 Smart Learn API

[![FastAPI](https://img.shields.io/badge/FastAPI-005571?style=for-the-badge&logo=fastapi)](https://fastapi.tiangolo.com/)
[![Python](https://img.shields.io/badge/python-3670A0?style=for-the-badge&logo=python&logoColor=ffdd54)](https://www.python.org/)
[![Docker](https://img.shields.io/badge/docker-%230db7ed.svg?style=for-the-badge&logo=docker&logoColor=white)](https://www.docker.com/)
[![OpenAI](https://img.shields.io/badge/OpenAI-412991?style=for-the-badge&logo=openai&logoColor=white)](https://openai.com/)

**Smart Learn API** is a high-performance backend built with **FastAPI** that powers the Smart Learn Avatar ecosystem. It provides advanced RAG (Retrieval-Augmented Generation) capabilities, multi-tenant knowledge base management, and specialized tools for IoT device provisioning.

> [!TIP]
> **See it in action:** Check out the [Smart Learn Avatar Demo Video](https://www.youtube.com/watch?v=sbAEzvDquOA) to see how the API powers the interactive tutor.

---


## 🚀 Key Features

- **📂 Dynamic Knowledge Bases**: Create and manage isolated knowledge bases for different topics or users.
- **🔍 Advanced RAG**: Intelligent document retrieval using **ChromaDB** and **OpenAI Embeddings**.
- **📄 Multi-format Ingestion**: Support for `PDF`, `DOCX`, `CSV`, `TXT`, and direct `URL` scraping.
- **💬 Chat with Context**: Context-aware querying with conversation history tracking.
- **🔧 IoT Flash Support**: Built-in endpoint to generate **NVS (Non-Volatile Storage)** binaries for ESP32 devices, enabling seamless configuration of WiFi and API keys.
- **⚡ Background Processing**: Asynchronous document processing to ensure a responsive API experience.

---

## 🛠️ Tech Stack

- **Framework**: [FastAPI](https://fastapi.tiangolo.com/)
- **Vector Database**: [ChromaDB](https://www.trychroma.com/)
- **LLM & Embeddings**: OpenAI (GPT-4o-mini & Text-Embedding-3-Small)
- **Package Manager**: [uv](https://github.com/astral-sh/uv) (recommended) or pip
- **Deployment**: Docker & Docker Compose

---

## ⚙️ Getting Started

### Prerequisites

- **Python 3.9+**
- **OpenAI API Key** (Required for embeddings and LLM responses)
- **Docker** (Optional, for containerized setup)

### 1. Project Setup

Clone the repository and navigate to the API directory:

```bash
cd smart-learn-api
```

### 2. Environment Configuration

Create a `.env` file in the root directory:

```env
OPENAI_API_KEY=sk-your-api-key-here
CHROMA_DB_PATH=./chroma_db
EMBEDDING_MODEL=text-embedding-3-small
LLM_MODEL=gpt-4o-mini
```

### 3. Installation & Run

#### **Option A: Using `uv` (Recommended)**
```bash
# Install dependencies and run
uvicorn app.main:app --reload --port 5000
```

#### **Option B: Using `pip`**
```bash
pip install -e .
uvicorn app.main:app --reload --port 5000
```

#### **Option C: Using Docker**
```bash
docker compose up -d --build
```

---

## 📖 API Documentation

Once the server is running, you can access the interactive documentation at:

- **Swagger UI**: [http://localhost:5000/docs](http://localhost:5000/docs)
- **ReDoc**: [http://localhost:5000/redoc](http://localhost:5000/redoc)

### Core Endpoints

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/api/v1/kbs` | List all available knowledge bases |
| `POST` | `/api/v1/kbs` | Create a new knowledge base |
| `POST` | `/api/v1/kb/{kb_id}/ingest` | Upload a file (.pdf, .docx, .csv, .txt) |
| `POST` | `/api/v1/kb/{kb_id}/ingest-url` | Scrape and ingest content from a URL |
| `POST` | `/api/v1/kb/{kb_id}/query` | Ask questions based on the knowledge base |
| `POST` | `/api/v1/iot/generate-nvs` | Generate NVS binary for ESP32 |

---

## 📡 IoT Integration

The `/api/v1/iot/generate-nvs` endpoint is specifically designed for the **Smart Learn Avatar** hardware. It generates a binary file that can be flashed to an ESP32 at address `0x9000` (or as configured in your partition table).

**Keys stored in NVS:**
- `ssid` / `password`: WiFi credentials.
- `ChatGPT_key`: OpenAI API key for the device.
- `Base_url`: API endpoint for LLM/TTS.
- `KB_url`: Specific knowledge base URL.
- `tts_voice`: Selected voice profile.
- `theme_type`: UI theme selection.

---

## 📁 Project Structure

```text
smart-learn-api/
├── app/
│   ├── api/            # Route definitions
│   ├── core/           # LLM, Embedding, and Ingestion logic
│   ├── utils/          # Helper functions
│   ├── config.py       # Pydantic settings management
│   └── main.py         # App entry point
├── chroma_db/          # Persistent vector storage
├── tests/              # Test suite
├── Dockerfile          # Container definition
├── pyproject.toml      # Dependency management
└── .env                # Environment variables
```

---

## 📄 License

This project is part of the Smart Learn Avatar ecosystem. See the root `README.md` for licensing information.
