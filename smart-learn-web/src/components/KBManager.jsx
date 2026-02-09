
import React from 'react';
import { setActiveKB } from '../api';

const KBManager = ({ kbs, activeKb, refreshKBs, showMessage }) => {

    const handleSetActive = async (name) => {
        try {
            await setActiveKB(name);
            refreshKBs();
            showMessage(`KB '${name}' activated successfully`, 'success');
        } catch (error) {
            showMessage('Failed to activate KB', 'error');
        }
    };

    return (
        <div className="bg-white p-6 rounded-lg shadow-md mb-6">
            <h2 className="text-xl font-bold mb-4">Knowledge Bases</h2>
            {kbs.length === 0 ? (
                <p className="text-gray-500">No Knowledge Bases created yet.</p>
            ) : (
                <ul className="space-y-2">
                    {kbs.map((kb) => (
                        <li key={kb} className="flex justify-between items-center p-3 bg-gray-50 rounded">
                            <span className="font-medium">{kb}</span>
                            {activeKb === kb ? (
                                <span className="text-green-600 font-bold text-sm bg-green-100 px-3 py-1 rounded-full">Active</span>
                            ) : (
                                <button
                                    onClick={() => handleSetActive(kb)}
                                    className="text-blue-600 hover:text-blue-800 text-sm border border-blue-600 px-3 py-1 rounded hover:bg-blue-50"
                                >
                                    Activate
                                </button>
                            )}
                        </li>
                    ))}
                </ul>
            )}
        </div>
    );
};

export default KBManager;
