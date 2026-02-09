import React, { useState, useRef, useEffect } from 'react';
import { PaperAirplaneIcon, UserIcon, CpuChipIcon } from '@heroicons/react/24/solid';
import { queryKB } from '../api';

const ChatInterface = ({ showMessage, activeKbId }) => {
    const [query, setQuery] = useState('');
    const [messages, setMessages] = useState([
        { role: 'assistant', content: 'Hello! I am your AI assistant. Ask me anything about your documents.' }
    ]);
    const [loading, setLoading] = useState(false);
    const messagesEndRef = useRef(null);

    const scrollToBottom = () => {
        messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
    };

    useEffect(() => {
        scrollToBottom();
    }, [messages]);

    const handleQuery = async () => {
        if (!query.trim()) return;
        if (!activeKbId) {
            showMessage("Please select or create a Knowledge Base first.", "error");
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
        <div className="flex flex-col h-full bg-white shadow-sm border border-gray-200 overflow-hidden max-w-5xl mx-auto w-full">

            {/* Header */}
            <div className="bg-white border-b border-gray-100 p-4 flex items-center gap-3">
                <div className="w-10 h-10 rounded-full bg-blue-100 flex items-center justify-center">
                    <CpuChipIcon className="w-6 h-6 text-blue-600" />
                </div>
                <div>
                    <h2 className="font-semibold text-gray-900">AI Assistant</h2>
                    <p className="text-xs text-green-500 font-medium flex items-center gap-1">
                        <span className="w-2 h-2 rounded-full bg-green-500"></span>
                        Online
                    </p>
                </div>
            </div>

            {/* Messages Area */}
            <div className="flex-1 overflow-y-auto p-4 space-y-6 bg-gray-50/50">
                {messages.map((msg, idx) => (
                    <div key={idx} className={`flex gap-4 ${msg.role === 'user' ? 'flex-row-reverse' : ''}`}>
                        <div className={`w-8 h-8 rounded-full flex-shrink-0 flex items-center justify-center ${msg.role === 'user' ? 'bg-indigo-100' : 'bg-blue-100'
                            }`}>
                            {msg.role === 'user' ?
                                <UserIcon className="w-5 h-5 text-indigo-600" /> :
                                <CpuChipIcon className="w-5 h-5 text-blue-600" />
                            }
                        </div>

                        <div className={`max-w-[80%] rounded-2xl px-5 py-3 shadow-sm ${msg.role === 'user'
                            ? 'bg-indigo-600 text-white rounded-tr-none'
                            : 'bg-white text-gray-800 border border-gray-100 rounded-tl-none'
                            }`}>
                            <p className="whitespace-pre-wrap leading-relaxed text-sm md:text-base">
                                {msg.content}
                            </p>
                        </div>
                    </div>
                ))}

                {loading && (
                    <div className="flex gap-4">
                        <div className="w-8 h-8 rounded-full bg-blue-100 flex-shrink-0 flex items-center justify-center">
                            <CpuChipIcon className="w-5 h-5 text-blue-600" />
                        </div>
                        <div className="bg-white border border-gray-100 rounded-2xl rounded-tl-none px-5 py-3 shadow-sm flex items-center gap-2">
                            <div className="w-2 h-2 bg-blue-400 rounded-full animate-bounce"></div>
                            <div className="w-2 h-2 bg-blue-400 rounded-full animate-bounce delay-75"></div>
                            <div className="w-2 h-2 bg-blue-400 rounded-full animate-bounce delay-150"></div>
                        </div>
                    </div>
                )}
                <div ref={messagesEndRef} />
            </div>

            {/* Input Area */}
            <div className="p-4 bg-white border-t border-gray-100">
                <div className="relative flex items-center">
                    <input
                        type="text"
                        value={query}
                        onChange={(e) => setQuery(e.target.value)}
                        onKeyDown={handleKeyDown}
                        placeholder="Type your message..."
                        className="w-full pl-4 pr-12 py-3 bg-gray-50 border border-gray-200 rounded-xl focus:ring-2 focus:ring-blue-500 focus:bg-white focus:border-transparent outline-none transition-all shadow-sm"
                        disabled={loading}
                    />
                    <button
                        onClick={handleQuery}
                        disabled={loading || !query.trim()}
                        className="absolute right-2 p-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 disabled:opacity-50 disabled:hover:bg-blue-600 transition-colors"
                    >
                        <PaperAirplaneIcon className="w-5 h-5" />
                    </button>
                </div>
                <p className="text-xs text-center text-gray-400 mt-2">
                    AI can make mistakes. Please verify important information.
                </p>
            </div>
        </div>
    );
};

export default ChatInterface;
