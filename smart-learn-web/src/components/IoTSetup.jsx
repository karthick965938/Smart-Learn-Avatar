import React, { useState, useEffect } from 'react';
import {
    XMarkIcon,
    SignalIcon,
    TrashIcon,
    ArrowPathIcon,
    CreditCardIcon,
} from '@heroicons/react/24/outline';
import { listRfidCards, assignRfidCard, deleteRfidCard } from '../api';

const UNO_Q_RC522_PINS = [
    { signal: 'VCC', pin: '3.3V' },
    { signal: 'GND', pin: 'GND' },
    { signal: 'RST', pin: 'D9' },
    { signal: 'CS', pin: 'D10' },
    { signal: 'MOSI', pin: 'D11' },
    { signal: 'MISO', pin: 'D12' },
    { signal: 'SCK', pin: 'D13' },
];

const UNO_Q_OLED_PINS = [
    { signal: 'VCC', pin: '3.3V' },
    { signal: 'GND', pin: 'GND' },
    { signal: 'SDA', pin: 'D20' },
    { signal: 'SCL', pin: 'D21' },
];

const IoTSetup = ({ isOpen, onClose, showMessage, kbs = [], onCardsChange }) => {
    const [cards, setCards] = useState([]);
    const [loading, setLoading] = useState(false);
    const [editingUid, setEditingUid] = useState(null);
    const [editKbId, setEditKbId] = useState('');

    const fetchCards = async () => {
        setLoading(true);
        try {
            const res = await listRfidCards();
            setCards(res.data || []);
        } catch (error) {
            console.error(error);
            showMessage('Failed to load RFID cards', 'error');
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        if (isOpen) {
            fetchCards();
        }
    }, [isOpen]);

    const handleAssign = async (uid) => {
        if (!editKbId) {
            showMessage('Select a Knowledge Base first', 'error');
            return;
        }
        try {
            await assignRfidCard(uid, editKbId);
            showMessage('Knowledge Base assigned to card', 'success');
            setEditingUid(null);
            setEditKbId('');
            await fetchCards();
            onCardsChange?.();
        } catch (error) {
            console.error(error);
            showMessage('Failed to assign Knowledge Base', 'error');
        }
    };

    const handleDelete = async (uid) => {
        if (!confirm(`Remove RFID card ${uid}?`)) return;
        try {
            await deleteRfidCard(uid);
            showMessage('RFID card removed', 'success');
            await fetchCards();
            onCardsChange?.();
        } catch (error) {
            console.error(error);
            showMessage('Failed to remove card', 'error');
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/80 backdrop-blur-sm overflow-hidden">
            <div className="bg-gray-900 rounded-3xl shadow-2xl w-full max-w-3xl border border-gray-800 flex flex-col overflow-hidden max-h-[90vh]">
                <div className="flex items-center justify-between px-6 py-4 border-b border-gray-800 flex-shrink-0">
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 bg-orange-500/20 rounded-xl flex items-center justify-center">
                            <SignalIcon className="w-6 h-6 text-orange-500" />
                        </div>
                        <div>
                            <h2 className="text-xl font-black text-white leading-none">RFID Cards</h2>
                            <p className="text-[10px] text-gray-500 uppercase tracking-widest mt-1">
                                RC522 · 13.56 MHz
                            </p>
                        </div>
                    </div>
                    <div className="flex items-center gap-2">
                        <button
                            onClick={fetchCards}
                            disabled={loading}
                            className="p-2 text-gray-500 hover:text-white transition-colors"
                            title="Refresh"
                        >
                            <ArrowPathIcon className={`w-5 h-5 ${loading ? 'animate-spin' : ''}`} />
                        </button>
                        <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors">
                            <XMarkIcon className="w-6 h-6" />
                        </button>
                    </div>
                </div>

                <div className="flex-1 overflow-y-auto p-6 space-y-6">
                    <div className="p-4 rounded-2xl border border-gray-800 bg-black/40 space-y-4">
                        <p className="text-sm text-gray-400 leading-relaxed">
                            Scan an RFID card on your Arduino UNO Q to register it here. A popup will appear
                            automatically so you can assign a Knowledge Base. Cards with an assigned KB will
                            open chat when scanned again.
                        </p>
                        <div>
                            <p className="text-[10px] font-black text-gray-500 uppercase tracking-widest mb-2">
                                RC522 · SPI · UNO Q
                            </p>
                            <div className="grid grid-cols-4 sm:grid-cols-7 gap-2">
                                {UNO_Q_RC522_PINS.map((pin) => (
                                    <div
                                        key={pin.signal}
                                        className="flex flex-col items-center gap-0.5 px-2 py-1.5 rounded-lg bg-gray-900 border border-gray-800"
                                    >
                                        <span className="text-[10px] font-bold text-gray-500">{pin.signal}</span>
                                        <span className="text-[10px] font-mono text-orange-500">{pin.pin}</span>
                                    </div>
                                ))}
                            </div>
                        </div>
                        <div>
                            <p className="text-[10px] font-black text-gray-500 uppercase tracking-widest mb-2">
                                0.96" OLED · SSD1306 · UNO Q
                            </p>
                            <div className="grid grid-cols-4 gap-2">
                                {UNO_Q_OLED_PINS.map((pin) => (
                                    <div
                                        key={pin.signal}
                                        className="flex flex-col items-center gap-0.5 px-2 py-1.5 rounded-lg bg-gray-900 border border-gray-800"
                                    >
                                        <span className="text-[10px] font-bold text-gray-500">{pin.signal}</span>
                                        <span className="text-[10px] font-mono text-sky-400">{pin.pin}</span>
                                    </div>
                                ))}
                            </div>
                        </div>
                    </div>

                    <section className="space-y-3">
                        <h3 className="text-[13px] font-black text-gray-400 uppercase tracking-[0.2em]">
                            Registered Cards
                        </h3>

                        {loading && cards.length === 0 ? (
                            <div className="text-center py-12 text-gray-600 text-sm animate-pulse">
                                Loading cards...
                            </div>
                        ) : cards.length === 0 ? (
                            <div className="text-center py-12 border border-dashed border-gray-800 rounded-2xl">
                                <CreditCardIcon className="w-10 h-10 text-gray-700 mx-auto mb-3" />
                                <p className="text-gray-500 text-sm font-medium">No RFID cards yet</p>
                                <p className="text-gray-600 text-xs mt-1">
                                    Scan a card on the device to get started
                                </p>
                            </div>
                        ) : (
                            <div className="space-y-3">
                                {cards.map((card) => (
                                    <div
                                        key={card.uid}
                                        className="p-4 rounded-2xl border border-gray-800 bg-black/40 flex flex-col gap-3"
                                    >
                                        <div className="flex items-center justify-between">
                                            <div className="flex items-center gap-3">
                                                <div className="w-10 h-10 rounded-xl bg-orange-500/10 flex items-center justify-center">
                                                    <CreditCardIcon className="w-5 h-5 text-orange-500" />
                                                </div>
                                                <div>
                                                    <p className="font-mono text-white font-bold tracking-wider">
                                                        {card.uid}
                                                    </p>
                                                    {card.kb_id ? (
                                                        <p className="text-xs text-[#04B900] mt-0.5">
                                                            {card.kb_name || card.kb_id}
                                                        </p>
                                                    ) : (
                                                        <p className="text-xs text-yellow-500 mt-0.5">
                                                            No Knowledge Base assigned
                                                        </p>
                                                    )}
                                                </div>
                                            </div>
                                            <div className="flex items-center gap-2">
                                                {!card.kb_id && (
                                                    <button
                                                        onClick={() => {
                                                            setEditingUid(card.uid);
                                                            setEditKbId('');
                                                        }}
                                                        className="px-3 py-1.5 text-[11px] font-bold uppercase tracking-wider text-orange-500 border border-orange-500/30 rounded-lg hover:bg-orange-500/10 transition-colors"
                                                    >
                                                        Assign KB
                                                    </button>
                                                )}
                                                <button
                                                    onClick={() => handleDelete(card.uid)}
                                                    className="p-2 text-gray-600 hover:text-red-400 transition-colors"
                                                    title="Remove card"
                                                >
                                                    <TrashIcon className="w-4 h-4" />
                                                </button>
                                            </div>
                                        </div>

                                        {editingUid === card.uid && (
                                            <div className="flex gap-2 pt-2 border-t border-gray-800">
                                                <select
                                                    value={editKbId}
                                                    onChange={(e) => setEditKbId(e.target.value)}
                                                    className="flex-1 px-3 py-2 bg-black border border-gray-800 rounded-xl text-white text-sm outline-none focus:ring-1 focus:ring-orange-500"
                                                >
                                                    <option value="">Select Knowledge Base...</option>
                                                    {kbs.map((kb) => (
                                                        <option key={kb.id} value={kb.id}>
                                                            {kb.name}
                                                        </option>
                                                    ))}
                                                </select>
                                                <button
                                                    onClick={() => handleAssign(card.uid)}
                                                    disabled={!editKbId}
                                                    className="px-4 py-2 bg-orange-500 text-white text-sm font-bold rounded-xl hover:bg-orange-600 disabled:opacity-40 transition-colors"
                                                >
                                                    Save
                                                </button>
                                                <button
                                                    onClick={() => {
                                                        setEditingUid(null);
                                                        setEditKbId('');
                                                    }}
                                                    className="px-3 py-2 text-gray-400 text-sm border border-gray-800 rounded-xl hover:bg-gray-800 transition-colors"
                                                >
                                                    Cancel
                                                </button>
                                            </div>
                                        )}
                                    </div>
                                ))}
                            </div>
                        )}
                    </section>
                </div>

                <div className="px-6 py-4 border-t border-gray-800 flex justify-end bg-gray-950/50">
                    <button
                        onClick={onClose}
                        className="px-6 py-2.5 text-sm font-medium text-gray-400 border border-gray-800 hover:bg-gray-800 rounded-xl transition-all"
                    >
                        Close
                    </button>
                </div>
            </div>
        </div>
    );
};

export default IoTSetup;
