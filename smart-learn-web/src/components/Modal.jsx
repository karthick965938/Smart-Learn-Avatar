import React from 'react';
import { XMarkIcon } from '@heroicons/react/24/outline';

const Modal = ({ isOpen, onClose, title, children }) => {
    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center overflow-y-auto overflow-x-hidden bg-black/50 backdrop-blur-sm p-4 md:p-0">
            <div className="relative w-full max-w-lg max-h-full rounded-2xl bg-white shadow-2xl ring-1 ring-gray-200 transaction-all duration-300 transform scale-100">

                {/* Header */}
                <div className="flex items-center justify-between border-b border-gray-100 p-5">
                    <h3 className="text-xl font-semibold text-gray-900">
                        {title}
                    </h3>
                    <button
                        onClick={onClose}
                        type="button"
                        className="text-gray-400 bg-transparent hover:bg-gray-100 hover:text-gray-900 rounded-lg text-sm w-8 h-8 ms-auto inline-flex justify-center items-center backdrop:transition-colors"
                    >
                        <XMarkIcon className="w-5 h-5" />
                        <span className="sr-only">Close modal</span>
                    </button>
                </div>

                {/* Body */}
                <div className="p-6 space-y-6">
                    {children}
                </div>
            </div>
        </div>
    );
};

export default Modal;
