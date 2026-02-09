import React, { useState } from 'react';
import { uploadFile, uploadUrl } from '../api';

const UploadSection = ({ onUploadSuccess, showMessage }) => {
    const [file, setFile] = useState(null);
    const [url, setUrl] = useState('');
    const [kbName, setKbName] = useState('');
    const [loading, setLoading] = useState(false);
    const [uploadType, setUploadType] = useState('file'); // 'file' or 'url'

    const handleFileChange = (e) => setFile(e.target.files[0]);

    const handleUpload = async () => {
        setLoading(true);
        try {
            if (uploadType === 'file' && file) {
                const formData = new FormData();
                formData.append('file', file);
                await uploadFile(formData);
            } else if (uploadType === 'url' && url) {
                await uploadUrl({ url, name: kbName });
            }
            onUploadSuccess();
            setFile(null);
            setUrl('');
            setKbName('');
            showMessage('Knowledge Base Created Successfully!', 'success');
        } catch (error) {
            console.error(error);
            showMessage('Upload Failed: ' + (error.response?.data?.error || error.message), 'error');
        } finally {
            setLoading(false);
        }
    };

    return (
        <div className="bg-white p-6 rounded-lg shadow-md mb-6">
            <h2 className="text-xl font-bold mb-4">Create Knowledge Base</h2>

            <div className="flex gap-4 mb-4">
                <button
                    className={`px-4 py-2 rounded ${uploadType === 'file' ? 'bg-blue-600 text-white' : 'bg-gray-200'}`}
                    onClick={() => setUploadType('file')}
                >
                    Upload File
                </button>
                <button
                    className={`px-4 py-2 rounded ${uploadType === 'url' ? 'bg-blue-600 text-white' : 'bg-gray-200'}`}
                    onClick={() => setUploadType('url')}
                >
                    Add URL
                </button>
            </div>

            {uploadType === 'file' && (
                <div className="mb-4">
                    <input
                        type="file"
                        onChange={handleFileChange}
                        className="block w-full text-sm text-gray-500
                        file:mr-4 file:py-2 file:px-4
                        file:rounded-full file:border-0
                        file:text-sm file:font-semibold
                        file:bg-blue-50 file:text-blue-700
                        hover:file:bg-blue-100"
                    />
                </div>
            )}

            {uploadType === 'url' && (
                <div className="mb-4 space-y-2">
                    <input
                        type="text"
                        placeholder="Enter URL"
                        value={url}
                        onChange={(e) => setUrl(e.target.value)}
                        className="w-full p-2 border rounded"
                    />
                    <input
                        type="text"
                        placeholder="KB Name (optional)"
                        value={kbName}
                        onChange={(e) => setKbName(e.target.value)}
                        className="w-full p-2 border rounded"
                    />
                </div>
            )}

            <button
                onClick={handleUpload}
                disabled={loading}
                className="w-full bg-green-600 text-white py-2 rounded hover:bg-green-700 disabled:bg-gray-400"
            >
                {loading ? 'Processing...' : 'Create KB'}
            </button>
        </div>
    );
};

export default UploadSection;
