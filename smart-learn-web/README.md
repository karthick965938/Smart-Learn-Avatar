# 🌐 Smart Learn Web

![React](https://img.shields.io/badge/react-%2320232a.svg?style=for-the-badge&logo=react&logoColor=%2361DAFB)
![Vite](https://img.shields.io/badge/vite-%23646CFF.svg?style=for-the-badge&logo=vite&logoColor=white)
![TailwindCSS](https://img.shields.io/badge/tailwindcss-%2338B2AC.svg?style=for-the-badge&logo=tailwind-css&logoColor=white)

**Smart Learn Web** is the interactive dashboard for the Smart Learn Avatar ecosystem. It provides a sleek, user-friendly interface to manage knowledge bases, configure AI personalities, and flash firmware directly to IoT devices (ESP32-S3-BOX-3) from the browser.

[![Watch Demo](https://img.shields.io/badge/Demo-Watch%20Video-red?style=for-the-badge&logo=youtube)](https://www.youtube.com/watch?v=sbAEzvDquOA)

---


## 🚀 Key Features

- **🗂️ Knowledge Base Management**: Create, delete, and organize multiple knowledge bases with ease.
- **📄 Document Control**: Seamlessly upload files (`PDF`, `DOCX`, `CSV`, `TXT`) or ingest content from `URLs`.
- **🤖 AI Personality Setup**: Customize assistant names and system instructions for each knowledge base to tailor the AI's behavior.
- **🔌 Web-based IoT Flashing**: Integrated **ESPTool-js** for flashing firmware and configuring device WiFi/API credentials via NVS (Non-Volatile Storage) directly from the browser.
- **💬 Instant Chat Preview**: Test your knowledge bases immediately with a built-in chat interface before deploying to hardware.
- **🎨 Dark Mode UI**: A premium, high-contrast dark interface designed for clarity and modern aesthetics.

---

## 🛠️ Tech Stack

- **Framework**: [React 19](https://react.dev/)
- **Build Tool**: [Vite](https://vitejs.dev/)
- **Styling**: [Tailwind CSS](https://tailwindcss.com/)
- **API Client**: [Axios](https://axios-http.com/)
- **Hardware Interaction**: [ESPTool-js](https://github.com/espressif/esptool-js)
- **Icons**: [Heroicons](https://heroicons.com/)

---

## ⚙️ Getting Started

### Prerequisites

- **Node.js**: v18.0 or higher
- **Modern Browser**: Chrome, Edge, or any browser supporting the [Web Serial API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API) (required for IoT flashing).

### 1. Installation

Clone the repository and install dependencies:

```bash
cd smart-learn-web
npm install
```

### 2. Environment Configuration

Create a `.env` file based on the example:

```bash
cp .env.example .env
```

Edit `.env` and set your API base URL:

```env
VITE_API_BASE_URL=http://localhost:5000
```

### 3. Run Development Server

```bash
npm run dev
```

The app will be available at `http://localhost:5173`.

---

## 🕹️ Usage

### Managing Knowledge Bases
1. Click **"New Knowledge Base"** to create a container.
2. Select a card and click the **"Document"** icon to upload files or add URLs.
3. Use the **"AI Setup"** button to define the personality for your KB.

### Flashing IoT Devices
1. Connect your **ESP32-S3-BOX-3** via USB.
2. Click **"IoT Setup"** in the dashboard.
3. Fill in the WiFi credentials and OpenAI API Key.
4. Select the Knowledge Base you want to associate with the device.
5. Click **"Initiate Flash"** and follow the browser's serial connection prompt.

---

## 📁 Project Structure

```text
smart-learn-web/
├── src/
│   ├── components/     # Reusable UI components (Modals, Cards, Chat)
│   ├── assets/         # Images and global styles
│   ├── api.js          # Axios API wrappers
│   ├── App.jsx         # Main application layout and state
│   └── main.jsx        # Entry point
├── public/             # Static assets (firmware binaries)
├── tailwind.config.js  # Styling configuration
└── vite.config.js      # Build configuration
```

---

## 📄 License

This project is part of the Smart Learn Avatar ecosystem. See the root `README.md` for licensing information.
