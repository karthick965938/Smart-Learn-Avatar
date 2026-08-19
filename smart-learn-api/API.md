# Smart Learn API Reference

Base URL (local development):

```text
http://localhost:5000
```

All endpoints below are prefixed with `/api/v1` unless noted otherwise.

**Content type:** JSON requests use `Content-Type: application/json`.

**Interactive docs:** [Swagger UI](http://localhost:5000/docs) · [ReDoc](http://localhost:5000/redoc)

---

## Health Check

### `GET /`

Check that the API is running.

**Sample request**

```bash
curl http://localhost:5000/
```

**Sample response**

```json
{
  "message": "RAG SecuraAI API is running"
}
```

---

## Knowledge Bases

### `GET /api/v1/kbs`

List all knowledge bases.

**Sample request**

```bash
curl http://localhost:5000/api/v1/kbs
```

**Sample response**

```json
[
  {
    "id": "a1b2c3d4",
    "name": "Product Docs",
    "assistant_name": "Alex",
    "instruction": "",
    "custom_instruction": false,
    "conversation_types": ["Q&A"],
    "document_count": 3
  }
]
```

---

### `POST /api/v1/kbs`

Create a new knowledge base.

**Request body**

| Field | Type   | Required | Description              |
|-------|--------|----------|--------------------------|
| name  | string | yes      | Display name for the KB  |

**Sample request**

```bash
curl -X POST http://localhost:5000/api/v1/kbs \
  -H "Content-Type: application/json" \
  -d '{
    "name": "Product Docs"
  }'
```

**Sample response**

```json
{
  "id": "f8e2a1b0",
  "name": "Product Docs",
  "assistant_name": "",
  "instruction": "",
  "custom_instruction": false,
  "conversation_types": [],
  "document_count": 0
}
```

---

### `POST /api/v1/kb/{kb_id}`

Update knowledge base metadata (name, AI personality, instructions).

**Request body**

| Field               | Type     | Required | Description                          |
|---------------------|----------|----------|--------------------------------------|
| name                | string   | yes      | KB display name                      |
| assistant_name      | string   | no       | AI assistant name                    |
| instruction         | string   | no       | Custom system instruction            |
| custom_instruction  | boolean  | no       | Use custom instruction when true     |
| conversation_types  | string[] | no       | e.g. `["Q&A", "Follow-up Question"]` |

**Sample request**

```bash
curl -X POST http://localhost:5000/api/v1/kb/f8e2a1b0 \
  -H "Content-Type: application/json" \
  -d '{
    "name": "Product Docs",
    "assistant_name": "Alex",
    "instruction": "You are {assistant_name}, a helpful product expert for {kb_name}.",
    "custom_instruction": true,
    "conversation_types": ["Q&A", "Follow-up Question"]
  }'
```

**Sample response**

```json
{
  "message": "Metadata updated for KB f8e2a1b0."
}
```

---

### `DELETE /api/v1/kb/{kb_id}`

Delete a knowledge base and all its documents.

**Sample request**

```bash
curl -X DELETE http://localhost:5000/api/v1/kb/f8e2a1b0
```

**Sample response**

```json
{
  "message": "Knowledge base f8e2a1b0 deleted successfully."
}
```

---

## Documents

### `POST /api/v1/kb/{kb_id}/ingest`

Upload a document for background ingestion. Supported formats: PDF, DOCX, CSV, TXT.

**Request:** `multipart/form-data` with field `file`

**Sample request**

```bash
curl -X POST http://localhost:5000/api/v1/kb/f8e2a1b0/ingest \
  -F "file=@/path/to/document.pdf"
```

**Sample response**

```json
{
  "message": "File upload accepted for KB f8e2a1b0. Processing in background."
}
```

---

### `POST /api/v1/kb/{kb_id}/ingest/url`

Ingest content from a URL (background processing).

**Request body**

| Field | Type   | Required | Description        |
|-------|--------|----------|--------------------|
| url   | string | yes      | URL to scrape      |

**Sample request**

```bash
curl -X POST http://localhost:5000/api/v1/kb/f8e2a1b0/ingest/url \
  -H "Content-Type: application/json" \
  -d '{
    "url": "https://en.wikipedia.org/wiki/Artificial_intelligence"
  }'
```

**Sample response**

```json
{
  "message": "URL ingestion accepted for KB f8e2a1b0. Processing in background."
}
```

---

### `GET /api/v1/kb/{kb_id}/documents`

List all document sources in a knowledge base.

**Sample request**

```bash
curl http://localhost:5000/api/v1/kb/f8e2a1b0/documents
```

**Sample response**

```json
[
  "document.pdf",
  "https://en.wikipedia.org/wiki/Artificial_intelligence"
]
```

---

### `DELETE /api/v1/kb/{kb_id}/documents`

Delete a document by filename or URL source.

**Query parameters**

| Param    | Type   | Required | Description                          |
|----------|--------|----------|--------------------------------------|
| filename | string | yes      | Exact source name from documents list |

**Sample request**

```bash
curl -X DELETE "http://localhost:5000/api/v1/kb/f8e2a1b0/documents?filename=document.pdf"
```

**Sample response**

```json
{
  "message": "Document document.pdf deleted successfully from KB f8e2a1b0."
}
```

---

### `PUT /api/v1/kb/{kb_id}/documents`

Replace an existing document (delete old chunks, re-ingest new file).

**Query parameters**

| Param    | Type   | Required | Description              |
|----------|--------|----------|--------------------------|
| filename | string | yes      | Existing document to replace |

**Request:** `multipart/form-data` with field `file`

**Sample request**

```bash
curl -X PUT "http://localhost:5000/api/v1/kb/f8e2a1b0/documents?filename=document.pdf" \
  -F "file=@/path/to/updated-document.pdf"
```

**Sample response**

```json
{
  "message": "Document document.pdf update started in background for KB f8e2a1b0."
}
```

---

## Query (RAG Chat)

### `POST /api/v1/kb/{kb_id}/query`

Ask a question against a knowledge base. Returns an LLM answer grounded in retrieved document chunks. Conversation history is kept per KB for ~5 minutes.

**Request body**

| Field | Type   | Required | Description      |
|-------|--------|----------|------------------|
| query | string | yes      | User question      |

**Sample request**

```bash
curl -X POST http://localhost:5000/api/v1/kb/f8e2a1b0/query \
  -H "Content-Type: application/json" \
  -d '{
    "query": "What are the main features of the product?"
  }'
```

**Sample response**

```json
{
  "answer": "The product includes voice interaction, RFID-based KB switching, and document Q&A.",
  "context": [
    "Feature 1: Voice interaction via ESP32...",
    "Feature 2: RFID card maps to knowledge bases..."
  ],
  "latency": 1.42
}
```

> **IoT device usage:** Set the device `KB_url` in NVS to  
> `http://<api-host>:5000/api/v1/kb/{kb_id}/query`

---

## IoT — NVS Generation

### `POST /api/v1/iot/generate-nvs`

Generate a 16 KB NVS binary for ESP32 flashing at partition offset `0x9000`.

**Request body**

| Field       | Type   | Required | NVS key on device | Description                    |
|-------------|--------|----------|-------------------|--------------------------------|
| ssid        | string | yes      | ssid              | WiFi network name              |
| password    | string | yes      | password          | WiFi password                  |
| openai_key  | string | yes      | ChatGPT_key       | OpenAI API key                 |
| base_url    | string | yes      | Base_url          | OpenAI API base URL            |
| kb_url      | string | yes      | KB_url            | Knowledge base query endpoint  |
| tts_voice   | string | yes      | tts_voice         | e.g. `nova`, `alloy`           |
| theme       | string | yes      | theme_type        | `light` or `dark`              |

**Sample request**

```bash
curl -X POST http://localhost:5000/api/v1/iot/generate-nvs \
  -H "Content-Type: application/json" \
  -d '{
    "ssid": "MyWiFi",
    "password": "secret123",
    "openai_key": "sk-...",
    "base_url": "https://api.openai.com/v1/",
    "kb_url": "http://192.168.1.10:5000/api/v1/kb/f8e2a1b0/query",
    "tts_voice": "nova",
    "theme": "light"
  }' \
  --output nvs.bin
```

**Sample response**

Binary file (`application/octet-stream`), saved as `nvs.bin`.

---

## IoT — RFID Cards

RFID card mappings are stored in `data/rfid_cards.json`. The IoT device notifies the API on each scan; the web dashboard polls for events.

### `POST /api/v1/iot/rfid/scan`

Called by the IoT device when an RFID card is scanned. Registers unknown cards and returns assignment status.

**Request body**

| Field | Type   | Required | Description                              |
|-------|--------|----------|------------------------------------------|
| uid   | string | yes      | Card UID (4–16 hex characters, e.g. `A1B2C3D4` or `12344`) |

**Sample request**

```bash
curl -X POST http://localhost:5000/api/v1/iot/rfid/scan \
  -H "Content-Type: application/json" \
  -d '{
    "uid": "12344"
  }'
```

**Sample response (unassigned card)**

```json
{
  "id": 1,
  "uid": "12344",
  "assigned": false,
  "kb_id": null,
  "kb_name": null,
  "timestamp": 1787071529
}
```

**Sample response (assigned card)**

```json
{
  "id": 2,
  "uid": "12344",
  "assigned": true,
  "kb_id": "f8e2a1b0",
  "kb_name": "Product Docs",
  "kb_url": "http://192.168.1.10:8000/api/v1/kb/f8e2a1b0/query",
  "timestamp": 1787071600
}
```

> The IoT device uses `kb_url` from this response to switch the active Knowledge Base for speech queries.

---

### `GET /api/v1/iot/rfid/events`

Poll for RFID scan events (used by the web dashboard). Pass the last seen event `id` as `since` to fetch only new events.

**Query parameters**

| Param | Type | Required | Default | Description                    |
|-------|------|----------|---------|--------------------------------|
| since | int  | no       | 0       | Return events with id > since  |

**Sample request**

```bash
curl "http://localhost:5000/api/v1/iot/rfid/events?since=0"
```

**Sample response**

```json
{
  "events": [
    {
      "id": 1,
      "uid": "12344",
      "assigned": false,
      "kb_id": null,
      "kb_name": null,
      "timestamp": 1787071529
    }
  ]
}
```

---

### `GET /api/v1/iot/rfid/cards`

List all registered RFID cards and their Knowledge Base assignments.

**Sample request**

```bash
curl http://localhost:5000/api/v1/iot/rfid/cards
```

**Sample response**

```json
[
  {
    "uid": "12344",
    "kb_id": "f8e2a1b0",
    "kb_name": "Product Docs",
    "label": "",
    "created_at": 1787071529,
    "updated_at": 1787071653
  }
]
```

---

### `GET /api/v1/iot/rfid/cards/{uid}`

Get a single RFID card by UID.

**Sample request**

```bash
curl http://localhost:5000/api/v1/iot/rfid/cards/12344
```

**Sample response**

```json
{
  "uid": "12344",
  "kb_id": "f8e2a1b0",
  "kb_name": "Product Docs",
  "label": "",
  "created_at": 1787071529,
  "updated_at": 1787071653
}
```

**Error (404)**

```json
{
  "detail": "RFID card not found"
}
```

---

### `POST /api/v1/iot/rfid/cards`

Assign (or update) a Knowledge Base for an RFID card.

**Request body**

| Field  | Type   | Required | Description                          |
|--------|--------|----------|--------------------------------------|
| uid    | string | yes      | Card UID (4–16 hex characters)       |
| kb_id  | string | yes      | Knowledge base id                    |
| label  | string | no       | Optional friendly label            |

**Sample request**

```bash
curl -X POST http://localhost:5000/api/v1/iot/rfid/cards \
  -H "Content-Type: application/json" \
  -d '{
    "uid": "12344",
    "kb_id": "f8e2a1b0",
    "label": "Card 1"
  }'
```

**Sample response**

```json
{
  "uid": "12344",
  "kb_id": "f8e2a1b0",
  "kb_name": "Product Docs",
  "label": "Card 1",
  "created_at": 1787071529,
  "updated_at": 1787071700
}
```

**Error (400)**

```json
{
  "detail": "UID must be 4–16 hexadecimal characters"
}
```

---

### `DELETE /api/v1/iot/rfid/cards/{uid}`

Remove an RFID card registration.

**Sample request**

```bash
curl -X DELETE http://localhost:5000/api/v1/iot/rfid/cards/12344
```

**Sample response**

```json
{
  "message": "RFID card 12344 removed"
}
```

---

## Quick Reference

| Method   | Endpoint                              | Description                    |
|----------|---------------------------------------|--------------------------------|
| `GET`    | `/`                                   | Health check                   |
| `GET`    | `/api/v1/kbs`                         | List knowledge bases           |
| `POST`   | `/api/v1/kbs`                         | Create knowledge base          |
| `POST`   | `/api/v1/kb/{kb_id}`                  | Update KB metadata             |
| `DELETE` | `/api/v1/kb/{kb_id}`                  | Delete knowledge base          |
| `POST`   | `/api/v1/kb/{kb_id}/ingest`           | Upload document                |
| `POST`   | `/api/v1/kb/{kb_id}/ingest/url`       | Ingest URL                     |
| `GET`    | `/api/v1/kb/{kb_id}/documents`        | List documents                 |
| `DELETE` | `/api/v1/kb/{kb_id}/documents`        | Delete document                |
| `PUT`    | `/api/v1/kb/{kb_id}/documents`        | Replace document               |
| `POST`   | `/api/v1/kb/{kb_id}/query`            | Query knowledge base           |
| `POST`   | `/api/v1/iot/generate-nvs`            | Generate ESP32 NVS binary      |
| `POST`   | `/api/v1/iot/rfid/scan`               | IoT device RFID scan notify    |
| `GET`    | `/api/v1/iot/rfid/events`             | Poll RFID scan events          |
| `GET`    | `/api/v1/iot/rfid/cards`              | List RFID cards                |
| `GET`    | `/api/v1/iot/rfid/cards/{uid}`        | Get RFID card                  |
| `POST`   | `/api/v1/iot/rfid/cards`              | Assign KB to RFID card         |
| `DELETE` | `/api/v1/iot/rfid/cards/{uid}`        | Delete RFID card               |

---

## Environment Variables

| Variable         | Default                  | Description              |
|------------------|--------------------------|--------------------------|
| OPENAI_API_KEY   | —                        | Required for embeddings/LLM |
| CHROMA_DB_PATH   | `./chroma_db`            | Vector DB storage path   |
| EMBEDDING_MODEL  | `text-embedding-3-small` | OpenAI embedding model   |
| LLM_MODEL        | `gpt-4o-mini`            | OpenAI chat model        |
| KB_URL           | —                        | Optional external KB proxy URL |
| API_BASE_URL     | `http://127.0.0.1:5000`  | Public API host for RFID `kb_url` in scan responses |
