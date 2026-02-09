import React, { useState, useEffect } from 'react';
import { FolderIcon, TrashIcon, DocumentTextIcon } from '@heroicons/react/24/outline';
import { listDocuments } from '../api';

const KnowledgeBaseCard = ({ kb, onSelect, onViewDocs, onDelete, isActive }) => {
    const [isHovered, setIsHovered] = useState(false);
    const docCount = kb.document_count || 0;

    return (
        <div
            className="relative group"
            onMouseEnter={() => setIsHovered(true)}
            onMouseLeave={() => setIsHovered(false)}
        >
            <div
                onClick={() => onSelect(kb)}
                className={`
                    relative overflow-hidden rounded-2xl p-6 cursor-pointer
                    transition-all duration-300 transform
                    ${isActive ? 'ring-2 ring-[#04B900] shadow-lg shadow-[#04B900]/20 scale-105' : 'shadow-md hover:shadow-xl hover:shadow-gray-900/50 hover:scale-105'}
                    bg-gray-900 border-2 ${isActive ? 'border-[#04B900]' : 'border-gray-800'}
                `}
            >
                {/* Background gradient overlay */}
                <div className={`absolute inset-0 bg-gradient-to-br from-[#04B900]/10 to-transparent opacity-0 group-hover:opacity-100 transition-opacity duration-300`}></div>

                {/* Content */}
                <div className="relative z-10">
                    {/* Icon */}
                    <div className={`w-16 h-16 rounded-xl flex items-center justify-center mb-4 transition-colors duration-300 ${isActive ? 'bg-[#04B900]' : 'bg-gray-800 group-hover:bg-[#04B900]'
                        }`}>
                        <FolderIcon className="w-8 h-8 text-white" />
                    </div>

                    {/* KB Name */}
                    <h3 className="text-lg font-bold text-white mb-2 truncate">
                        {kb.name}
                    </h3>

                    {/* Document Count */}
                    <div className="flex items-center gap-2 text-sm text-gray-400">
                        <DocumentTextIcon className="w-4 h-4" />
                        <span>{docCount} {docCount === 1 ? 'document' : 'documents'}</span>
                    </div>
                </div>

                {/* Active indicator - hidden when actions are shown */}
                {isActive && !isHovered && (
                    <div className="absolute top-3 right-3">
                        <div className="w-3 h-3 rounded-full bg-[#04B900] animate-pulse"></div>
                    </div>
                )}
            </div>

            {/* Actions overlay - shows on hover */}
            {isHovered && (
                <div className="absolute top-3 right-3 z-20 flex items-center gap-2 opacity-0 group-hover:opacity-100 transition-opacity duration-300">
                    <button
                        onClick={(e) => {
                            e.stopPropagation();
                            onViewDocs(kb);
                        }}
                        className="p-2 rounded-lg bg-gray-800 text-white hover:bg-gray-700 border border-gray-700 transition-colors"
                        title="View Documents"
                    >
                        <DocumentTextIcon className="w-4 h-4" />
                    </button>
                    <button
                        onClick={(e) => {
                            e.stopPropagation();
                            onDelete(kb.id);
                        }}
                        className="p-2 rounded-lg bg-[#D24B09] text-white hover:bg-[#D24B09]/90 transition-colors"
                        title="Delete Knowledge Base"
                    >
                        <TrashIcon className="w-4 h-4" />
                    </button>
                </div>
            )}
        </div>
    );
};

export default KnowledgeBaseCard;
