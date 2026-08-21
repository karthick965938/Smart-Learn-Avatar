# 🌐 Smart Learn Web

![React](https://img.shields.io/badge/react-%2320232a.svg?style=for-the-badge&logo=react&logoColor=%2361DAFB)
![Vite](https://img.shields.io/badge/vite-%23646CFF.svg?style=for-the-badge&logo=vite&logoColor=white)
![TailwindCSS](https://img.shields.io/badge/tailwindcss-%2338B2AC.svg?style=for-the-badge&logo=tailwind-css&logoColor=white)

**Smart Learn Web** is the dashboard for the Smart Learn ecosystem. Create and manage knowledge bases, upload documents (including images), configure AI personalities, map RFID cards to knowledge bases, and preview answers in chat—all from the browser.

[![Watch Demo](https://img.shields.io/badge/Demo-Watch%20Video-red?style=for-the-badge&logo=youtube)](https://www.youtube.com/watch?v=sbAEzvDquOA)

---

## 🚀 Key Features

- **🗂️ Knowledge Base Management** — Create, select, and delete knowledge bases from a card-based dashboard.
- **📄 Document Control** — Upload `PDF`, `DOCX`, `CSV`, `TXT`, and images (`PNG`, `JPG`, `WEBP`, `GIF`, up to 10MB), or ingest content from URLs. Images are processed on the backend with vision extraction, then embedded for search.
- **🤖 AI Setup** — Per–knowledge base assistant name, custom system instructions, and conversation modes (Q&A, Follow-up Question, Revision Mode).
- **📡 RFID Card Configuration** — Register RFID cards scanned on IoT devices, assign each card to a knowledge base, and manage assignments from **IoT Setup**.
- **🔔 Live RFID Scan Popups** — When a device scans a card, the dashboard polls for events and shows a popup to assign an unregistered card or open chat for an assigned one.
- **💬 Chat Preview** — Floating chat widget to test any knowledge base; opens automatically when an assigned RFID card is scanned.
- **🎨 Dark Mode UI** — High-contrast dark interface built with Tailwind CSS.

---

## 🛠️ Tech Stack

- **Framework**: [React 19](https://react.dev/)
- **Build Tool**: [Vite](https://vitejs.dev/)
- **Styling**: [Tailwind CSS](https://tailwindcss.com/)
- **API Client**: [Axios](https://axios-http.com/)
- **Icons**: [Heroicons](https://heroicons.com/)

---

## ⚙️ Getting Started

### Prerequisites

- **Node.js** v18 or higher
- **Smart Learn API** running (see `smart-learn-api/README.md`)

### 1. Installation

```bash
cd smart-learn-web
npm install
```

### 2. Environment Configuration

```bash
cp .env.example .env
```

Set your backend URL:

```env
VITE_API_BASE_URL=http://localhost:5000
```

### 3. Run Development Server

```bash
npm run dev
```

The app runs at `http://localhost:5173`.

---

## 🕹️ Usage

### Dashboard (Home)

The main screen shows all knowledge bases as cards.

| Action | How |
|--------|-----|
| Create a KB | Click **New Knowledge Base**, enter a name, and confirm |
| Select active KB | Click a card (green ring = selected) |
| Open documents | Hover a card → click the **document** icon |
| Delete a KB | Hover a card → click the **trash** icon |

The header also provides **AI Setup**, **IoT Setup**, and **New Knowledge Base**.

---

### Documents (per Knowledge Base)

Open from a KB card’s document icon.

| Action | Details |
|--------|---------|
| **Upload File** | PDF, TXT, DOCX, CSV, PNG, JPG, WEBP, GIF (max 10MB). Processing runs in the background after upload. |
| **Add URL** | Scrape and ingest a web page into the knowledge base |
| **View / Delete** | Listed sources show file names or URLs; hover to delete |

Supported image flow: the API extracts text and visual descriptions from images, then stores embeddings like any other document.

---

### AI Setup

Configure how the assistant behaves **per knowledge base**.

1. Click **AI Setup** in the header.
2. Select a knowledge base.
3. Set **Assistant Name** (e.g. “Science Tutor”).
4. Optionally enable **Custom System Instructions** and write a personality prompt.
5. Choose one or more **Conversation Types**: Q&A, Follow-up Question, Revision Mode.
6. Click **Save Configuration** — settings are stored on the API and in local browser storage.

---

### IoT Setup (RFID Cards)

IoT Setup manages **RFID card registry and knowledge base mapping** for the **Arduino UNO Q main board** (RC522 + OLED). The **ESP32-S3 Mini** provides the **Hi Json** voice assistant with built-in microphone and speaker output.

1. Run **Smart Learn API** and open this web app.
2. Click **IoT Setup** in the header.
3. Tap an RFID card on the **UNO Q** — the card appears here or in the scan popup.
4. For unassigned cards, click **Assign KB**, pick a knowledge base, and save.
5. Assigned cards show the linked KB name; use **Remove** to delete a card entry.

The panel shows **RC522 wiring reference** for the UNO Q.

**Hardware highlights:**

| Board | Project | Highlights |
|-------|---------|------------|
| **Arduino UNO Q** (main) | `smart-learn-uno-q` | RC522 RFID, OLED display, web integration |
| **ESP32-S3 Mini** (voice) | `smart-learn-iot/smart-learn-board` | **Hi Json**, STT/TTS, built-in mic & amplifier |

---

### RFID Scan Popup

The dashboard polls RFID events every 2 seconds.

| Scan result | What happens |
|-------------|----------------|
| **Unassigned card** | Popup shows card UID → select a knowledge base → **Assign Knowledge Base** |
| **Assigned card** | Chat opens automatically with that knowledge base selected |

You can also assign cards manually from **IoT Setup** after a scan registers the UID.

---

### Chat Preview

- Click the green **chat bubble** (bottom-right) to test the currently selected knowledge base.
- When an **assigned RFID card** is scanned, chat opens with that KB pre-loaded.
- Minimize, close, or type questions to query ingested documents.

---

## 🔄 End-to-End Flow

```text
1. Create Knowledge Base
2. Upload documents / URLs (including images)
3. Configure AI Setup for that KB
4. Scan RFID card on **Arduino UNO Q** → card registers in API
5. Assign card to KB (popup or IoT Setup)
6. Scan again → UNO Q OLED shows KB / web chat opens; say **"Hi Json"** on **ESP32** for voice (NVS `KB_url`)
```

---

## 📁 Project Structure

```text
smart-learn-web/
├── src/
│   ├── components/
│   │   ├── KnowledgeBaseCard.jsx   # KB grid cards
│   │   ├── DocumentModal.jsx       # Upload, URL ingest, doc list
│   │   ├── AiSetup.jsx             # Assistant personality config
│   │   ├── IoTSetup.jsx            # RFID card registry & KB assignment
│   │   ├── RfidScanPopup.jsx       # Live scan assign / chat trigger
│   │   ├── ChatPopup.jsx           # KB query preview
│   │   └── ...
│   ├── api.js                      # Axios API wrappers
│   ├── App.jsx                     # Layout, polling, global state
│   └── main.jsx
├── .env.example
├── tailwind.config.js
└── vite.config.js
```

---

## 📄 License

This project is part of the Smart Learn ecosystem. See the root `README.md` for licensing information.
