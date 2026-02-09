import axios from 'axios';

const API_Base = import.meta.env.VITE_API_BASE_URL || 'http://127.0.0.1:8000';

const api = axios.create({
    baseURL: `${API_Base}/api/v1`,
});

// Use a default KB ID for now since we don't have multi-tenant auth yet
export const DEFAULT_KB_ID = 'default_kb';

export const listKBs = () => api.get('/kbs');

export const createKB = (name) => api.post('/kbs', { name });

export const queryKB = (kb_id, query) => api.post(`/kb/${kb_id}/query`, { query });

export const ingestFile = (kb_id, file) => {
    const formData = new FormData();
    formData.append('file', file);
    return api.post(`/kb/${kb_id}/ingest`, formData, {
        headers: {
            'Content-Type': 'multipart/form-data',
        },
    });
};

export const ingestUrl = (kb_id, url) => api.post(`/kb/${kb_id}/ingest/url`, { url });

export const listDocuments = (kb_id) => api.get(`/kb/${kb_id}/documents`);

export const deleteDocument = (kb_id, filename) => api.delete(`/kb/${kb_id}/documents`, { params: { filename } });

export const setKBMetadata = (kb_id, name, assistant_name = "", instruction = "", custom_instruction = false, conversation_types = []) =>
    api.post(`/kb/${kb_id}`, {
        name,
        assistant_name,
        instruction,
        custom_instruction,
        conversation_types: Array.isArray(conversation_types) ? conversation_types : []
    });

export const deleteKB = (kb_id) => api.delete(`/kb/${kb_id}`);

export const generateNvs = (config) => api.post('/iot/generate-nvs', config, {
    responseType: 'arraybuffer'
});

export default api;
