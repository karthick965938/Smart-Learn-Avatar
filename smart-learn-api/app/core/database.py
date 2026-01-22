import chromadb
from chromadb.config import Settings as ChromaSettings
from app.config import settings

client = chromadb.PersistentClient(path=settings.CHROMA_DB_PATH)

def list_knowledge_bases() -> list[dict]:
    """
    List all available knowledge bases (collections).
    """
    try:
        collections = client.list_collections()
        kbs = []
        for col in collections:
            if col.name.startswith("kb_"):
                kb_id = col.name[3:]
                metadata = col.metadata or {}
                # Robust boolean casting for the custom_instruction flag
                raw_custom = metadata.get("custom_instruction", False)
                custom_instruction = str(raw_custom).lower() in ["true", "1", "t", "yes", "y"] if not isinstance(raw_custom, bool) else raw_custom

                kbs.append({
                    "id": kb_id,
                    "name": metadata.get("name", kb_id),
                    "assistant_name": metadata.get("assistant_name", ""),
                    "instruction": metadata.get("instruction", ""),
                    "custom_instruction": custom_instruction
                })
        return kbs
    except Exception as e:
        print(f"Error listing KBs: {e}")
        return []


def get_collection(kb_id: str):
    """
    Get or create a ChromaDB collection for a specific knowledge base.
    """
    return client.get_or_create_collection(name=f"kb_{kb_id}")

def add_documents(kb_id: str, ids: list[str], documents: list[str], embeddings: list[list[float]], metadatas: list[dict]):
    """
    Add documents and their embeddings to a specific KB.
    """
    collection = get_collection(kb_id)
    collection.add(
        ids=ids,
        documents=documents,
        embeddings=embeddings,
        metadatas=metadatas
    )

def query_documents(kb_id: str, query_embedding: list[float], n_results: int = 5):
    """
    Query a specific KB for similar documents.
    """
    collection = get_collection(kb_id)
    results = collection.query(
        query_embeddings=[query_embedding],
        n_results=n_results
    )
    return results

def list_documents(kb_id: str) -> list[str]:
    """
    List all unique documents (filenames) in a specific KB.
    """
    collection = get_collection(kb_id)
    result = collection.get(include=["metadatas"])
    metadatas = result.get("metadatas", [])
    if not metadatas:
        return []
    
    filenames = set()
    for meta in metadatas:
        if meta and "source" in meta:
            filenames.add(meta["source"])
            
    return list(filenames)

def delete_document(kb_id: str, filename: str):
    """
    Delete all chunks associated with a specific filename in a KB.
    """
    collection = get_collection(kb_id)
    collection.delete(
        where={"source": filename}
    )

def delete_knowledge_base(kb_id: str):
    """
    Delete an entire knowledge base (collection).
    """
    try:
        client.delete_collection(name=f"kb_{kb_id}")
    except ValueError:
        pass  # Collection doesn't exist

def set_kb_metadata(kb_id: str, name: str, assistant_name: str = None, instruction: str = None, custom_instruction: bool = False):
    """
    Set metadata for a knowledge base including name, assistant name, custom instruction, and instruction text.
    """
    collection = get_collection(kb_id)
    metadata = {"name": name}
    
    if assistant_name is not None:
        metadata["assistant_name"] = assistant_name
    if instruction is not None:
        metadata["instruction"] = instruction
    
    metadata["custom_instruction"] = custom_instruction
    
    collection.modify(metadata=metadata)

def get_kb_metadata(kb_id: str) -> dict:
    """
    Get metadata for a knowledge base.
    """
    collection = get_collection(kb_id)
    return collection.metadata or {}

# API Key Management
def get_api_key_collection():
    """
    Get or create the API key mapping collection.
    """
    return client.get_or_create_collection(name="iot_api_keys")

def store_api_key_mapping(api_key: str, kb_id: str):
    """
    Store API key to KB ID mapping.
    """
    collection = get_api_key_collection()
    # Use upsert to handle both insert and update cases
    collection.upsert(
        ids=[api_key],
        documents=[f"API key for KB {kb_id}"],
        metadatas=[{"kb_id": kb_id}]
    )
    print(f"Stored API key mapping: {api_key} -> {kb_id}")


def get_kb_id_by_api_key(api_key: str) -> str | None:
    """
    Retrieve KB ID associated with an API key.
    """
    collection = get_api_key_collection()
    try:
        result = collection.get(ids=[api_key], include=["metadatas"])
        print(f"API Key lookup for '{api_key}': {result}")
        if result and result.get("metadatas") and len(result["metadatas"]) > 0:
            kb_id = result["metadatas"][0].get("kb_id")
            print(f"Found KB ID: {kb_id}")
            return kb_id
    except Exception as e:
        print(f"Error retrieving KB ID for API key: {e}")
    print(f"No KB ID found for API key: {api_key}")
    return None
