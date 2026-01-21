import pytest
from fastapi.testclient import TestClient
from unittest.mock import MagicMock, patch
from app.main import app

client = TestClient(app)

def test_read_main():
    response = client.get("/")
    assert response.status_code == 200
    assert response.json() == {"message": "RAG SecuraAI API is running"}

@patch("app.api.routes.extract_text")
@patch("app.api.routes.get_embeddings")
@patch("app.api.routes.add_documents")
def test_ingest_endpoint(mock_add, mock_embed, mock_extract):
    mock_extract.return_value = "Sample text content"
    mock_embed.return_value = [[0.1, 0.2, 0.3]]
    
    # Mocking file upload
    files = {'file': ('test.txt', b'Sample text content', 'text/plain')}
    response = client.post("/api/v1/kb/kb1/ingest", files=files)
    
    assert response.status_code == 200
    assert response.json() == {"message": "File upload accepted for KB kb1. Processing in background."}
    
    # Verify add_documents called with kb_id
    args = mock_add.call_args
    assert args[0][0] == "kb1"

@patch("app.api.routes.extract_text_from_url")
@patch("app.api.routes.get_embeddings")
@patch("app.api.routes.add_documents")
def test_ingest_url_endpoint(mock_add, mock_embed, mock_extract):
    mock_extract.return_value = "Sample web content"
    mock_embed.return_value = [[0.1, 0.2, 0.3]]
    
    response = client.post("/api/v1/kb/kb1/ingest/url", json={"url": "http://example.com"})
    
    assert response.status_code == 200
    assert response.json() == {"message": "URL ingestion accepted for KB kb1. Processing in background."}
    
    # Verify add_documents called with kb_id and correct metadata
    args = mock_add.call_args
    assert args[0][0] == "kb1"
    metadatas = args[1]['metadatas']
    assert metadatas[0]['source'] == "http://example.com"

@patch("app.api.routes.list_documents")
def test_list_documents(mock_list):
    mock_list.return_value = ["doc1.pdf", "doc2.txt"]
    
    response = client.get("/api/v1/kb/kb1/documents")
    
    assert response.status_code == 200
    assert response.json() == ["doc1.pdf", "doc2.txt"]
    mock_list.assert_called_once_with("kb1")

@patch("app.api.routes.delete_document")
def test_delete_document(mock_delete):
    response = client.delete("/api/v1/kb/kb1/documents/doc1.pdf")
    
    assert response.status_code == 200
    assert response.json() == {"message": "Document doc1.pdf deleted successfully from KB kb1."}
    mock_delete.assert_called_once_with("kb1", "doc1.pdf")

@patch("app.api.routes.delete_document")
@patch("app.api.routes.extract_text")
@patch("app.api.routes.get_embeddings")
@patch("app.api.routes.add_documents")
def test_update_document(mock_add, mock_embed, mock_extract, mock_delete):
    mock_extract.return_value = "New content"
    mock_embed.return_value = [[0.1, 0.2, 0.3]]
    
    files = {'file': ('new_doc.txt', b'New content', 'text/plain')}
    response = client.put("/api/v1/kb/kb1/documents/doc1.pdf", files=files)
    
    assert response.status_code == 200
    assert response.json() == {"message": "Document doc1.pdf update started in background for KB kb1."}
    
    # Verify delete was called for the old filename
    mock_delete.assert_called_once_with("kb1", "doc1.pdf")
    
    # Check if add_documents was called with correct kb_id and metadata source
    call_args = mock_add.call_args
    assert call_args[0][0] == "kb1"
    metadatas = call_args[1]['metadatas']
    assert metadatas[0]['source'] == 'doc1.pdf'

@patch("app.api.routes.cached_query_embedding")
@patch("app.api.routes.query_documents")
@patch("app.api.routes.generate_response")
def test_query_endpoint(mock_llm, mock_query_docs, mock_embed_query):
    mock_embed_query.return_value = [0.1, 0.2, 0.3]
    mock_query_docs.return_value = {'documents': [['Chunk 1', 'Chunk 2']]}
    mock_llm.return_value = "This is the answer."
    
    response = client.post("/api/v1/kb/kb1/query", json={"query": "What is this?"})
    
    assert response.status_code == 200
    data = response.json()
    assert data["answer"] == "This is the answer."
    assert "latency" in data
    assert len(data["context"]) == 2
    
    # Verify query_documents called with kb_id
    mock_query_docs.assert_called_once()
    args = mock_query_docs.call_args
    assert args[0][0] == "kb1"

@patch("app.api.routes.set_kb_metadata")
def test_set_kb_metadata(mock_set_meta):
    response = client.post("/api/v1/kb/kb1", json={"name": "Marketing KB"})
    assert response.status_code == 200
    assert response.json() == {"message": "Metadata updated for KB kb1. Name set to 'Marketing KB'."}
    mock_set_meta.assert_called_once_with("kb1", "Marketing KB")

@patch("app.api.routes.get_kb_metadata")
@patch("app.api.routes.cached_query_embedding")
@patch("app.api.routes.query_documents")
@patch("app.api.routes.generate_response")
def test_query_with_metadata(mock_llm, mock_query_docs, mock_embed_query, mock_get_meta):
    mock_embed_query.return_value = [0.1, 0.2, 0.3]
    mock_query_docs.return_value = {'documents': [['Chunk 1']]}
    mock_llm.return_value = "Answer"
    mock_get_meta.return_value = {"name": "Marketing KB"}
    
    response = client.post("/api/v1/kb/kb1/query", json={"query": "Hello"})
    
    assert response.status_code == 200
    
    # Verify generate_response called with correct system instruction
    call_args = mock_llm.call_args
    assert call_args is not None
    # args: context, query, system_instruction
    system_instruction = call_args[0][2]
    assert "You are the Marketing KB assistant" in system_instruction
