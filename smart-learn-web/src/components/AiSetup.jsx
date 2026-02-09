import React, { useState, useEffect } from 'react';
import { XMarkIcon, SparklesIcon } from '@heroicons/react/24/outline';
import { setKBMetadata } from '../api';

const CONVERSATION_TYPE_OPTIONS = [
    'Q&A',
    'Follow-up Question',
    'Revision Mode',
];

const AiSetup = ({ isOpen, onClose, showMessage, kbs = [], refreshKBs }) => {
    const [aiName, setAiName] = useState('Smart Assistant');
    const [systemPrompt, setSystemPrompt] = useState('You are a helpful AI assistant.');
    const [selectedKbId, setSelectedKbId] = useState('');
    const [useCustomInstructions, setUseCustomInstructions] = useState(false);
    const [conversationTypes, setConversationTypes] = useState([]);

    // Load stored config when modal opens
    useEffect(() => {
        if (isOpen) {
            const stored = localStorage.getItem('ai_config');
            if (stored) {
                try {
                    const config = JSON.parse(stored);
                    if (config.name) setAiName(config.name);
                    if (config.prompt) setSystemPrompt(config.prompt);
                    if (config.kbId) setSelectedKbId(config.kbId);
                    if (config.useCustomInstructions !== undefined) setUseCustomInstructions(config.useCustomInstructions);
                    if (Array.isArray(config.conversationTypes)) setConversationTypes(config.conversationTypes);
                } catch (e) {
                    console.error('Error loading AI config:', e);
                }
            }
        }
    }, [isOpen]);

    // Auto-populate Assistant Name and Instructions based on selected Knowledge Base
    useEffect(() => {
        if (selectedKbId && kbs && kbs.length > 0) {
            const selectedKb = kbs.find(kb => kb.id === selectedKbId);
            if (selectedKb) {
                // Priority 1: Usestored metadata if it exists
                // Use assistant_name if it exists and is NOT empty, otherwise fallback to KB name
                const nextAiName = selectedKb.assistant_name || selectedKb.name || 'Assistant';
                const nextPrompt = selectedKb.instruction || 'You are a helpful AI assistant.';

                // Robust boolean check for the flag
                const rawEnabled = selectedKb.custom_instruction;
                const nextEnabled = typeof rawEnabled === 'string'
                    ? rawEnabled.toLowerCase() === 'true'
                    : !!rawEnabled;

                // Set states
                setAiName(nextAiName);
                setSystemPrompt(nextPrompt);
                setUseCustomInstructions(nextEnabled);
                setConversationTypes(Array.isArray(selectedKb.conversation_types) ? selectedKb.conversation_types : []);
            }
        } else if (!selectedKbId) {
            // Reset to defaults if no KB is selected
            setAiName('Smart Assistant');
            setSystemPrompt('You are a helpful AI assistant.');
            setUseCustomInstructions(false);
            setConversationTypes([]);
        }
    }, [selectedKbId, kbs]);

    const toggleConversationType = (opt) => {
        setConversationTypes((prev) =>
            prev.includes(opt) ? prev.filter((t) => t !== opt) : [...prev, opt]
        );
    };

    const handleSave = async () => {
        try {
            // 1. Save to backend metadata if a KB is selected
            if (selectedKbId) {
                await setKBMetadata(
                    selectedKbId,
                    kbs.find(kb => kb.id === selectedKbId)?.name || aiName,
                    aiName,
                    systemPrompt,
                    useCustomInstructions,
                    conversationTypes
                );
            }

            // 2. Store in localStorage for current session state
            localStorage.setItem('ai_config', JSON.stringify({
                name: aiName,
                prompt: systemPrompt,
                kbId: selectedKbId,
                useCustomInstructions,
                conversationTypes,
                updated_at: new Date().toISOString()
            }));

            showMessage('AI configuration saved successfully!', 'success');
            onClose();

            if (refreshKBs) {
                await refreshKBs();
            }
        } catch (error) {
            console.error('Error saving AI config:', error);
            showMessage('Failed to save AI configuration', 'error');
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/80 backdrop-blur-sm overflow-y-auto">
            <div className="bg-gray-900 rounded-2xl shadow-2xl w-full max-w-2xl border border-gray-800 my-8 max-h-[90vh] flex flex-col">
                {/* Header */}
                <div className="flex items-center justify-between p-4 sm:p-6 border-b border-gray-800 flex-shrink-0">
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 sm:w-12 sm:h-12 bg-[#04B900]/20 rounded-xl flex items-center justify-center flex-shrink-0">
                            <SparklesIcon className="w-6 h-6 sm:w-7 sm:h-7 text-[#04B900]" />
                        </div>
                        <div className="min-w-0">
                            <h2 className="text-xl sm:text-2xl font-bold text-white truncate">AI Setup</h2>
                            <p className="text-xs sm:text-sm text-gray-400 mt-1">Configure your AI assistant's personality</p>
                        </div>
                    </div>
                    <button
                        onClick={onClose}
                        className="p-2 rounded-lg hover:bg-gray-800 transition-colors flex-shrink-0"
                    >
                        <XMarkIcon className="w-6 h-6 text-white" />
                    </button>
                </div>

                {/* Content - Scrollable */}
                <div className="p-4 sm:p-6 space-y-4 sm:space-y-6 overflow-y-auto flex-1">
                    {/* Knowledge Base Selection */}
                    <div>
                        <label className="block text-sm font-medium text-gray-300 mb-2">
                            Select Knowledge Base
                        </label>
                        <select
                            value={selectedKbId}
                            onChange={(e) => setSelectedKbId(e.target.value)}
                            className="w-full px-4 py-3 bg-black border border-gray-700 rounded-lg focus:ring-2 focus:ring-[#04B900] focus:border-[#04B900] outline-none text-white appearance-none cursor-pointer"
                        >
                            <option value="">Select a Knowledge Base...</option>
                            {kbs.map((kb) => (
                                <option key={kb.id} value={kb.id}>
                                    {kb.name}
                                </option>
                            ))}
                        </select>
                    </div>

                    {/* AI Name */}
                    <div>
                        <label className="block text-sm font-medium text-gray-300 mb-2">
                            Assistant Name
                        </label>
                        <input
                            type="text"
                            value={aiName}
                            onChange={(e) => setAiName(e.target.value)}
                            placeholder="e.g. Jarvis"
                            className="w-full px-4 py-3 bg-black border border-gray-700 rounded-lg focus:ring-2 focus:ring-[#04B900] focus:border-[#04B900] outline-none text-white placeholder-gray-500"
                        />
                    </div>

                    {/* Custom Instructions Toggle */}
                    <div className="flex items-center gap-3">
                        <input
                            type="checkbox"
                            id="customInstructions"
                            checked={useCustomInstructions}
                            onChange={(e) => setUseCustomInstructions(e.target.checked)}
                            className="w-5 h-5 rounded border-gray-700 bg-black text-[#04B900] focus:ring-[#04B900] focus:ring-offset-gray-900"
                        />
                        <label htmlFor="customInstructions" className="text-sm font-medium text-gray-300 cursor-pointer select-none">
                            Add Custom System Instructions
                        </label>
                    </div>

                    {/* System Prompt - Conditional */}
                    {useCustomInstructions && (
                        <div>
                            <label className="block text-sm font-medium text-gray-300 mb-2">
                                System Instructions (Personality)
                            </label>
                            <textarea
                                value={systemPrompt}
                                onChange={(e) => setSystemPrompt(e.target.value)}
                                placeholder="Define how the AI should behave..."
                                rows="6"
                                className="w-full px-4 py-3 bg-black border border-gray-700 rounded-lg focus:ring-2 focus:ring-[#04B900] focus:border-[#04B900] outline-none text-white placeholder-gray-500 resize-none"
                            />
                            <p className="text-xs text-gray-400 mt-1 font-medium">
                                Note: Responses are automatically optimized for brevity (under 200 characters).
                            </p>
                        </div>
                    )}

                    {/* Conversation Type - Multi-select */}
                    <div>
                        <label className="block text-sm font-medium text-gray-300 mb-2">
                            Conversation Type
                        </label>
                        <p className="text-xs text-gray-500 mb-2">Select one or more. These influence how the AI responds.</p>
                        <div className="flex flex-wrap gap-3">
                            {CONVERSATION_TYPE_OPTIONS.map((opt) => (
                                <label
                                    key={opt}
                                    className="flex items-center gap-2 cursor-pointer select-none"
                                >
                                    <input
                                        type="checkbox"
                                        checked={conversationTypes.includes(opt)}
                                        onChange={() => toggleConversationType(opt)}
                                        className="w-4 h-4 rounded border-gray-700 bg-black text-[#04B900] focus:ring-[#04B900] focus:ring-offset-gray-900"
                                    />
                                    <span className="text-sm text-gray-300">{opt}</span>
                                </label>
                            ))}
                        </div>
                    </div>
                </div>

                {/* Footer */}
                <div className="p-4 sm:p-6 border-t border-gray-800 flex justify-end gap-3 flex-shrink-0">
                    <button
                        onClick={onClose}
                        className="px-6 py-2 text-gray-300 border border-gray-700 hover:bg-gray-800 rounded-lg transition-colors"
                    >
                        Cancel
                    </button>
                    <button
                        onClick={handleSave}
                        className="px-6 py-2 bg-[#04B900] text-white rounded-lg hover:bg-[#04B900]/90 transition-colors font-medium"
                    >
                        Save Configuration
                    </button>
                </div>
            </div>
        </div>
    );
};

export default AiSetup;
