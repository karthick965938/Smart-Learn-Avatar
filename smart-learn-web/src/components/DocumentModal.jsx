import React, { useState, useEffect } from 'react';
import { XMarkIcon, PlusIcon, TrashIcon, DocumentIcon, LinkIcon } from '@heroicons/react/24/outline';
import { listDocuments, ingestFile, ingestUrl, deleteDocument } from '../api';

const DocumentModal = ({ isOpen, onClose, kbId, kbName, showMessage, onDocumentsChange }) => {
    const [documents, setDocuments] = useState([]);
    const [loading, setLoading] = useState(false);
    const [isUploadModalOpen, setIsUploadModalOpen] = useState(false);
    const [isUrlModalOpen, setIsUrlModalOpen] = useState(false);
    const [uploadFile, setUploadFile] = useState(null);
    const [urlInput, setUrlInput] = useState('');

    const fetchDocuments = async () => {
        if (!kbId) return;
        setLoading(true);
        try {
            const res = await listDocuments(kbId);
            const docs = res.data || [];
            setDocuments(docs);
            // Notify parent if count changed or as a general refresh
            if (onDocumentsChange) {
                onDocumentsChange();
            }
        } catch (error) {
            console.error("Error fetching docs:", error);
            showMessage("Failed to load documents", "error");
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        if (isOpen && kbId) {
            fetchDocuments();
        }
    }, [isOpen, kbId]);

    const handleFileUpload = async (e) => {
        e.preventDefault();
        if (!uploadFile) return;

        try {
            showMessage("Uploading file...", "info");
            await ingestFile(kbId, uploadFile);
            showMessage("File uploaded successfully. Processing started.", "success");
            setIsUploadModalOpen(false);
            setUploadFile(null);
            // Poll for updates
            setTimeout(() => fetchDocuments(), 1000);
            setTimeout(() => fetchDocuments(), 3000);
            setTimeout(() => fetchDocuments(), 5000);
            setTimeout(() => fetchDocuments(), 8000);
        } catch (error) {
            console.error(error);
            showMessage(error.response?.data?.detail || "Upload failed", "error");
        }
    };

    const handleUrlUpload = async (e) => {
        e.preventDefault();
        if (!urlInput) return;

        try {
            showMessage("Ingesting URL...", "info");
            await ingestUrl(kbId, urlInput);
            showMessage("URL added successfully. Processing started.", "success");
            setIsUrlModalOpen(false);
            setUrlInput('');
            // Poll for updates
            setTimeout(() => fetchDocuments(), 1000);
            setTimeout(() => fetchDocuments(), 3000);
            setTimeout(() => fetchDocuments(), 5000);
            setTimeout(() => fetchDocuments(), 8000);
        } catch (error) {
            console.error(error);
            showMessage(error.response?.data?.detail || "URL ingestion failed", "error");
        }
    };

    const handleDelete = async (filename) => {
        if (!confirm(`Are you sure you want to delete ${filename}?`)) return;

        try {
            await deleteDocument(kbId, filename);
            showMessage("Document deleted", "success");
            fetchDocuments();
        } catch (error) {
            console.error(error);
            showMessage(error.response?.data?.detail || "Delete failed", "error");
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/80 backdrop-blur-sm">
            <div className="bg-gray-900 rounded-2xl shadow-2xl w-full max-w-4xl max-h-[90vh] flex flex-col border border-gray-800">
                {/* Header */}
                <div className="flex items-center justify-between p-6 border-b border-gray-800">
                    <div>
                        <h2 className="text-2xl font-bold text-white">{kbName}</h2>
                        <p className="text-sm text-gray-400 mt-1">Manage documents and sources</p>
                    </div>
                    <button
                        onClick={onClose}
                        className="p-2 rounded-lg hover:bg-gray-800 transition-colors"
                    >
                        <XMarkIcon className="w-6 h-6 text-white" />
                    </button>
                </div>

                {/* Action Buttons */}
                <div className="flex gap-3 p-6 border-b border-gray-800">
                    <button
                        onClick={() => setIsUrlModalOpen(true)}
                        className="flex items-center gap-2 px-4 py-2 bg-gray-800 text-white rounded-lg hover:bg-gray-700 transition-colors font-medium"
                    >
                        <LinkIcon className="w-5 h-5" />
                        Add URL
                    </button>
                    <button
                        onClick={() => setIsUploadModalOpen(true)}
                        className="flex items-center gap-2 px-4 py-2 bg-[#04B900] text-white rounded-lg hover:bg-[#04B900]/90 transition-colors font-medium"
                    >
                        <PlusIcon className="w-5 h-5" />
                        Upload File
                    </button>
                </div>

                {/* Document List */}
                <div className="flex-1 overflow-y-auto p-6">
                    {loading && <div className="text-center text-gray-400">Loading documents...</div>}

                    {!loading && documents.length === 0 && (
                        <div className="flex flex-col items-center justify-center h-full text-center">
                            <div className="w-16 h-16 bg-gray-100 rounded-full flex items-center justify-center mb-4">
                                <DocumentIcon className="w-8 h-8 text-gray-400" />
                            </div>
                            <h3 className="text-lg font-medium text-white">No documents yet</h3>
                            <p className="text-gray-400 max-w-sm mt-2">
                                Upload PDF, TXT files or add URLs to train your AI assistant.
                            </p>
                        </div>
                    )}

                    {!loading && documents.length > 0 && (
                        <div className="space-y-2">
                            {documents.map((doc, idx) => (
                                <div
                                    key={idx}
                                    className="flex items-center justify-between p-4 bg-gray-800 rounded-lg hover:bg-gray-700 transition-colors group"
                                >
                                    <div className="flex items-center gap-3 flex-1 min-w-0">
                                        <div className="p-2 bg-[#A0CCE5]/20 rounded-lg">
                                            {doc.startsWith('http') ? (
                                                <LinkIcon className="w-5 h-5 text-[#A0CCE5]" />
                                            ) : (
                                                <DocumentIcon className="w-5 h-5 text-[#A0CCE5]" />
                                            )}
                                        </div>
                                        <span className="font-medium text-white truncate" title={doc}>
                                            {doc}
                                        </span>
                                    </div>
                                    <button
                                        onClick={() => handleDelete(doc)}
                                        className="p-2 text-gray-400 hover:text-white hover:bg-[#D24B09] rounded-lg transition-all opacity-0 group-hover:opacity-100"
                                        title="Delete"
                                    >
                                        <TrashIcon className="w-5 h-5" />
                                    </button>
                                </div>
                            ))}
                        </div>
                    )}
                </div>

                {/* Upload File Modal */}
                {isUploadModalOpen && (
                    <div className="absolute inset-0 bg-black/80 backdrop-blur-sm rounded-2xl flex items-center justify-center p-4">
                        <div className="bg-gray-900 rounded-xl p-6 w-full max-w-md border border-gray-800">
                            <h3 className="text-xl font-bold text-white mb-4">Upload Document</h3>
                            <form onSubmit={handleFileUpload} className="space-y-4">
                                <div className="border-2 border-dashed border-gray-300 rounded-xl p-8 text-center hover:border-[#04B900] transition-colors cursor-pointer relative group">
                                    <input
                                        type="file"
                                        onChange={(e) => setUploadFile(e.target.files[0])}
                                        className="absolute inset-0 w-full h-full opacity-0 cursor-pointer"
                                        required
                                    />
                                    <DocumentIcon className="w-10 h-10 text-gray-400 mx-auto mb-3 group-hover:text-[#04B900] transition-colors" />
                                    <p className="text-sm font-medium text-white">
                                        {uploadFile ? uploadFile.name : "Click to select or drag file here"}
                                    </p>
                                    <p className="text-xs text-gray-500 mt-1">PDF, TXT, DOCX up to 10MB</p>
                                </div>
                                <div className="flex justify-end gap-2">
                                    <button
                                        type="button"
                                        onClick={() => setIsUploadModalOpen(false)}
                                        className="px-4 py-2 text-gray-300 border border-gray-700 hover:bg-gray-800 rounded-lg transition-colors"
                                    >
                                        Cancel
                                    </button>
                                    <button
                                        type="submit"
                                        disabled={!uploadFile}
                                        className="px-4 py-2 bg-[#04B900] text-white rounded-lg hover:bg-[#04B900]/90 disabled:opacity-50"
                                    >
                                        Upload
                                    </button>
                                </div>
                            </form>
                        </div>
                    </div>
                )}

                {/* URL Modal */}
                {isUrlModalOpen && (
                    <div className="absolute inset-0 bg-black/80 backdrop-blur-sm rounded-2xl flex items-center justify-center p-4">
                        <div className="bg-gray-900 rounded-xl p-6 w-full max-w-md border border-gray-800">
                            <h3 className="text-xl font-bold text-white mb-4">Ingest from URL</h3>
                            <form onSubmit={handleUrlUpload} className="space-y-4">
                                <div>
                                    <label className="block text-sm font-medium text-gray-300 mb-1">Website URL</label>
                                    <input
                                        type="url"
                                        value={urlInput}
                                        onChange={(e) => setUrlInput(e.target.value)}
                                        placeholder="https://example.com/article"
                                        className="w-full px-4 py-2 bg-black border border-gray-700 rounded-lg focus:ring-2 focus:ring-[#04B900] focus:border-[#04B900] outline-none text-white placeholder-gray-500"
                                        required
                                    />
                                </div>
                                <div className="flex justify-end gap-2">
                                    <button
                                        type="button"
                                        onClick={() => setIsUrlModalOpen(false)}
                                        className="px-4 py-2 text-gray-300 border border-gray-700 hover:bg-gray-800 rounded-lg transition-colors"
                                    >
                                        Cancel
                                    </button>
                                    <button
                                        type="submit"
                                        disabled={!urlInput}
                                        className="px-4 py-2 bg-[#04B900] text-white rounded-lg hover:bg-[#04B900]/90 disabled:opacity-50"
                                    >
                                        Add URL
                                    </button>
                                </div>
                            </form>
                        </div>
                    </div>
                )}
            </div>
        </div>
    );
};

export default DocumentModal;
