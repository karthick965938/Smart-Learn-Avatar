# 🧠 Smart Learn API

[![FastAPI](https://img.shields.io/badge/FastAPI-005571?style=for-the-badge&logo=fastapi)](https://fastapi.tiangolo.com/)
[![Python](https://img.shields.io/badge/python-3670A0?style=for-the-badge&logo=python&logoColor=ffdd54)](https://www.python.org/)
[![Docker](https://img.shields.io/badge/docker-%230db7ed.svg?style=for-the-badge&logo=docker&logoColor=white)](https://www.docker.com/)
[![OpenAI](https://img.shields.io/badge/OpenAI-412991?style=for-the-badge&logo=openai&logoColor=white)](https://openai.com/)

**Smart Learn API** is the backend for the Smart Learn platform. It provides RAG (Retrieval-Augmented Generation) over multiple knowledge bases, document ingestion (including images), AI personality configuration, and RFID card management for IoT devices.

> [!TIP]
> **See it in action:** [Smart Learn Demo Video](https://www.youtube.com/watch?v=sbAEzvDquOA)

---

## 🚀 Key Features

- **📂 Knowledge Bases** — Create, update metadata, list, and delete isolated knowledge bases.
- **🔍 RAG Querying** — ChromaDB retrieval with OpenAI embeddings and GPT-4o-mini responses, plus per-KB conversation history.
- **📄 Multi-format Ingestion** — `PDF`, `DOCX`, `CSV`, `TXT`, and images (`PNG`, `JPG`, `WEBP`, `GIF`, max 10MB), plus URL scraping.
- **🖼️ Image Support** — Images are processed with GPT-4o-mini vision (`detail: low`) to extract text and visual descriptions, then embedded with `text-embedding-3-small`.
- **🤖 AI Personality** — Per-KB assistant name, custom system instructions, and conversation modes (Q&A, Follow-up Question, Revision Mode).
- **📡 RFID Integration** — IoT devices report card scans; the web dashboard assigns cards to knowledge bases and polls live scan events.
- **⚡ Background Processing** — File and URL ingestion runs asynchronously so uploads return immediately.
- **🔧 ESP32 NVS Generation** — Optional endpoint to generate NVS binaries for legacy device provisioning.

---

## 🛠️ Tech Stack

- **Framework**: [FastAPI](https://fastapi.tiangolo.com/)
- **Vector Database**: [ChromaDB](https://www.trychroma.com/)
- **LLM & Embeddings**: OpenAI (`gpt-4o-mini`, `text-embedding-3-small`)
- **Package Manager**: [uv](https://github.com/astral-sh/uv) or pip
- **Deployment**: Docker & Docker Compose

---

## ⚙️ Getting Started

### Prerequisites

- **Python 3.9+**
- **OpenAI API Key** (embeddings, LLM, and image vision extraction)

### 1. Project Setup

```bash
cd smart-learn-api
```

### 2. Environment Configuration

Create a `.env` file in the project root:

```env
OPENAI_API_KEY=sk-your-api-key-here
CHROMA_DB_PATH=./chroma_db
EMBEDDING_MODEL=text-embedding-3-small
LLM_MODEL=gpt-4o-mini
API_BASE_URL=http://localhost:5000
```

| Variable | Description |
|----------|-------------|
| `OPENAI_API_KEY` | Required. Used for embeddings, chat, and image text extraction. |
| `CHROMA_DB_PATH` | Path to persistent ChromaDB storage. |
| `EMBEDDING_MODEL` | OpenAI embedding model (default: `text-embedding-3-small`). |
| `LLM_MODEL` | OpenAI chat model (default: `gpt-4o-mini`). |
| `API_BASE_URL` | Public base URL of this API. Used to build `kb_url` in RFID scan responses for IoT devices. |
| `KB_URL` | Optional. If set, `/query` proxies to an external knowledge base instead of local RAG. |

### 3. Installation & Run

#### Option A: pip

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
uvicorn app.main:app --reload --host 0.0.0.0 --port 5000
```

#### Option B: uv

```bash
uv sync
uvicorn app.main:app --reload --host 0.0.0.0 --port 5000
```

#### Option C: Docker

```bash
docker compose up -d --build
```

---

## 📖 API Documentation

Interactive docs (server must be running):

- **Swagger UI**: [http://localhost:5000/docs](http://localhost:5000/docs)
- **ReDoc**: [http://localhost:5000/redoc](http://localhost:5000/redoc)

Full reference with curl examples: [`API.md`](./API.md)

### Core Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/api/v1/kbs` | List all knowledge bases |
| `POST` | `/api/v1/kbs` | Create a knowledge base |
| `POST` | `/api/v1/kb/{kb_id}` | Update KB metadata (name, assistant, instructions) |
| `DELETE` | `/api/v1/kb/{kb_id}` | Delete a knowledge base |
| `POST` | `/api/v1/kb/{kb_id}/ingest` | Upload a file (PDF, DOCX, CSV, TXT, PNG, JPG, WEBP, GIF) |
| `POST` | `/api/v1/kb/{kb_id}/ingest/url` | Ingest content from a URL |
| `GET` | `/api/v1/kb/{kb_id}/documents` | List documents in a KB |
| `DELETE` | `/api/v1/kb/{kb_id}/documents` | Delete a document |
| `PUT` | `/api/v1/kb/{kb_id}/documents` | Replace a document |
| `POST` | `/api/v1/kb/{kb_id}/query` | Query the knowledge base |

### RFID Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| `POST` | `/api/v1/iot/rfid/scan` | IoT device reports a card scan; returns assignment status |
| `GET` | `/api/v1/iot/rfid/events` | Poll scan events (web dashboard popups) |
| `GET` | `/api/v1/iot/rfid/cards` | List registered RFID cards |
| `POST` | `/api/v1/iot/rfid/cards` | Assign a knowledge base to a card |
| `GET` | `/api/v1/iot/rfid/cards/{uid}` | Get a single card |
| `DELETE` | `/api/v1/iot/rfid/cards/{uid}` | Remove a card |

### IoT (Legacy)

| Method | Endpoint | Description |
|--------|----------|-------------|
| `POST` | `/api/v1/iot/generate-nvs` | Generate NVS binary for ESP32 provisioning |

---

## 📡 RFID Flow

1. **Arduino UNO Q** (RC522) scans an RFID card and calls `POST /api/v1/iot/rfid/scan` with `{ "uid": "0A308005" }`.
2. The API registers the card if new, checks KB assignment, and returns:

```json
{
  "id": 1,
  "uid": "0A308005",
  "assigned": true,
  "kb_id": "f8e2a1b0",
  "kb_name": "Science Notes",
  "kb_url": "http://localhost:5000/api/v1/kb/f8e2a1b0/query",
  "timestamp": 1710000000
}
```

3. The web dashboard polls `GET /api/v1/iot/rfid/events?since=0` and shows a popup to assign unregistered cards.
4. Card mappings are stored in `data/rfid_cards.json`.

Set `API_BASE_URL` in `.env` to the address your IoT devices can reach (e.g. your LAN IP), not `localhost`.

---

## 📄 Document Ingestion

| Type | Formats | Notes |
|------|---------|-------|
| Documents | PDF, DOCX, CSV, TXT | Text extracted and chunked |
| Images | PNG, JPG, JPEG, WEBP, GIF | Vision extraction → text chunks → embeddings |
| URLs | HTTP/HTTPS links | HTML scraped via BeautifulSoup |

Ingestion is backgrounded: the API responds immediately with `"Processing in background."`

---

## 📁 Project Structure

```text
smart-learn-api/
├── app/
│   ├── api/
│   │   └── routes.py       # All HTTP endpoints
│   ├── core/
│   │   ├── database.py     # ChromaDB + KB metadata
│   │   ├── embedding.py    # OpenAI embeddings
│   │   ├── ingestion.py    # PDF, DOCX, CSV, TXT, image, URL extraction
│   │   ├── llm.py          # GPT response generation
│   │   └── rfid.py         # RFID card store and scan events
│   ├── utils/
│   │   └── nvs_gen.py      # ESP32 NVS binary generator
│   ├── config.py           # Settings from .env
│   └── main.py             # FastAPI app entry point
├── data/
│   └── rfid_cards.json     # RFID card → KB mappings
├── chroma_db/              # Persistent vector storage
├── tests/
├── API.md                  # Full API reference with curl samples
├── requirements.txt
├── Dockerfile
└── docker-compose.yml
```

---

## 📄 License

This project is part of the Smart Learn ecosystem. See the root `README.md` for licensing information.
