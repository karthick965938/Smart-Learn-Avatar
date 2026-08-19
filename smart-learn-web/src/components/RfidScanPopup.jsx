import React, { useState } from 'react';
import {
    XMarkIcon,
    CreditCardIcon,
    ChatBubbleLeftRightIcon,
    SignalIcon,
} from '@heroicons/react/24/outline';
import { assignRfidCard } from '../api';

const RfidScanPopup = ({
    event,
    kbs = [],
    onClose,
    onAssign,
    onOpenChat,
    showMessage,
}) => {
    const [selectedKbId, setSelectedKbId] = useState('');
    const [saving, setSaving] = useState(false);

    if (!event) return null;

    const handleAssign = async () => {
        if (!selectedKbId) {
            showMessage('Select a Knowledge Base', 'error');
            return;
        }
        setSaving(true);
        try {
            const res = await assignRfidCard(event.uid, selectedKbId);
            showMessage('Knowledge Base assigned to card', 'success');
            onAssign?.(res.data);
            onClose();
        } catch (error) {
            console.error(error);
            const detail = error.response?.data?.detail;
            showMessage(detail || 'Failed to assign Knowledge Base', 'error');
        } finally {
            setSaving(false);
        }
    };

    const handleOpenChat = () => {
        if (event.kb_id) {
            onOpenChat?.({ kbId: event.kb_id, kbName: event.kb_name, uid: event.uid });
            onClose();
        }
    };

    return (
        <div className="fixed inset-0 z-[60] flex items-center justify-center p-4 bg-black/70 backdrop-blur-sm">
            <div className="bg-gray-900 rounded-2xl shadow-2xl w-full max-w-md border-2 border-orange-500/50 overflow-hidden animate-[fadeIn_0.2s_ease-out]">
                <div className="flex items-center justify-between px-5 py-4 border-b border-gray-800 bg-orange-500/10">
                    <div className="flex items-center gap-3">
                        <div className="w-9 h-9 bg-orange-500/20 rounded-xl flex items-center justify-center">
                            <SignalIcon className="w-5 h-5 text-orange-500 animate-pulse" />
                        </div>
                        <div>
                            <h3 className="font-bold text-white">RFID Card Detected</h3>
                            <p className="text-[10px] text-gray-500 uppercase tracking-widest">
                                From IoT Device
                            </p>
                        </div>
                    </div>
                    <button
                        onClick={onClose}
                        className="text-gray-500 hover:text-white transition-colors"
                    >
                        <XMarkIcon className="w-5 h-5" />
                    </button>
                </div>

                <div className="p-5 space-y-5">
                    <div className="flex items-center gap-4 p-4 rounded-xl bg-black/50 border border-gray-800">
                        <CreditCardIcon className="w-8 h-8 text-orange-500 flex-shrink-0" />
                        <div>
                            <p className="text-[10px] font-black text-gray-500 uppercase tracking-widest">
                                Card UID
                            </p>
                            <p className="font-mono text-lg font-bold text-white tracking-widest">
                                {event.uid}
                            </p>
                        </div>
                    </div>

                    {event.assigned ? (
                        <div className="space-y-4">
                            <div className="p-4 rounded-xl bg-[#04B900]/10 border border-[#04B900]/30">
                                <p className="text-[10px] font-black text-[#04B900] uppercase tracking-widest mb-1">
                                    Assigned Knowledge Base
                                </p>
                                <p className="text-white font-semibold">
                                    {event.kb_name || event.kb_id}
                                </p>
                            </div>
                            <button
                                onClick={handleOpenChat}
                                className="w-full flex items-center justify-center gap-2 px-4 py-3 bg-[#04B900] text-white rounded-xl hover:bg-[#04B900]/90 transition-colors font-semibold"
                            >
                                <ChatBubbleLeftRightIcon className="w-5 h-5" />
                                Open Voice Chat
                            </button>
                        </div>
                    ) : (
                        <div className="space-y-4">
                            <p className="text-sm text-gray-400">
                                This card is not linked to a Knowledge Base yet. Choose one to assign:
                            </p>
                            <select
                                value={selectedKbId}
                                onChange={(e) => setSelectedKbId(e.target.value)}
                                className="w-full px-4 py-3 bg-black border border-gray-800 rounded-xl text-white text-sm outline-none focus:ring-1 focus:ring-orange-500"
                            >
                                <option value="">Select Knowledge Base...</option>
                                {kbs.map((kb) => (
                                    <option key={kb.id} value={kb.id}>
                                        {kb.name}
                                    </option>
                                ))}
                            </select>
                            <button
                                onClick={handleAssign}
                                disabled={!selectedKbId || saving}
                                className="w-full px-4 py-3 bg-orange-500 text-white rounded-xl hover:bg-orange-600 disabled:opacity-40 transition-colors font-semibold"
                            >
                                {saving ? 'Assigning...' : 'Assign Knowledge Base'}
                            </button>
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
};

export default RfidScanPopup;
