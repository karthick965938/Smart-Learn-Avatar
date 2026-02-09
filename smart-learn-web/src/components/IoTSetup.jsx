import React, { useState, useEffect, useRef } from 'react';
import { XMarkIcon, BoltIcon, CheckCircleIcon, CommandLineIcon, ExclamationTriangleIcon, PlayIcon, PauseIcon, ChevronUpIcon, ChevronDownIcon } from '@heroicons/react/24/outline';
import { generateNvs } from '../api';
import { Transport } from 'esptool-js/lib/webserial';
import { ESPLoader } from 'esptool-js/lib/esploader';

// Female Avatars
import tutorFemale from '../assets/avatars/Tutor - Female.png';
import devFemale from '../assets/avatars/Developer - Female.png';
import docFemale from '../assets/avatars/Doctor - Female.png';
import hrFemale from '../assets/avatars/HR - Female.png';
import policeFemale from '../assets/avatars/Police - Female.png';

// Male Avatars
import tutorMale from '../assets/avatars/Tutor - Male.png';
import devMale from '../assets/avatars/Developer - Male.png';
import docMale from '../assets/avatars/Doctor - Male.png';
import hrMale from '../assets/avatars/HR - Male.png';
import policeMale from '../assets/avatars/Police - Male.png';

// Voices
import alloyAudio from '../assets/voices/alloy.mp3';
import echoAudio from '../assets/voices/echo.mp3';
import fableAudio from '../assets/voices/fable.mp3';
import novaAudio from '../assets/voices/nova.mp3';
import onyxAudio from '../assets/voices/onyx.mp3';
import shimmerAudio from '../assets/voices/shimmer.mp3';

const femaleAvatars = [
    { id: 'tutor-female', name: 'Tutor', img: tutorFemale },
    { id: 'dev-female', name: 'Developer', img: devFemale },
    { id: 'doc-female', name: 'Doctor', img: docFemale },
    { id: 'hr-female', name: 'HR Specialist', img: hrFemale },
    { id: 'police-female', name: 'Police', img: policeFemale },
];

const maleAvatars = [
    { id: 'tutor-male', name: 'Tutor', img: tutorMale },
    { id: 'dev-male', name: 'Developer', img: devMale },
    { id: 'doc-male', name: 'Doctor', img: docMale },
    { id: 'hr-male', name: 'HR Specialist', img: hrMale },
    { id: 'police-male', name: 'Police', img: policeMale },
];

const firmwareOptions = [
    { id: "tutor_female", label: "Tutor Female Avatar", manifest: "/firmware/tutor_female/manifest.json" },
    { id: "tutor_male", label: "Tutor Male Avatar", manifest: "/firmware/tutor_male/manifest.json" },
    { id: "developer_female", label: "Developer Female Avatar", manifest: "/firmware/developer_female/manifest.json" },
    { id: "developer_male", label: "Developer Male Avatar", manifest: "/firmware/developer_male/manifest.json" },
    { id: "doctor_female", label: "Doctor Female Avatar", manifest: "/firmware/doctor_female/manifest.json" },
    { id: "doctor_male", label: "Doctor Male Avatar", manifest: "/firmware/doctor_male/manifest.json" },
    { id: "hr_female", label: "HR Female Avatar", manifest: "/firmware/hr_female/manifest.json" },
    { id: "hr_male", label: "HR Male Avatar", manifest: "/firmware/hr_male/manifest.json" },
    { id: "police_female", label: "Police Female Avatar", manifest: "/firmware/police_female/manifest.json" },
    { id: "police_male", label: "Police Male Avatar", manifest: "/firmware/police_male/manifest.json" },
];

const avatarToFirmwareId = {
    'tutor-female': 'tutor_female', 'tutor-male': 'tutor_male',
    'dev-female': 'developer_female', 'dev-male': 'developer_male',
    'doc-female': 'doctor_female', 'doc-male': 'doctor_male',
    'hr-female': 'hr_female', 'hr-male': 'hr_male',
    'police-female': 'police_female', 'police-male': 'police_male',
};

const FRAME_COUNT = 6;
const FRAME_INTERVAL_MS_LISTEN = 300;
const FRAME_INTERVAL_MS_SPEAK = 300;

// NVS partition: must match partitions.csv (nvs @ 0x9000, size 0x4000)
const NVS_OFFSET = 0x9000;
const NVS_SIZE = 0x4000;
const NVS_END = NVS_OFFSET + NVS_SIZE; // 0xd000

/** Get frame path for listen/speak animation from public/avatars/<folder>/<mode>/ */
function getAvatarFrameSrc(avatarFolder, mode, frameIndex) {
    if (!avatarFolder || frameIndex < 0 || frameIndex >= FRAME_COUNT) return null;
    const i = frameIndex;
    if (mode === 'listen') {
        return `/avatars/${avatarFolder}/listen/avatar_${i}.png`;
    }
    if (mode === 'speak') {
        return `/avatars/${avatarFolder}/speak/speaker_${i}.png`;
    }
    return null;
}

const IoTSetup = ({ isOpen, onClose, showMessage, kbs = [] }) => {
    const [selectedAvatar, setSelectedAvatar] = useState('');
    const [selectedVoice, setSelectedVoice] = useState('');
    const [selectedKbId, setSelectedKbId] = useState('');
    const [selectedFirmware, setSelectedFirmware] = useState(null);
    const [theme, setTheme] = useState('light');
    // Prefill from .env when set; these become read-only (VITE_NETWORK_ACCESS_ID, VITE_ACCESS_CREDENTIALS, VITE_OPENAI_API_KEY)
    const [ssid, setSsid] = useState(() => (import.meta.env.VITE_NETWORK_ACCESS_ID || '').toString().trim());
    const [password, setPassword] = useState(() => (import.meta.env.VITE_ACCESS_CREDENTIALS || '').toString().trim());
    const [openaiKey, setOpenaiKey] = useState(() => (import.meta.env.VITE_OPENAI_API_KEY || '').toString().trim());

    const ssidFromEnv = !!(import.meta.env.VITE_NETWORK_ACCESS_ID);
    const passwordFromEnv = !!(import.meta.env.VITE_ACCESS_CREDENTIALS);
    const openaiKeyFromEnv = !!(import.meta.env.VITE_OPENAI_API_KEY);
    const [baseUrl, setBaseUrl] = useState('');
    const [kbUrl, setKbUrl] = useState('');
    const [apiBaseOverride, setApiBaseOverride] = useState('');
    const [isFlashing, setIsFlashing] = useState(false);
    const [flashLogs, setFlashLogs] = useState([]);
    const [flashProgress, setFlashProgress] = useState(0);
    const [flashStatus, setFlashStatus] = useState(''); // 'connecting', 'loading', 'flashing', 'complete'
    const [isWebSerialSupported, setIsWebSerialSupported] = useState(true);
    const [isPlaying, setIsPlaying] = useState(false);
    const [frameIndex, setFrameIndex] = useState(0);
    const [isLogsExpanded, setIsLogsExpanded] = useState(true);
    const [showLogs, setShowLogs] = useState(false);
    const [currentFileName, setCurrentFileName] = useState('');
    const [isFlashComplete, setIsFlashComplete] = useState(false);
    const audioRef = useRef(null);
    const logContainerRef = useRef(null);

    // Auto-scroll logs
    useEffect(() => {
        if (logContainerRef.current) {
            logContainerRef.current.scrollTop = logContainerRef.current.scrollHeight;
        }
    }, [flashLogs]);

    const avatarFolder = selectedAvatar ? avatarToFirmwareId[selectedAvatar] : null;
    const animMode = isPlaying ? 'speak' : 'listen';

    // Check for Web Serial support
    useEffect(() => {
        if (!('serial' in navigator)) {
            setIsWebSerialSupported(false);
        }
    }, []);

    // Auto-scroll logs
    useEffect(() => {
        if (logContainerRef.current) {
            logContainerRef.current.scrollTop = logContainerRef.current.scrollHeight;
        }
    }, [flashLogs]);

    // Auto-select firmware based on avatar (avatar id -> firmware id via avatarToFirmwareId)
    React.useEffect(() => {
        if (selectedAvatar) {
            const firmwareId = avatarToFirmwareId[selectedAvatar];
            setSelectedFirmware(firmwareId ? firmwareOptions.find(f => f.id === firmwareId) : null);
        } else {
            setSelectedFirmware(null);
        }
    }, [selectedAvatar]);

    // Animate listen/speak frames when an avatar is selected; reset and re-tick when mode (play/stop) changes
    useEffect(() => {
        if (!avatarFolder) return;
        setFrameIndex(0);
        const intervalMs = animMode === 'speak' ? FRAME_INTERVAL_MS_SPEAK : FRAME_INTERVAL_MS_LISTEN;
        const id = setInterval(() => {
            setFrameIndex((prev) => (prev + 1) % FRAME_COUNT);
        }, intervalMs);
        return () => clearInterval(id);
    }, [avatarFolder, animMode]);

    // Auto-update URLs based on selection and environment
    useEffect(() => {
        setBaseUrl('https://api.openai.com/v1/');
        // API base for KB: optional override, else VITE_API_BASE_URL, else current origin
        const apiBase = (apiBaseOverride || import.meta.env.VITE_API_BASE_URL || (typeof window !== 'undefined' ? window.location.origin : '') || '').toString().replace(/\/$/, '');
        if (selectedKbId && apiBase) {
            setKbUrl(`${apiBase}/api/v1/kb/${selectedKbId}/query`);
        } else {
            setKbUrl('');
        }
    }, [selectedKbId, apiBaseOverride]);

    const voices = [
        { id: 'nova', name: 'Nova (Female - Warm)', audio: novaAudio },
        { id: 'shimmer', name: 'Shimmer (Female - Clear)', audio: shimmerAudio },
        { id: 'fable', name: 'Fable (Female - Expressive)', audio: fableAudio },
        { id: 'echo', name: 'Echo (Male - Deep)', audio: echoAudio },
        { id: 'onyx', name: 'Onyx (Male - Rich)', audio: onyxAudio },
        { id: 'alloy', name: 'Alloy (Male - Neutral)', audio: alloyAudio },


    ];

    const currentAvatarData = [...femaleAvatars, ...maleAvatars].find(a => a.id === selectedAvatar);
    const currentVoiceData = voices.find(v => v.id === selectedVoice);

    // Audio Playback Logic
    const togglePlay = () => {
        if (!currentVoiceData?.audio) return;

        if (isPlaying) {
            audioRef.current.pause();
            audioRef.current.currentTime = 0;
            setIsPlaying(false);
        } else {
            if (audioRef.current) {
                audioRef.current.pause();
            }
            audioRef.current = new Audio(currentVoiceData.audio);
            audioRef.current.play();
            setIsPlaying(true);
            audioRef.current.onended = () => setIsPlaying(false);
        }
    };

    // Stop audio when changing voice or closing
    useEffect(() => {
        if (audioRef.current) {
            audioRef.current.pause();
            setIsPlaying(false);
        }
    }, [selectedVoice, onClose]);

    useEffect(() => {
        return () => {
            if (audioRef.current) {
                audioRef.current.pause();
            }
        };
    }, []);

    const themes = [
        { id: 'light', name: 'Light Mode' },
        { id: 'dark', name: 'Dark Mode' },
    ];

    const addLog = (msg, type = 'info') => {
        const time = new Date().toLocaleTimeString();
        setFlashLogs(prev => [...prev, { msg, type, time }]);
        console.log(`[Flash ${type.toUpperCase()}] ${msg}`);
    };

    const handleFlash = async () => {
        if (!isReadyToFlash) return;

        setIsFlashing(true);
        setShowLogs(true);
        setIsLogsExpanded(true);
        setIsFlashComplete(false);
        setCurrentFileName('');
        setFlashLogs([]);
        setFlashProgress(0);
        setFlashStatus('connecting');
        addLog("Starting firmware flash process...", "info");

        let transport;
        try {
            // 0. Generate NVS binary
            setFlashStatus('connecting');
            addLog("Generating NVS configuration binary...", "info");
            const nvsResponse = await generateNvs({
                ssid,
                password,
                openai_key: openaiKey,
                base_url: baseUrl,
                kb_url: kbUrl,
                tts_voice: selectedVoice,
                theme: theme
            });

            const nvsBin = new Uint8Array(nvsResponse.data);
            addLog(`NVS binary generated successfully (${nvsBin.length} bytes)`, "success");

            // 1. Request Port
            addLog("Please select the ESP32-S3-BOX-3 port from the browser popup.", "warn");
            addLog("Requesting serial port access...", "info");

            let port;
            try {
                port = await navigator.serial.requestPort();
            } catch (portError) {
                if (portError.name === 'NotFoundError') {
                    addLog("Selection cancelled by user.", "info");
                    setIsFlashing(false);
                    return;
                }
                throw portError;
            }

            if (!port) {
                throw new Error("No port selected");
            }

            // Fix: Check if port is already open and close it if necessary
            // Note: esptool-js Transport handles the open internally, but if we have a stale connection
            // it can cause a conflict.
            if (port.writable || port.readable) {
                try {
                    addLog("Closing existing port connection...", "info");
                    await port.close();
                } catch (e) {
                    console.warn("Failed to close existing port:", e);
                }
            }

            console.log("Flash Debug - SerialPort:", port);

            // Create transport
            transport = new Transport(port);

            // 2. Initialize Loader (Using the new Object-based Options API)
            addLog("Port selected. Initializing connection...", "info");

            const esploader = new ESPLoader({
                transport: transport,
                baudrate: 115200,
                terminal: {
                    clean: () => console.log("Terminal cleaned"),
                    write: (msg) => addLog(msg, 'info'),
                    writeLine: (msg) => addLog(msg, 'info'),
                    info: (msg) => addLog(msg, 'info'),
                    error: (msg) => addLog(msg, 'error'),
                    warn: (msg) => addLog(msg, 'warn'),
                    debug: (msg) => console.debug(msg),
                    log: (msg) => addLog(msg, 'info')
                }
            });

            addLog("Connecting to ESP32... (If it sticks, hold BOOT button)", "warn");

            // esploader.main() handles the connection and chip detection
            await esploader.main();

            const chipName = esploader.chip ? esploader.chip.CHIP_NAME : "ESP32 Device";
            addLog(`Connected to ${chipName} successfully`, "success");

            // 3. Load Manifest and Files
            setFlashStatus('loading');
            addLog(`Reading manifest...`, "info");
            const response = await fetch(selectedFirmware.manifest);
            if (!response.ok) throw new Error("Failed to load manifest");
            const manifest = await response.json();

            const basePath = selectedFirmware.manifest.substring(0, selectedFirmware.manifest.lastIndexOf('/'));
            const fileArray = [];

            // Ensure NVS binary is exactly NVS partition size (truncate if API ever returns more)
            const nvsToWrite = nvsBin.byteLength > NVS_SIZE
                ? nvsBin.subarray(0, NVS_SIZE)
                : nvsBin;
            addLog(`NVS partition: 0x${NVS_OFFSET.toString(16)}–0x${NVS_END.toString(16)} (${NVS_SIZE} bytes)`, "info");

            addLog(`Downloading firmware components...`, "info");
            for (const file of manifest.files) {
                const fileResp = await fetch(`${basePath}/${file.path}`);
                if (!fileResp.ok) throw new Error(`Failed to download ${file.path}`);

                const buffer = await fileResp.arrayBuffer();
                const ui8 = new Uint8Array(buffer);
                const offStr = String(file.offset || '0');
                const fileStart = offStr.toLowerCase().startsWith('0x')
                    ? parseInt(offStr, 16)
                    : parseInt(offStr, 10);
                const fileEnd = fileStart + ui8.length;

                // Skip any manifest file that would overlap the NVS partition so our generated NVS wins
                if (fileStart < NVS_END && fileEnd > NVS_OFFSET) {
                    addLog(`Skipping ${file.path} (overlaps NVS at 0x${NVS_OFFSET.toString(16)}); using generated NVS`, "warn");
                    continue;
                }

                let binary = "";
                const chunkSize = 10000;
                for (let j = 0; j < ui8.length; j += chunkSize) {
                    const chunk = ui8.subarray(j, j + chunkSize);
                    binary += String.fromCharCode.apply(null, chunk);
                }

                fileArray.push({ data: binary, address: fileStart, path: file.path });
                addLog(`Loaded ${file.path} (${(ui8.length / 1024).toFixed(1)} KB) @ 0x${fileStart.toString(16)}`, "info");
            }

            // Append generated NVS last so it is the final write for the NVS region (CONFIG.INI / runtime config)
            let nvsBinaryStr = "";
            const nvsChunkSize = 10000;
            for (let j = 0; j < nvsToWrite.length; j += nvsChunkSize) {
                const chunk = nvsToWrite.subarray(j, j + nvsChunkSize);
                nvsBinaryStr += String.fromCharCode.apply(null, chunk);
            }
            fileArray.push({ data: nvsBinaryStr, address: NVS_OFFSET, path: "nvs.bin" });
            addLog(`Added generated NVS at 0x${NVS_OFFSET.toString(16)} (${nvsToWrite.length} bytes) – config will apply after flash`, "success");

            // 4. Write to Flash
            setFlashStatus('flashing');
            addLog("Starting flash write... DO NOT disconnect or close this tab!", "warn");

            const filenames = fileArray.map(f => (f.path || '').split('/').pop() || 'nvs.bin');

            await esploader.writeFlash({
                fileArray,
                flashSize: "keep",
                flashMode: "keep",
                flashFreq: "keep",
                eraseAll: false,
                compress: true,
                reportProgress: (fileIndex, written, total) => {
                    const progress = Math.round((written / total) * 100);
                    setFlashProgress(progress);
                    if (filenames[fileIndex]) {
                        setCurrentFileName(filenames[fileIndex]);
                    }
                }
            });

            addLog("Flash complete! The device will now reboot.", "success");
            try {
                await esploader.hardReset();
            } catch (resetErr) {
                console.warn("Hard reset failed, user might need to press Reset button:", resetErr);
            }
            setFlashProgress(100);
            setFlashStatus('complete');
            setIsFlashComplete(true);
            setCurrentFileName('');
            showMessage("Firmware flashed successfully!", "success");

            // No longer auto-closing results to allow log review
            setIsFlashing(false);

        } catch (error) {
            console.error(error);
            const errMsg = error.message || "Unknown error";
            addLog(`Error: ${errMsg}`, "error");
            if (errMsg.includes("User cancelled")) {
                addLog("Selection cancelled by user.", "info");
            } else {
                showMessage("Flash failed: " + errMsg, "error");
            }
        } finally {
            if (transport) {
                try {
                    await transport.disconnect();
                    addLog("Serial port disconnected.", "info");
                } catch (e) {
                    console.error("Disconnect error:", e);
                }
            }
            setIsFlashing(false);
        }
    };

    const isReadyToFlash = selectedKbId && selectedAvatar && selectedVoice && selectedFirmware && ssid && password && openaiKey;

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/80 backdrop-blur-sm overflow-hidden">
            <div className="bg-gray-900 rounded-3xl shadow-2xl w-full max-w-7xl border border-gray-800 flex flex-col overflow-hidden max-h-[95vh]">
                {/* Header - Compact */}
                <div className="flex items-center justify-between px-6 py-4 border-b border-gray-800 flex-shrink-0 bg-gray-900/50 backdrop-blur-md">
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 bg-orange-500/20 rounded-xl flex items-center justify-center">
                            <BoltIcon className="w-6 h-6 text-orange-500" />
                        </div>
                        <div>
                            <h2 className="text-xl font-black text-white leading-none">IoT Setup Engine</h2>
                            <p className="text-[10px] text-gray-500 uppercase tracking-widest mt-1">Direct Hardware Configuration</p>
                        </div>
                    </div>
                    <button
                        onClick={onClose}
                        disabled={isFlashing}
                        className={`text-gray-500 hover:text-white transition-colors ${isFlashing ? 'opacity-20 cursor-not-allowed' : ''}`}
                    >
                        <XMarkIcon className="w-6 h-6" />
                    </button>
                </div>

                {/* Main Content Area - Scrollable */}
                <div className="flex-1 overflow-y-auto p-6 scrollbar-thin scrollbar-thumb-gray-800 scrollbar-track-transparent">
                    <div className="space-y-8">
                        {/* Top Section: Selection & Preview */}
                        <div className="grid grid-cols-12 gap-8 items-start">
                            {/* Selection Sections (span 7) */}
                            <div className="col-span-12 lg:col-span-7 space-y-8">
                                {/* Avatar Section */}
                                <section>
                                    <div className="flex items-center justify-between mb-4">
                                        <h3 className="text-[13px] font-black text-gray-400 uppercase tracking-[0.2em]">01. Persona Selection</h3>
                                        <span className="text-[11px] text-orange-500 font-bold px-2 py-0.5 bg-orange-500/10 rounded">Required</span>
                                    </div>
                                    <div className="grid grid-cols-5 gap-3">
                                        {[...femaleAvatars, ...maleAvatars].map((avatar) => (
                                            <button
                                                key={avatar.id}
                                                onClick={() => setSelectedAvatar(avatar.id)}
                                                className={`group relative aspect-square rounded-2xl overflow-hidden border-2 transition-all ${selectedAvatar === avatar.id
                                                    ? 'border-orange-500 ring-4 ring-orange-500/10 scale-105 z-10'
                                                    : 'border-gray-800 hover:border-gray-600'
                                                    }`}
                                            >
                                                <img src={avatar.img} alt={avatar.name} className="w-full h-full object-cover" />
                                                <div className="absolute inset-0 bg-black/40 opacity-0 group-hover:opacity-100 transition-opacity flex items-center justify-center">
                                                    <span className="text-[10px] text-white font-black uppercase tracking-widest">{avatar.name}</span>
                                                </div>
                                                {selectedAvatar === avatar.id && (
                                                    <div className="absolute top-1.5 right-1.5">
                                                        <CheckCircleIcon className="w-5 h-5 text-orange-500 bg-gray-900 rounded-full" />
                                                    </div>
                                                )}
                                            </button>
                                        ))}
                                    </div>
                                </section>

                                {/* Voice Profile Section */}
                                <section>
                                    <h3 className="text-[13px] font-black text-gray-400 uppercase tracking-[0.2em] mb-4">02. Audio Synthesis Engine</h3>
                                    <div className="grid grid-cols-3 gap-3">
                                        {voices.map((voice) => (
                                            <button
                                                key={voice.id}
                                                onClick={() => setSelectedVoice(voice.id)}
                                                className={`flex items-center gap-3 p-3 rounded-2xl border transition-all text-left ${selectedVoice === voice.id
                                                    ? 'border-orange-500 bg-orange-500/5'
                                                    : 'border-gray-800 bg-black/40 hover:border-gray-700'
                                                    }`}
                                            >
                                                <div className={`w-2.5 h-2.5 rounded-full ${selectedVoice === voice.id ? 'bg-orange-500 shadow-[0_0_8px_rgba(249,115,22,0.5)]' : 'bg-gray-700'}`} />
                                                <span className={`text-[11px] font-bold truncate tracking-wide ${selectedVoice === voice.id ? 'text-white' : 'text-gray-400'}`}>{voice.name}</span>
                                            </button>
                                        ))}
                                    </div>
                                </section>

                                {/* Visual Atmosphere Section */}
                                <section>
                                    <h3 className="text-[13px] font-black text-gray-400 uppercase tracking-[0.2em] mb-4">03. Visual Atmosphere</h3>
                                    <div className="grid grid-cols-2 gap-3">
                                        {themes.map((t) => (
                                            <button
                                                key={t.id}
                                                onClick={() => setTheme(t.id)}
                                                className={`flex items-center justify-center gap-3 p-3 rounded-2xl border transition-all ${theme === t.id
                                                    ? 'border-orange-500 bg-orange-500/5 text-white'
                                                    : 'border-gray-800 bg-black/40 text-gray-400 hover:border-gray-700'
                                                    }`}
                                            >
                                                <div className={`w-3 h-3 rounded-full border-2 ${theme === t.id ? 'border-orange-500 bg-orange-500' : 'border-gray-600'}`} />
                                                <span className="text-[11px] font-black uppercase tracking-widest">{t.name}</span>
                                            </button>
                                        ))}
                                    </div>
                                </section>
                            </div>

                            {/* Preview Section (span 5) */}
                            <div className="col-span-12 lg:col-span-5">
                                <div className={`relative flex flex-col border rounded-[2.5rem] transition-all duration-500 overflow-hidden ${theme === 'dark'
                                    ? 'bg-[#0a0a0c] border-gray-800'
                                    : 'bg-gray-50 border-gray-200 shadow-xl'
                                    }`}>

                                    <div className="p-4 flex items-center justify-between z-10">
                                        <div className="flex items-center gap-2">
                                            <div className="w-2 h-2 rounded-full bg-orange-500 animate-pulse" />
                                            <span className="text-[11px] font-black text-gray-500 uppercase tracking-[0.2em]">Avatar Preview</span>
                                        </div>
                                        <span className="text-[10px] px-2 py-1 bg-green-500/10 text-green-500 rounded-lg border border-green-500/20 font-black uppercase tracking-widest">Sync Status: Online</span>
                                    </div>

                                    <div className="relative flex-1 flex items-center justify-center p-2 min-h-[300px]">
                                        <div className={`absolute w-48 h-48 bg-orange-500/10 blur-[80px] rounded-full transition-opacity duration-1000 ${currentAvatarData ? 'opacity-100' : 'opacity-0'}`} />

                                        <div className={`relative w-full max-w-[300px] aspect-[4/5] rounded-[2.5rem] overflow-hidden border transition-all duration-500 ${theme === 'dark' ? 'bg-black border-gray-800' : 'bg-white border-gray-200 shadow-2xl'}`}>
                                            <div className="absolute inset-0 z-10 pointer-events-none">
                                                <div className="w-full h-[1px] bg-orange-500/30 absolute top-0 animate-[scan_8s_linear_infinite]" />
                                            </div>

                                            {currentAvatarData ? (
                                                <div className="w-full h-full flex items-center justify-center p-2">
                                                    <img
                                                        src={avatarFolder ? getAvatarFrameSrc(avatarFolder, animMode, frameIndex) : currentAvatarData.img}
                                                        alt="Preview"
                                                        className="w-full h-full object-contain"
                                                        style={{ imageRendering: 'auto' }}
                                                    />
                                                    {currentVoiceData && (
                                                        <button
                                                            onClick={togglePlay}
                                                            className={`absolute bottom-6 right-6 w-12 h-12 rounded-2xl flex items-center justify-center transition-all z-20 shadow-2xl ${isPlaying ? 'bg-red-500 scale-95 shadow-red-500/40' : 'bg-orange-500 hover:scale-110 shadow-orange-500/40'}`}
                                                        >
                                                            {isPlaying ? <PauseIcon className="w-6 h-6 text-white fill-current" /> : <PlayIcon className="w-6 h-6 text-white fill-current ml-1" />}
                                                        </button>
                                                    )}
                                                </div>
                                            ) : (
                                                <div className="flex flex-col items-center justify-center h-full gap-4 text-gray-800">
                                                    <BoltIcon className="w-12 h-12 opacity-5 animate-pulse" />
                                                    <span className="text-[11px] font-black uppercase tracking-[0.4em] opacity-20">Standby Mode</span>
                                                </div>
                                            )}
                                        </div>
                                    </div>

                                    <div className={`p-4 border-t transition-colors duration-500 ${theme === 'dark' ? 'bg-gray-900/60 border-gray-800/40' : 'bg-white border-gray-100'}`}>
                                        {currentAvatarData ? (
                                            <div className="space-y-4">
                                                <div className="flex items-center justify-between">
                                                    <div>
                                                        <span className="text-[11px] text-gray-500 font-black uppercase tracking-widest block mb-0.5">Identified Aura</span>
                                                        <h3 className={`text-xl font-black ${theme === 'dark' ? 'text-white' : 'text-gray-900'} truncate`}>{currentAvatarData.name}</h3>
                                                    </div>
                                                    <div className="flex gap-1 h-6 items-end px-2">
                                                        {[1, 2, 3, 4, 5, 6].map(i => <div key={i} className={`w-1.5 bg-orange-500/40 rounded-full transition-all ${isPlaying ? 'animate-bounce' : 'h-1.5'}`} style={{ height: isPlaying ? `${Math.random() * 100}%` : '25%', animationDelay: `${i * 0.1}s` }} />)}
                                                    </div>
                                                </div>
                                                <div className="flex items-center gap-3 p-2 rounded-2xl border border-orange-500/10 bg-orange-500/5">
                                                    <div className="w-8 h-8 rounded-lg bg-orange-500/10 flex items-center justify-center">
                                                        <BoltIcon className="w-4 h-4 text-orange-500" />
                                                    </div>
                                                    <div className="min-w-0 flex-1">
                                                        <span className="text-[11px] text-gray-500 font-black uppercase tracking-widest block mb-0.5">Active Synthesis</span>
                                                        <span className="text-[13px] font-bold text-orange-500 truncate block">{currentVoiceData ? currentVoiceData.name : 'Vocal Interface: Off'}</span>
                                                    </div>
                                                </div>
                                            </div>
                                        ) : (
                                            <div className="h-16 flex items-center justify-center rounded-2xl bg-black/10 border border-dashed border-gray-800/30">
                                                <span className="text-[11px] font-black text-gray-600 uppercase tracking-widest">Protocol Initialization...</span>
                                            </div>
                                        )}
                                    </div>
                                </div>
                            </div>
                        </div>

                        {/* Bottom Section: Environmental Configuration */}
                        <section className="space-y-6">
                            <h3 className="text-[13px] font-black text-gray-400 uppercase tracking-[0.2em] border-b border-gray-800 pb-3">04. Environmental Configuration</h3>

                            <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
                                <div className="space-y-2">
                                    <div className="flex items-center justify-between mb-1">
                                        <label className="text-[11px] font-black text-gray-500 uppercase tracking-widest ml-1">Knowledge Base</label>
                                        <span className="text-[9px] text-orange-500 font-bold px-1.5 py-0.5 bg-orange-500/10 rounded">Required</span>
                                    </div>
                                    <select
                                        value={selectedKbId}
                                        onChange={(e) => setSelectedKbId(e.target.value)}
                                        className="w-full px-4 py-2.5 bg-black border border-gray-800 rounded-xl focus:ring-1 focus:ring-orange-500 focus:border-orange-500 outline-none text-white text-sm cursor-pointer appearance-none"
                                    >
                                        <option value="">Select Knowledge Base...</option>
                                        {kbs.map((kb) => <option key={kb.id} value={kb.id}>{kb.name}</option>)}
                                    </select>
                                </div>
                                <div className="space-y-2">
                                    <div className="flex items-center justify-between mb-1">
                                        <label className="text-[11px] font-black text-gray-500 uppercase tracking-widest ml-1">Network Access ID</label>
                                        <span className="flex items-center gap-1.5">
                                            {ssidFromEnv && <span className="text-[9px] text-gray-500">(from .env)</span>}
                                            <span className="text-[9px] text-orange-500 font-bold px-1.5 py-0.5 bg-orange-500/10 rounded">Required</span>
                                        </span>
                                    </div>
                                    <input
                                        type="text"
                                        value={ssid}
                                        autoComplete="off"
                                        onChange={(e) => !ssidFromEnv && setSsid(e.target.value)}
                                        readOnly={ssidFromEnv}
                                        placeholder="SSID Identification"
                                        title={ssidFromEnv ? 'Filled from .env (not editable)' : undefined}
                                        className={`w-full px-4 py-2.5 border border-gray-800 rounded-xl outline-none text-white text-sm placeholder:text-gray-700 ${ssidFromEnv ? 'bg-gray-950 cursor-not-allowed' : 'bg-black focus:ring-1 focus:ring-orange-500 focus:border-orange-500'}`}
                                    />
                                </div>
                                <div className="space-y-2">
                                    <div className="flex items-center justify-between mb-1">
                                        <label className="text-[11px] font-black text-gray-500 uppercase tracking-widest ml-1">Access Credentials</label>
                                        <span className="flex items-center gap-1.5">
                                            {passwordFromEnv && <span className="text-[9px] text-gray-500">(from .env)</span>}
                                            <span className="text-[9px] text-orange-500 font-bold px-1.5 py-0.5 bg-orange-500/10 rounded">Required</span>
                                        </span>
                                    </div>
                                    <input
                                        type="password"
                                        value={password}
                                        autoComplete="new-password"
                                        onChange={(e) => !passwordFromEnv && setPassword(e.target.value)}
                                        readOnly={passwordFromEnv}
                                        placeholder="••••••••"
                                        title={passwordFromEnv ? 'Filled from .env (not editable)' : undefined}
                                        className={`w-full px-4 py-2.5 border border-gray-800 rounded-xl outline-none text-white text-sm ${passwordFromEnv ? 'bg-gray-950 cursor-not-allowed' : 'bg-black focus:ring-1 focus:ring-orange-500 focus:border-orange-500'}`}
                                    />
                                </div>
                            </div>

                            <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
                                <div className="space-y-2">
                                    <div className="flex items-center justify-between mb-1">
                                        <label className="text-[11px] font-black text-gray-500 uppercase tracking-widest ml-1">OpenAI Key</label>
                                        <span className="flex items-center gap-1.5">
                                            {openaiKeyFromEnv && <span className="text-[9px] text-gray-500">(from .env)</span>}
                                            <span className="text-[9px] text-orange-500 font-bold px-1.5 py-0.5 bg-orange-500/10 rounded">Required</span>
                                        </span>
                                    </div>
                                    <input
                                        type="password"
                                        value={openaiKey}
                                        autoComplete="new-password"
                                        onChange={(e) => !openaiKeyFromEnv && setOpenaiKey(e.target.value)}
                                        readOnly={openaiKeyFromEnv}
                                        placeholder="sk-••••••••••••••••••••••••••••"
                                        title={openaiKeyFromEnv ? 'Filled from .env (not editable)' : undefined}
                                        className={`w-full px-4 py-2.5 border border-gray-800 rounded-xl outline-none text-white text-sm font-mono ${openaiKeyFromEnv ? 'bg-gray-950 cursor-not-allowed' : 'bg-black focus:ring-1 focus:ring-orange-500 focus:border-orange-500'}`}
                                    />
                                </div>
                                {/* <div className="space-y-2">
                                    <label className="text-[11px] font-black text-gray-500 uppercase tracking-widest ml-1">API Base URL (optional)</label>
                                    <input
                                        type="text"
                                        value={apiBaseOverride}
                                        onChange={(e) => setApiBaseOverride(e.target.value)}
                                        placeholder="e.g. https://your-api.com — overrides VITE_API_BASE_URL for KB URL"
                                        className="w-full px-4 py-2.5 bg-black border border-gray-800 rounded-xl focus:ring-1 focus:ring-orange-500 focus:border-orange-500 outline-none text-white text-sm placeholder:text-gray-600"
                                    />
                                </div> */}
                                <div className="space-y-2">
                                    <label className="text-[11px] font-black text-gray-500 uppercase tracking-widest ml-1">Knowledge Base URL</label>
                                    <div className="relative">
                                        <input
                                            type="text"
                                            value={selectedKbId ? kbUrl.replace(selectedKbId, 'xxxxxxxx') : ''}
                                            readOnly
                                            disabled
                                            className="w-full px-4 py-2.5 bg-gray-950 border border-gray-800 rounded-xl text-[11px] text-gray-600 font-mono italic pr-12"
                                        />
                                        <CheckCircleIcon className={`absolute right-4 top-1/2 -translate-y-1/2 w-5 h-5 ${selectedKbId ? 'text-orange-500 opacity-100' : 'opacity-10'}`} />
                                    </div>
                                </div>
                                {/* <div className="space-y-2">
                                    <label className="text-[11px] font-black text-gray-500 uppercase tracking-widest ml-1">Firmware Package</label>
                                    <div className={`px-4 py-2.5 rounded-xl border flex items-center justify-between ${selectedFirmware ? 'bg-orange-500/5 border-orange-500/20' : 'bg-gray-800/20 border-gray-800'}`}>
                                        <span className={`text-sm font-black truncate ${selectedFirmware ? 'text-white' : 'text-gray-600 italic'}`}>
                                            {selectedFirmware ? selectedFirmware.label : 'AWAITING_SELECTION'}
                                        </span>
                                        {selectedFirmware && <BoltIcon className="w-4 h-4 text-orange-500" />}
                                    </div>
                                </div> */}
                            </div>
                        </section>

                        {/* Enhanced Flashing Overlay */}
                        {showLogs && (
                            <div className="relative p-6 bg-black border-2 border-orange-500/40 rounded-3xl space-y-5 shadow-[0_0_40px_rgba(249,115,22,0.15)] backdrop-blur-md transition-all duration-300">
                                {/* Header with Dynamic Status */}
                                <div className="flex items-center justify-between border-b border-gray-800 pb-3">
                                    <div className="flex items-center gap-3">
                                        <div className="relative">
                                            <CommandLineIcon className="w-5 h-5 text-orange-500" />
                                            <div className="absolute inset-0 bg-orange-500 blur-md opacity-20 animate-pulse" />
                                        </div>
                                        <span className="text-xs font-black text-white uppercase tracking-[0.2em]">
                                            {flashStatus === 'connecting' && 'Connection Core Initializing'}
                                            {flashStatus === 'loading' && 'Downloading Neural Assets'}
                                            {flashStatus === 'flashing' && 'Writing Physical Sectors'}
                                            {flashStatus === 'complete' && 'Rebooting Knowledge Module'}
                                            {!flashStatus && 'System Write Protocol'}
                                        </span>
                                    </div>
                                    <div className="flex items-center gap-3">
                                        {currentFileName && (
                                            <span className="text-[10px] font-mono text-gray-400 bg-gray-900 px-2 py-0.5 rounded border border-gray-800 animate-pulse">
                                                {currentFileName}
                                            </span>
                                        )}
                                        <span className="text-xs font-black text-orange-500 font-mono tracking-tighter">{flashProgress}%</span>
                                        <button
                                            onClick={() => setIsLogsExpanded(!isLogsExpanded)}
                                            className="p-1 hover:bg-gray-800 rounded-lg transition-colors text-gray-500 hover:text-white"
                                        >
                                            {isLogsExpanded ? <ChevronUpIcon className="w-4 h-4" /> : <ChevronDownIcon className="w-4 h-4" />}
                                        </button>
                                    </div>
                                </div>

                                {isLogsExpanded && (
                                    <>
                                        {/* Progress Bar */}
                                        <div className="space-y-2">
                                            <div className="w-full bg-gray-950 rounded-full h-3 p-1 ring-1 ring-white/5">
                                                <div
                                                    className="h-full rounded-full bg-[linear-gradient(90deg,rgb(249,115,22),rgb(194,65,12))] transition-all duration-300 relative group"
                                                    style={{ width: `${flashProgress}%` }}
                                                >
                                                    <div className="absolute inset-0 bg-white/20 rounded-full" />
                                                    <div className="absolute -inset-1 bg-orange-500/20 blur-md group-hover:opacity-100 opacity-50 transition-opacity" />
                                                </div>
                                            </div>
                                        </div>

                                        {/* Premium Log Interface */}
                                        <div
                                            ref={logContainerRef}
                                            className="h-40 overflow-y-auto font-mono text-[11px] space-y-1.5 scrollbar-thin scrollbar-thumb-orange-500/20 pr-2 custom-scrollbar"
                                        >
                                            {flashLogs.map((log, i) => (
                                                <div key={i} className={`flex items-start gap-3 py-0.5 transition-colors duration-200 border-l-2 pl-3 ${log.type === 'error' ? 'border-red-500 bg-red-500/5' :
                                                    log.type === 'success' ? 'border-green-500 bg-green-500/5' :
                                                        log.type === 'warn' ? 'border-yellow-500 bg-yellow-500/5' :
                                                            'border-transparent'
                                                    }`}>
                                                    <span className="text-gray-600 font-bold opacity-40 shrink-0 text-[9px] uppercase tracking-tighter mt-0.5">[{log.time.split(' ')[0]}]</span>
                                                    <span className={`leading-relaxed break-words ${log.type === 'error' ? 'text-red-400 font-bold' :
                                                        log.type === 'success' ? 'text-green-400 font-bold' :
                                                            log.type === 'warn' ? 'text-yellow-400 italic' :
                                                                'text-gray-400'
                                                        }`}>
                                                        {log.msg}
                                                    </span>
                                                </div>
                                            ))}
                                            {flashLogs.length === 0 && (
                                                <div className="text-gray-700 italic animate-pulse">Initializing data stream...</div>
                                            )}
                                        </div>
                                    </>
                                )}
                            </div>
                        )}
                    </div>
                </div>

                {/* Web Serial Not Supported Warning */}
                {!isWebSerialSupported && (
                    <div className="mx-6 mb-6 bg-red-500/10 border border-red-500/20 rounded-2xl p-4 flex items-start gap-3">
                        <div className="w-10 h-10 bg-red-500/20 rounded-lg flex items-center justify-center flex-shrink-0">
                            <ExclamationTriangleIcon className="w-6 h-6 text-red-500" />
                        </div>
                        <div>
                            <p className="text-sm font-bold text-red-500">Browser Environment Error</p>
                            <p className="text-xs text-red-400/80 mt-0.5 leading-relaxed">
                                The Web Serial API is not supported in your current browser session. This tool requires a browser that supports direct hardware communication (e.g. Chrome 89+, Edge 89+, or Opera).
                            </p>
                        </div>
                    </div>
                )}

                {/* Footer - Dense */}
                <div className="px-6 py-4 border-t border-gray-800 flex items-center justify-between bg-gray-950/50 backdrop-blur-md flex-shrink-0">
                    <p className="text-[10px] text-gray-500 font-medium">
                        {!isWebSerialSupported ? <span className="text-red-500 font-black tracking-widest">ENVIRONMENT_INCOMPATIBLE</span> : <span className="tracking-widest opacity-40">READY_FOR_DATA_TRANSFER</span>}
                    </p>
                    <div className="flex gap-3">
                        <button onClick={onClose} className="px-6 py-2.5 text-[11px] font-black uppercase tracking-widest text-gray-400 border border-gray-800 hover:bg-gray-800 rounded-xl transition-all">
                            Terminate
                        </button>
                        <button
                            onClick={handleFlash}
                            disabled={!isReadyToFlash || isFlashing || !isWebSerialSupported || isFlashComplete}
                            className={`px-8 py-2.5 rounded-xl transition-all text-[11px] font-black uppercase tracking-widest flex items-center gap-2 shadow-2xl ${isReadyToFlash && !isFlashing && isWebSerialSupported && !isFlashComplete
                                ? 'bg-orange-500 text-white hover:bg-orange-600 hover:shadow-orange-500/40 hover:-translate-y-0.5'
                                : 'bg-gray-800 text-gray-600 cursor-not-allowed border border-gray-700'
                                }`}
                        >
                            {isFlashing ? (
                                <>
                                    <div className="w-3 h-3 border-2 border-white/30 border-t-white rounded-full animate-spin" />
                                    Synchronizing...
                                </>
                            ) : isFlashComplete ? (
                                <>
                                    <CheckCircleIcon className="w-4 h-4 text-green-500" />
                                    Flash Secure
                                </>
                            ) : (
                                <>
                                    <BoltIcon className="w-4 h-4" />
                                    Initiate Flash
                                </>
                            )}
                        </button>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default IoTSetup;
