import React, { useState, useEffect } from 'react';
import { PlusIcon, TrashIcon, DocumentIcon, LinkIcon } from '@heroicons/react/24/outline';
import { listDocuments, ingestFile, ingestUrl, deleteDocument, DEFAULT_KB_ID } from '../api';
import Modal from './Modal';

const DocumentManager = ({ showMessage, activeKbId }) => {
    const [documents, setDocuments] = useState([]);
    const [loading, setLoading] = useState(false);
    const [isUploadModalOpen, setIsUploadModalOpen] = useState(false);
    const [isUrlModalOpen, setIsUrlModalOpen] = useState(false);

    // Form states
    const [uploadFile, setUploadFile] = useState(null);
    const [urlInput, setUrlInput] = useState('');

    const fetchDocuments = async () => {
        if (!activeKbId) return;
        setLoading(true);
        try {
            const res = await listDocuments(activeKbId);
            // API returns list of strings (filenames/sources)
            setDocuments(res.data || []);
        } catch (error) {
            console.error("Error fetching docs:", error);
            showMessage("Failed to load documents", "error");
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        fetchDocuments();
    }, [activeKbId]);

    const handleFileUpload = async (e) => {
        e.preventDefault();
        if (!uploadFile) return;

        try {
            showMessage("Uploading file...", "info");
            await ingestFile(activeKbId, uploadFile);
            showMessage("File uploaded successfully. Processing started.", "success");
            setIsUploadModalOpen(false);
            setUploadFile(null);
            // Poll for document list updates (backend processes asynchronously)
            // Check at 1s, 3s, 5s, and 8s
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
            await ingestUrl(activeKbId, urlInput);
            showMessage("URL added successfully. Processing started.", "success");
            setIsUrlModalOpen(false);
            setUrlInput('');
            // Poll for document list updates (backend processes asynchronously)
            // Check at 1s, 3s, 5s, and 8s
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
            await deleteDocument(activeKbId, filename);
            showMessage("Document deleted", "success");
            fetchDocuments();
        } catch (error) {
            console.error(error);
            showMessage(error.response?.data?.detail || "Delete failed", "error");
        }
    };

    return (
        <div className="h-full flex flex-col p-8 max-w-5xl mx-auto w-full">
            <div className="flex items-center justify-between mb-8">
                <div>
                    <h2 className="text-2xl font-bold text-gray-900">Knowledge Base</h2>
                    <p className="text-gray-500 mt-1">Manage documents and sources for your AI</p>
                </div>
                <div className="flex gap-3">
                    <button
                        onClick={() => setIsUrlModalOpen(true)}
                        className="flex items-center gap-2 px-4 py-2 bg-white border border-gray-300 text-gray-700 rounded-lg hover:bg-gray-50 transition-colors shadow-sm font-medium"
                    >
                        <LinkIcon className="w-5 h-5" />
                        Add URL
                    </button>
                    <button
                        onClick={() => setIsUploadModalOpen(true)}
                        className="flex items-center gap-2 px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 transition-colors shadow-sm font-medium"
                    >
                        <PlusIcon className="w-5 h-5" />
                        Upload File
                    </button>
                </div>
            </div>

            <div className="flex-1 bg-white rounded-2xl shadow-sm border border-gray-200 overflow-hidden">
                {loading && <div className="p-8 text-center text-gray-500">Loading documents...</div>}

                {!loading && documents.length === 0 && (
                    <div className="h-full flex flex-col items-center justify-center p-8 text-center">
                        <div className="w-16 h-16 bg-gray-100 rounded-full flex items-center justify-center mb-4">
                            <DocumentIcon className="w-8 h-8 text-gray-400" />
                        </div>
                        <h3 className="text-lg font-medium text-gray-900">No documents yet</h3>
                        <p className="text-gray-500 max-w-sm mt-2">
                            Upload PDF, TXT files or add URLs to train your AI assistant using the buttons above.
                        </p>
                    </div>
                )}

                {!loading && documents.length > 0 && (
                    <div className="overflow-auto h-full">
                        <table className="w-full text-left">
                            <thead className="bg-gray-50 border-b border-gray-200">
                                <tr>
                                    <th className="px-6 py-4 text-xs font-semibold text-gray-500 uppercase tracking-wider">Name/Source</th>
                                    <th className="px-6 py-4 text-xs font-semibold text-gray-500 uppercase tracking-wider text-right">Actions</th>
                                </tr>
                            </thead>
                            <tbody className="divide-y divide-gray-100">
                                {documents.map((doc, idx) => (
                                    <tr key={idx} className="hover:bg-gray-50 transition-colors">
                                        <td className="px-6 py-4">
                                            <div className="flex items-center gap-3">
                                                <div className="p-2 bg-blue-50 rounded-lg">
                                                    {doc.startsWith('http') ? (
                                                        <LinkIcon className="w-5 h-5 text-blue-600" />
                                                    ) : (
                                                        <DocumentIcon className="w-5 h-5 text-blue-600" />
                                                    )}
                                                </div>
                                                <span className="font-medium text-gray-700 truncate max-w-md block" title={doc}>
                                                    {doc}
                                                </span>
                                            </div>
                                        </td>
                                        <td className="px-6 py-4 text-right">
                                            <button
                                                onClick={() => handleDelete(doc)}
                                                className="p-2 text-gray-400 hover:text-red-600 hover:bg-red-50 rounded-lg transition-colors"
                                                title="Delete"
                                            >
                                                <TrashIcon className="w-5 h-5" />
                                            </button>
                                        </td>
                                    </tr>
                                ))}
                            </tbody>
                        </table>
                    </div>
                )}
            </div>

            {/* Upload Modal */}
            <Modal
                isOpen={isUploadModalOpen}
                onClose={() => setIsUploadModalOpen(false)}
                title="Upload Document"
            >
                <form onSubmit={handleFileUpload} className="space-y-4">
                    <div className="border-2 border-dashed border-gray-300 rounded-xl p-8 text-center hover:border-blue-500 transition-colors cursor-pointer relative group">
                        <input
                            type="file"
                            onChange={(e) => setUploadFile(e.target.files[0])}
                            className="absolute inset-0 w-full h-full opacity-0 cursor-pointer"
                            required
                        />
                        <div className="pointer-events-none">
                            <DocumentIcon className="w-10 h-10 text-gray-400 mx-auto mb-3 group-hover:text-blue-500 transition-colors" />
                            <p className="text-sm font-medium text-gray-900">
                                {uploadFile ? uploadFile.name : "Click to select or drag file here"}
                            </p>
                            <p className="text-xs text-gray-500 mt-1">PDF, TXT, MD up to 10MB</p>
                        </div>
                    </div>
                    <div className="flex justify-end gap-3 pt-4">
                        <button
                            type="button"
                            onClick={() => setIsUploadModalOpen(false)}
                            className="px-4 py-2 text-sm font-medium text-gray-700 hover:bg-gray-50 rounded-lg"
                        >
                            Cancel
                        </button>
                        <button
                            type="submit"
                            disabled={!uploadFile}
                            className="px-4 py-2 text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 rounded-lg disabled:opacity-50 disabled:cursor-not-allowed"
                        >
                            Upload
                        </button>
                    </div>
                </form>
            </Modal>

            {/* URL Modal */}
            <Modal
                isOpen={isUrlModalOpen}
                onClose={() => setIsUrlModalOpen(false)}
                title="Ingest from URL"
            >
                <form onSubmit={handleUrlUpload} className="space-y-4">
                    <div>
                        <label className="block text-sm font-medium text-gray-700 mb-1">Website URL</label>
                        <input
                            type="url"
                            value={urlInput}
                            onChange={(e) => setUrlInput(e.target.value)}
                            placeholder="https://example.com/article"
                            className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-blue-500 outline-none transition-shadow"
                            required
                        />
                    </div>
                    <div className="flex justify-end gap-3 pt-4">
                        <button
                            type="button"
                            onClick={() => setIsUrlModalOpen(false)}
                            className="px-4 py-2 text-sm font-medium text-gray-700 hover:bg-gray-50 rounded-lg"
                        >
                            Cancel
                        </button>
                        <button
                            type="submit"
                            disabled={!urlInput}
                            className="px-4 py-2 text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 rounded-lg disabled:opacity-50 disabled:cursor-not-allowed"
                        >
                            Add URL
                        </button>
                    </div>
                </form>
            </Modal>
        </div>
    );
};

export default DocumentManager;
