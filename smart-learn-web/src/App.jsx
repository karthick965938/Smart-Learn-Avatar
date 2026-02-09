import React, { useState, useEffect } from 'react';
import { PlusIcon, SparklesIcon, CpuChipIcon, WrenchScrewdriverIcon } from '@heroicons/react/24/outline';
import KnowledgeBaseCard from './components/KnowledgeBaseCard';
import DocumentModal from './components/DocumentModal';
import ChatPopup from './components/ChatPopup';
import FlashMessage from './components/FlashMessage';
import AiSetup from './components/AiSetup';
import IoTSetup from './components/IoTSetup';
import { listKBs, createKB, deleteKB } from './api';

function App() {
  const [flash, setFlash] = useState({ message: '', type: '' });
  const [kbs, setKbs] = useState([]);
  const [activeKbId, setActiveKbId] = useState(null);
  const [selectedKb, setSelectedKb] = useState(null);
  const [isDocModalOpen, setIsDocModalOpen] = useState(false);
  const [newKbName, setNewKbName] = useState('');
  const [isCreateModalOpen, setIsCreateModalOpen] = useState(false);
  const [isAiSetupOpen, setIsAiSetupOpen] = useState(false);
  const [isIoTSetupOpen, setIsIoTSetupOpen] = useState(false);

  const showMessage = (message, type = 'success') => {
    setFlash({ message, type });
    if (type === 'success' || type === 'info') {
      setTimeout(() => setFlash({ message: '', type: '' }), 3000);
    }
  };

  const closeFlash = () => setFlash({ message: '', type: '' });

  const fetchKBs = async () => {
    try {
      const res = await listKBs();
      const fetchedKbs = res.data || [];
      setKbs(fetchedKbs);

      if (fetchedKbs.length > 0 && !activeKbId) {
        setActiveKbId(fetchedKbs[0].id);
      } else if (fetchedKbs.length === 0) {
        setActiveKbId(null);
      }
    } catch (error) {
      console.error("Error fetching KBs:", error);
      showMessage("Failed to load Knowledge Bases", "error");
    }
  };

  const handleCreateKB = async (name) => {
    try {
      const res = await createKB(name);
      const newKb = res.data;
      setKbs(prev => [...prev, newKb]);
      setActiveKbId(newKb.id);
      showMessage(`Created KB: ${name}`, "success");
      setIsCreateModalOpen(false);
      setNewKbName('');
    } catch (error) {
      console.error(error);
      showMessage("Failed to create Knowledge Base", "error");
    }
  };

  const handleDeleteKB = async (id) => {
    if (!confirm("Are you sure? This will delete all documents in this knowledge base.")) return;
    try {
      await deleteKB(id);
      const newKbs = kbs.filter(k => k.id !== id);
      setKbs(newKbs);
      if (activeKbId === id) {
        setActiveKbId(newKbs.length > 0 ? newKbs[0].id : null);
      }
      showMessage("Knowledge Base deleted", "success");
    } catch (error) {
      console.error(error);
      showMessage("Failed to delete KB", "error");
    }
  };

  const handleSelectKB = (kb) => {
    setActiveKbId(kb.id);
  };

  const handleViewDocs = (kb) => {
    setSelectedKb(kb);
    setIsDocModalOpen(true);
  };

  useEffect(() => {
    fetchKBs();
  }, []);

  const activeKb = kbs.find(kb => kb.id === activeKbId);

  return (
    <div className="min-h-screen bg-black">
      <FlashMessage
        message={flash.message}
        type={flash.type}
        onClose={closeFlash}
      />

      <header className="bg-black border-b border-gray-800 shadow-lg">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-6">
          <div className="flex items-center justify-between">
            <div>
              <h1 className="text-3xl font-bold text-white tracking-tight">Smart Learn Avatar</h1>
              <p className="text-sm text-gray-400 mt-1">Manage your knowledge bases — the gateway to your ESP32-S3-BOX-3</p>
            </div>
            <div className="flex items-center gap-3">
              <button
                onClick={() => setIsAiSetupOpen(true)}
                className="flex items-center gap-2 px-4 py-3 bg-gray-800 text-white rounded-xl hover:bg-gray-700 transition-all font-medium border border-gray-700"
              >
                <SparklesIcon className="w-5 h-5" />
                AI Setup
              </button>
              <button
                onClick={() => setIsIoTSetupOpen(true)}
                className="flex items-center gap-2 px-4 py-3 bg-gray-800 text-white rounded-xl hover:bg-gray-700 transition-all font-medium border border-gray-700"
              >
                <WrenchScrewdriverIcon className="w-5 h-5" />
                IoT Setup
              </button>
              <button
                onClick={() => setIsCreateModalOpen(true)}
                className="flex items-center gap-2 px-6 py-3 bg-[#04B900] text-white rounded-xl hover:bg-[#04B900]/90 transition-all shadow-md transform hover:scale-[1.02] font-medium"
              >
                <PlusIcon className="w-5 h-5" />
                New Knowledge Base
              </button>
            </div>
          </div>
        </div>
      </header>

      <main className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        {kbs.length === 0 ? (
          <div className="flex flex-col items-center justify-center h-96 text-center">
            <div className="w-20 h-20 bg-gray-800 rounded-full flex items-center justify-center mb-4">
              <PlusIcon className="w-10 h-10 text-gray-400" />
            </div>
            <h2 className="text-2xl font-bold text-white mb-2">No Knowledge Bases Yet</h2>
            <p className="text-gray-400 mb-6 font-medium">Create your first knowledge base to get started</p>
            <button
              onClick={() => setIsCreateModalOpen(true)}
              className="px-6 py-3 bg-[#04B900] text-white rounded-xl hover:bg-[#04B900]/90 transition-all font-medium"
            >
              Create Knowledge Base
            </button>
          </div>
        ) : (
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-6">
            {kbs.map((kb) => (
              <KnowledgeBaseCard
                key={kb.id}
                kb={kb}
                onSelect={handleSelectKB}
                onViewDocs={handleViewDocs}
                onDelete={handleDeleteKB}
                isActive={kb.id === activeKbId}
              />
            ))}
          </div>
        )}
      </main>

      <DocumentModal
        isOpen={isDocModalOpen}
        onClose={() => setIsDocModalOpen(false)}
        kbId={selectedKb?.id}
        kbName={selectedKb?.name}
        showMessage={showMessage}
        onDocumentsChange={fetchKBs}
      />

      {kbs.length > 0 && (
        <ChatPopup
          activeKbId={activeKbId}
          kbName={activeKb?.name}
          showMessage={showMessage}
        />
      )}

      <AiSetup
        isOpen={isAiSetupOpen}
        onClose={() => setIsAiSetupOpen(false)}
        showMessage={showMessage}
        kbs={kbs}
        refreshKBs={fetchKBs}
      />

      <IoTSetup
        isOpen={isIoTSetupOpen}
        onClose={() => setIsIoTSetupOpen(false)}
        showMessage={showMessage}
        kbs={kbs}
      />

      {isCreateModalOpen && (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/80 backdrop-blur-sm">
          <div className="bg-gray-900 rounded-2xl shadow-2xl w-full max-w-md p-6 border border-gray-800">
            <h2 className="text-2xl font-bold text-white mb-4">Create Knowledge Base</h2>
            <form onSubmit={(e) => { e.preventDefault(); handleCreateKB(newKbName); }} className="space-y-4">
              <div>
                <label className="block text-sm font-medium text-gray-300 mb-2">Knowledge Base Name</label>
                <input
                  type="text"
                  value={newKbName}
                  onChange={(e) => setNewKbName(e.target.value)}
                  placeholder="e.g., Product Documentation"
                  className="w-full px-4 py-3 bg-black border border-gray-700 rounded-lg focus:ring-2 focus:ring-[#04B900] focus:border-[#04B900] outline-none text-white placeholder-gray-500 font-medium"
                  required
                  autoFocus
                />
              </div>
              <div className="flex justify-end gap-3 pt-4">
                <button
                  type="button"
                  onClick={() => { setIsCreateModalOpen(false); setNewKbName(''); }}
                  className="px-4 py-2 text-gray-300 border border-gray-700 hover:bg-gray-800 rounded-lg transition-colors font-medium"
                >
                  Cancel
                </button>
                <button
                  type="submit"
                  disabled={!newKbName.trim()}
                  className="px-6 py-2 bg-[#04B900] text-white rounded-lg hover:bg-[#04B900]/90 disabled:opacity-50 transition-colors font-medium"
                >
                  Create
                </button>
              </div>
            </form>
          </div>
        </div>
      )}
    </div>
  );
}

export default App;
