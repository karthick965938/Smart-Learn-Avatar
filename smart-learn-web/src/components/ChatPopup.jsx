import React, { useState, useRef, useEffect } from 'react';
import { ChatBubbleLeftRightIcon, PaperAirplaneIcon, XMarkIcon, MinusIcon, UserIcon, CpuChipIcon } from '@heroicons/react/24/solid';
import { queryKB } from '../api';

const ChatPopup = ({ activeKbId, kbName, showMessage }) => {
    const [isOpen, setIsOpen] = useState(false);
    const [isMinimized, setIsMinimized] = useState(false);
    const [query, setQuery] = useState('');
    const [messages, setMessages] = useState([
        { role: 'assistant', content: `Hello! I am your ${kbName || 'AI'} assistant. Ask me anything about your documents.` }
    ]);
    const [loading, setLoading] = useState(false);
    const messagesEndRef = useRef(null);

    const scrollToBottom = () => {
        messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
    };

    useEffect(() => {
        scrollToBottom();
    }, [messages]);

    // Update welcome message when KB changes
    useEffect(() => {
        if (kbName) {
            setMessages([
                { role: 'assistant', content: `Hello! I am your ${kbName} assistant. Ask me anything about your documents.` }
            ]);
        }
    }, [kbName]);

    const handleQuery = async () => {
        if (!query.trim()) return;
        if (!activeKbId) {
            showMessage("Please select a Knowledge Base first.", "error");
            return;
        }

        const userMessage = { role: 'user', content: query };
        setMessages(prev => [...prev, userMessage]);
        setQuery('');
        setLoading(true);

        try {
            const res = await queryKB(activeKbId, userMessage.content);
            const answer = res.data.answer;
            setMessages(prev => [...prev, { role: 'assistant', content: answer }]);
        } catch (error) {
            console.error(error);
            showMessage('Error fetching answer.', 'error');
            setMessages(prev => [...prev, { role: 'assistant', content: "I'm sorry, I encountered an error trying to answer that." }]);
        } finally {
            setLoading(false);
        }
    };

    const handleKeyDown = (e) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            handleQuery();
        }
    };

    return (
        <>
            {/* Floating Chat Button */}
            {!isOpen && (
                <button
                    onClick={() => setIsOpen(true)}
                    className="fixed bottom-6 right-6 z-40 p-4 bg-[#04B900] text-white rounded-full shadow-2xl hover:bg-[#04B900]/90 transition-all transform hover:scale-110 group"
                    title="Open Chat"
                >
                    <ChatBubbleLeftRightIcon className="w-7 h-7" />
                    <div className="absolute -top-1 -right-1 w-3 h-3 bg-[#D24B09] rounded-full animate-pulse"></div>
                </button>
            )}

            {/* Chat Popup */}
            {isOpen && (
                <div
                    className={`fixed bottom-6 right-6 z-50 bg-gray-900 rounded-2xl shadow-2xl border-2 border-[#04B900] transition-all duration-300 ${isMinimized ? 'w-80 h-16' : 'w-96 h-[600px]'
                        } flex flex-col`}
                >
                    {/* Header */}
                    <div className="flex items-center justify-between p-4 border-b border-gray-800 bg-gradient-to-r from-[#04B900] to-[#04B900]/80 rounded-t-2xl">
                        <div className="flex items-center gap-3">
                            <div className="w-10 h-10 rounded-full bg-white/20 flex items-center justify-center">
                                <CpuChipIcon className="w-6 h-6 text-white" />
                            </div>
                            <div>
                                <h3 className="font-semibold text-white">{kbName || 'AI Assistant'}</h3>
                                <p className="text-xs text-white/80 flex items-center gap-1">
                                    <span className="w-2 h-2 rounded-full bg-white"></span>
                                    Online
                                </p>
                            </div>
                        </div>
                        <div className="flex items-center gap-2">
                            <button
                                onClick={() => setIsMinimized(!isMinimized)}
                                className="p-2 rounded-lg hover:bg-white/20 transition-colors"
                                title={isMinimized ? "Maximize" : "Minimize"}
                            >
                                <MinusIcon className="w-5 h-5 text-white" />
                            </button>
                            <button
                                onClick={() => setIsOpen(false)}
                                className="p-2 rounded-lg hover:bg-white/20 transition-colors"
                                title="Close"
                            >
                                <XMarkIcon className="w-5 h-5 text-white" />
                            </button>
                        </div>
                    </div>

                    {/* Messages Area */}
                    {!isMinimized && (
                        <>
                            <div className="flex-1 overflow-y-auto p-4 space-y-4 bg-gray-900">
                                {messages.map((msg, idx) => (
                                    <div key={idx} className={`flex gap-3 ${msg.role === 'user' ? 'flex-row-reverse' : ''}`}>
                                        <div className={`w-8 h-8 rounded-full flex-shrink-0 flex items-center justify-center ${msg.role === 'user' ? 'bg-[#A0CCE5]' : 'bg-[#04B900]'
                                            }`}>
                                            {msg.role === 'user' ?
                                                <UserIcon className="w-5 h-5 text-white" /> :
                                                <CpuChipIcon className="w-5 h-5 text-white" />
                                            }
                                        </div>

                                        <div className={`max-w-[75%] rounded-2xl px-4 py-3 shadow-sm ${msg.role === 'user'
                                                ? 'bg-gray-800 text-white rounded-tr-none'
                                                : 'bg-gray-900 text-white border border-gray-800 rounded-tl-none'
                                            }`}>
                                            <p className="whitespace-pre-wrap leading-relaxed text-sm">
                                                {msg.content}
                                            </p>
                                        </div>
                                    </div>
                                ))}

                                {loading && (
                                    <div className="flex gap-3">
                                        <div className="w-8 h-8 rounded-full bg-[#04B900] flex-shrink-0 flex items-center justify-center">
                                            <CpuChipIcon className="w-5 h-5 text-white" />
                                        </div>
                                        <div className="bg-gray-900 border border-gray-800 rounded-2xl rounded-tl-none px-4 py-3 shadow-sm flex items-center gap-2">
                                            <div className="w-2 h-2 bg-[#04B900] rounded-full animate-bounce"></div>
                                            <div className="w-2 h-2 bg-[#04B900] rounded-full animate-bounce delay-75"></div>
                                            <div className="w-2 h-2 bg-[#04B900] rounded-full animate-bounce delay-150"></div>
                                        </div>
                                    </div>
                                )}
                                <div ref={messagesEndRef} />
                            </div>

                            {/* Input Area */}
                            <div className="p-4 bg-gray-900 border-t border-gray-800 rounded-b-2xl">
                                <div className="relative flex items-center">
                                    <input
                                        type="text"
                                        value={query}
                                        onChange={(e) => setQuery(e.target.value)}
                                        onKeyDown={handleKeyDown}
                                        placeholder="Type your message..."
                                        className="w-full pl-4 pr-12 py-3 bg-black border border-gray-700 rounded-xl focus:ring-2 focus:ring-[#04B900] focus:bg-black focus:border-transparent outline-none transition-all text-white placeholder-gray-500"
                                        disabled={loading}
                                    />
                                    <button
                                        onClick={handleQuery}
                                        disabled={loading || !query.trim()}
                                        className="absolute right-2 p-2 bg-[#04B900] text-white rounded-lg hover:bg-[#04B900]/90 disabled:opacity-50 transition-colors"
                                    >
                                        <PaperAirplaneIcon className="w-5 h-5" />
                                    </button>
                                </div>
                            </div>
                        </>
                    )}
                </div>
            )}
        </>
    );
};

export default ChatPopup;
