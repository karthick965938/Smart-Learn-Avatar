import React, { useState } from 'react';
import { ChatBubbleLeftRightIcon, DocumentTextIcon, Cog6ToothIcon, FolderIcon, PlusIcon, TrashIcon } from '@heroicons/react/24/outline';
import Modal from './Modal';
import { createKB, deleteKB } from '../api';

const Sidebar = ({ activeTab, onTabChange, kbs, activeKbId, onKbChange, onKbCreate, onDeleteKb }) => {
    const [isCreateModalOpen, setIsCreateModalOpen] = useState(false);
    const [newKbName, setNewKbName] = useState('');

    const menuItems = [
        { id: 'chat', label: 'Chat Assistant', icon: ChatBubbleLeftRightIcon },
        { id: 'documents', label: 'Documents', icon: DocumentTextIcon },
        // { id: 'settings', label: 'Settings', icon: Cog6ToothIcon },
    ];

    const handleCreate = async (e) => {
        e.preventDefault();
        if (!newKbName.trim()) return;
        await onKbCreate(newKbName);
        setNewKbName('');
        setIsCreateModalOpen(false);
    };

    return (
        <>
            <div className="w-64 bg-white border-r border-gray-200 h-full flex flex-col hidden md:flex shrink-0">
                <div className="p-6 border-b border-gray-100">
                    <h1 className="text-2xl font-bold bg-gradient-to-r from-blue-600 to-indigo-600 bg-clip-text text-transparent">
                        SmartBase
                    </h1>
                    <p className="text-xs text-gray-500 mt-1">AI Knowledge Assistant</p>
                </div>

                {/* KB Switcher */}
                <div className="px-4 py-4">
                    <div className="flex items-center justify-between mb-2">
                        <h3 className="text-xs font-semibold text-gray-400 uppercase tracking-wider">Knowledge Bases</h3>
                        <button
                            onClick={() => setIsCreateModalOpen(true)}
                            className="text-blue-600 hover:text-blue-700 p-1 rounded hover:bg-blue-50 transition-colors"
                            title="Create New KB"
                        >
                            <PlusIcon className="w-4 h-4" />
                        </button>
                    </div>

                    <div className="space-y-1 max-h-40 overflow-y-auto custom-scrollbar">
                        {kbs.map((kb) => (
                            <div
                                key={kb.id}
                                className={`group flex items-center justify-between px-3 py-2 rounded-lg text-sm transition-all cursor-pointer ${activeKbId === kb.id
                                        ? 'bg-blue-50 text-blue-700 font-medium'
                                        : 'text-gray-600 hover:bg-gray-50'
                                    }`}
                                onClick={() => onKbChange(kb.id)}
                            >
                                <div className="flex items-center gap-2 truncate">
                                    <FolderIcon className={`w-4 h-4 ${activeKbId === kb.id ? 'text-blue-500' : 'text-gray-400'}`} />
                                    <span className="truncate">{kb.name}</span>
                                </div>
                                {kbs.length > 1 && (
                                    <button
                                        onClick={(e) => {
                                            e.stopPropagation();
                                            onDeleteKb(kb.id);
                                        }}
                                        className="opacity-0 group-hover:opacity-100 p-1 text-gray-400 hover:text-red-500 hover:bg-red-50 rounded transition-all"
                                    >
                                        <TrashIcon className="w-3 h-3" />
                                    </button>
                                )}
                            </div>
                        ))}

                        {kbs.length === 0 && (
                            <div className="text-xs text-center text-gray-400 py-2 italic">
                                No KBs found. Create one!
                            </div>
                        )}
                    </div>
                </div>

                <div className="border-t border-gray-100 my-2"></div>

                <nav className="flex-1 px-4 space-y-2 py-2">
                    {menuItems.map((item) => {
                        const Icon = item.icon;
                        const isActive = activeTab === item.id;
                        return (
                            <button
                                key={item.id}
                                onClick={() => onTabChange(item.id)}
                                className={`w-full flex items-center gap-3 px-4 py-3 text-sm font-medium rounded-xl transition-all duration-200 ${isActive
                                        ? 'bg-blue-50 text-blue-600 shadow-sm'
                                        : 'text-gray-600 hover:bg-gray-50 hover:text-gray-900'
                                    }`}
                            >
                                <Icon className={`w-5 h-5 ${isActive ? 'text-blue-600' : 'text-gray-400'}`} />
                                {item.label}
                            </button>
                        );
                    })}
                </nav>

                <div className="p-4 border-t border-gray-100 mt-auto">
                    <div className="flex items-center gap-3 px-4 py-3 rounded-xl bg-gray-50">
                        <div className="w-8 h-8 rounded-full bg-gradient-to-tr from-blue-500 to-indigo-500 flex items-center justify-center text-white font-bold text-xs">
                            SB
                        </div>
                        <div className="flex-1 min-w-0">
                            <p className="text-sm font-medium text-gray-900 truncate">Demo User</p>
                            <p className="text-xs text-gray-500 truncate">Free Plan</p>
                        </div>
                    </div>
                </div>
            </div>

            {/* Create KB Modal */}
            <Modal
                isOpen={isCreateModalOpen}
                onClose={() => setIsCreateModalOpen(false)}
                title="Create Knowledge Base"
            >
                <form onSubmit={handleCreate} className="space-y-4">
                    <div>
                        <label className="block text-sm font-medium text-gray-700 mb-1">Name</label>
                        <input
                            type="text"
                            value={newKbName}
                            onChange={(e) => setNewKbName(e.target.value)}
                            placeholder="e.g. My Project Docs"
                            className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 outline-none"
                            autoFocus
                        />
                    </div>
                    <div className="flex justify-end gap-2">
                        <button
                            type="button"
                            onClick={() => setIsCreateModalOpen(false)}
                            className="px-4 py-2 text-gray-600 hover:bg-gray-100 rounded-lg"
                        >
                            Cancel
                        </button>
                        <button
                            type="submit"
                            disabled={!newKbName.trim()}
                            className="px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 disabled:opacity-50"
                        >
                            Create
                        </button>
                    </div>
                </form>
            </Modal>
        </>
    );
};

export default Sidebar;
