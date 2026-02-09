import React, { useEffect } from 'react';
import { CheckCircleIcon, XCircleIcon, InformationCircleIcon, XMarkIcon } from '@heroicons/react/24/solid';

const FlashMessage = ({ message, type, onClose }) => {
    if (!message) return null;

    const variants = {
        success: {
            bg: 'bg-[#04B900]/20',
            border: 'border-[#04B900]',
            text: 'text-white',
            icon: <CheckCircleIcon className="w-5 h-5 text-[#04B900]" />
        },
        error: {
            bg: 'bg-red-900/30',
            border: 'border-red-500',
            text: 'text-white',
            icon: <XCircleIcon className="w-5 h-5 text-red-500" />
        },
        info: {
            bg: 'bg-blue-900/30',
            border: 'border-blue-400',
            text: 'text-white',
            icon: <InformationCircleIcon className="w-5 h-5 text-blue-400" />
        }
    };

    const currentVariant = variants[type] || variants.info;

    return (
        <div
            className="fixed top-[110px] right-6 animate-fade-in w-full max-w-sm px-4"
            style={{ zIndex: 99999 }}
        >
            <div className={`flex items-center gap-3 p-4 rounded-xl border shadow-lg ${currentVariant.bg} ${currentVariant.border}`}>
                <div className="flex-shrink-0">
                    {currentVariant.icon}
                </div>
                <p className={`flex-1 text-sm font-medium ${currentVariant.text}`}>
                    {message}
                </p>
                <button
                    onClick={onClose}
                    className={`flex-shrink-0 p-1 rounded-lg hover:bg-black/5 transition-colors ${currentVariant.text}`}
                >
                    <XMarkIcon className="w-4 h-4" />
                </button>
            </div>
        </div>
    );
};

export default FlashMessage;
