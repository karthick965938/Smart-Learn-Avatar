from fastapi import APIRouter, UploadFile, File, BackgroundTasks, HTTPException, Response
from pydantic import BaseModel
import uuid
import time
from async_lru import alru_cache
import httpx

from app.core.ingestion import extract_text, chunk_text, extract_text_from_url
from app.core.embedding import get_embeddings, get_embedding
from app.core.database import add_documents, query_documents, list_documents, delete_document, delete_knowledge_base, set_kb_metadata, get_kb_metadata, list_knowledge_bases, get_collection
from app.core.llm import generate_response
from app.config import settings

router = APIRouter()

class CreateKBRequest(BaseModel):
    name: str

class KBResponse(BaseModel):
    id: str
    name: str
    assistant_name: str = ""
    instruction: str = ""
    custom_instruction: bool = False

class NvsConfigRequest(BaseModel):
    """API fields; values are written to NVS under device keys: ssid, password, ChatGPT_key, Base_url, KB_url, tts_voice, theme_type."""
    ssid: str
    password: str
    openai_key: str   # → NVS key "ChatGPT_key"
    base_url: str     # → NVS key "Base_url"
    kb_url: str       # → NVS key "KB_url"
    tts_voice: str
    theme: str        # "light"|"dark" → NVS "theme_type" as "1"|"0"

@router.get("/kbs", response_model=list[KBResponse])
async def get_all_knowledge_bases():
    """
    List all available knowledge bases.
    """
    return list_knowledge_bases()

@router.post("/kbs", response_model=KBResponse)
async def create_knowledge_base(request: CreateKBRequest):
    """
    Create a new knowledge base.
    """
    kb_id = str(uuid.uuid4())[:8] # Short ID
    # Set default metadata with empty custom fields
    set_kb_metadata(kb_id, request.name, assistant_name="", instruction="", custom_instruction=False)
    # Ensure collection exists
    get_collection(kb_id)
    return KBResponse(
        id=kb_id, 
        name=request.name,
        assistant_name="",
        instruction="",
        custom_instruction=False
    )

class QueryRequest(BaseModel):
    query: str

class QueryResponse(BaseModel):
    answer: str
    context: list[str]
    latency: float

class KBMetadataRequest(BaseModel):
    name: str
    assistant_name: str = ""
    instruction: str = ""
    custom_instruction: bool = False

@router.post("/kb/{kb_id}")
async def set_knowledge_base_metadata(kb_id: str, request: KBMetadataRequest):
    """
    Set metadata (e.g., name, assistant_name, instruction, custom_instruction) for a knowledge base.
    """
    set_kb_metadata(
        kb_id, 
        request.name, 
        assistant_name=request.assistant_name,
        instruction=request.instruction,
        custom_instruction=request.custom_instruction
    )
    return {"message": f"Metadata updated for KB {kb_id}."}

# ... (existing endpoints) ...

@router.post("/kb/{kb_id}/query", response_model=QueryResponse)
async def query_knowledge_base(kb_id: str, request: QueryRequest):
    """
    Query the specific knowledge base and get an answer from the LLM.
    """
    if settings.KB_URL:
        try:
            async with httpx.AsyncClient() as client:
                response = await client.post(
                    settings.KB_URL,
                    json={"query": request.query},
                    timeout=60.0
                )
                response.raise_for_status()
                return QueryResponse(**response.json())
        except Exception as e:
            print(f"Error querying external KB: {e}")
            raise HTTPException(status_code=500, detail=f"External KB Error: {str(e)}")

    start_time = time.time()
    
    query_vec = await cached_query_embedding(request.query)
    
    results = query_documents(kb_id, query_vec, n_results=5)
    
    if not results['documents'] or not results['documents'][0]:
        return QueryResponse(answer="I don't have enough information to answer that.", context=[], latency=time.time() - start_time)
    
    retrieved_chunks = results['documents'][0]
    
    context = "\n\n".join(retrieved_chunks)
    
    # Fetch KB metadata for system prompt
    metadata = get_kb_metadata(kb_id)
    kb_name = metadata.get("name", "Knowledge Base")
    assistant_name = metadata.get("assistant_name", kb_name)
    
    # Robust boolean check (handles bool, int 0/1, or string "true"/"false")
    raw_enabled = metadata.get("custom_instruction", False)
    custom_instruction_enabled = str(raw_enabled).lower() in ["true", "1", "t", "yes", "y"] if not isinstance(raw_enabled, bool) else raw_enabled
    
    custom_instruction_text = metadata.get("instruction", "")
    
    # Use custom instruction if enabled AND not empty, otherwise use default
    if custom_instruction_enabled and custom_instruction_text.strip():
        system_instruction = custom_instruction_text
    else:
        # Default instruction
        system_instruction = f"""You are the {assistant_name} assistant - a helpful, polite, and friendly AI assistant.
        Your tone should be warm, professional, and conversational.
        Answer user questions using ONLY the information provided in the given context.
        Do not use external knowledge, prior assumptions, or general world knowledge.

        IMPORTANT GUIDELINES:
        1. For greetings ('Hi', 'Hello', 'Hey') or identity questions ('Who are you?', 'What can you do?'):
        Respond warmly and introduce yourself as the {assistant_name} assistant, mentioning you can help with questions about the knowledge base.

        2. If the question is directly answered in the context:
        Provide a clear, concise answer based on the context.

        3. If the question is partially related but not fully answered in the context:
        Politely acknowledge the question and provide a brief (one-liner) suggestion of related topics available in the knowledge base.
        Example: "I don't have specific details on that, but I can help you with [topic A], [topic B], or [topic C] from the knowledge base."

        4. If the question is completely unrelated to the context:
        Politely redirect by mentioning what you CAN help with in one concise sentence.
        Example: "That topic isn't covered in my knowledge base, but I can assist you with [main topic areas]."

        5. NEVER simply say "I don't know" without offering helpful alternatives or suggestions.

        6. Keep responses concise and friendly. For suggestions, use a single sentence with 2-3 key topics.

        Always remain strictly scoped to the active knowledge base and do not reference any other knowledge bases.
        """

    
    answer = await generate_response(context, request.query, system_instruction)
    
    latency = time.time() - start_time
    
    return QueryResponse(
        answer=answer,
        context=retrieved_chunks,
        latency=latency
    )

async def process_file(kb_id: str, file_content: bytes, filename: str, filename_override: str = None):
    """
    Background task to process uploaded file: extract, chunk, embed, store.
    """
    try:
        # Create a temporary UploadFile-like object from bytes
        from io import BytesIO
        
        class TempUploadFile:
            def __init__(self, content: bytes, filename: str):
                self.file = BytesIO(content)
                self.filename = filename
            
            async def read(self):
                return self.file.read()
        
        temp_file = TempUploadFile(file_content, filename)
        text = await extract_text(temp_file)
        chunks = chunk_text(text, chunk_size=settings.CHUNK_SIZE, overlap=settings.CHUNK_OVERLAP)
        
        if not chunks:
            print(f"No text extracted from {filename}")
            return

        embeddings = await get_embeddings(chunks)
        ids = [str(uuid.uuid4()) for _ in chunks]
        
        target_filename = filename_override if filename_override else filename
        metadatas = [{"source": target_filename, "chunk_index": i} for i in range(len(chunks))]
        
        add_documents(kb_id, ids=ids, documents=chunks, embeddings=embeddings, metadatas=metadatas)
        print(f"Successfully processed {filename} for KB {kb_id}")
        
    except Exception as e:
        print(f"Error processing file {filename} for KB {kb_id}: {e}")

@router.post("/kb/{kb_id}/ingest")
async def ingest_document(kb_id: str, background_tasks: BackgroundTasks, file: UploadFile = File(...)):
    """
    Upload a document (PDF, CSV, TXT) for background ingestion into a specific KB.
    """
    # Read file content before passing to background task (file handle will be closed after this endpoint returns)
    file_content = await file.read()
    background_tasks.add_task(process_file, kb_id, file_content, file.filename)
    return {"message": f"File upload accepted for KB {kb_id}. Processing in background."}

class UrlRequest(BaseModel):
    url: str

async def process_url(kb_id: str, url: str):
    """
    Background task to process URL: scrape, chunk, embed, store.
    """
    try:
        text = await extract_text_from_url(url)
        
        chunks = chunk_text(text, chunk_size=settings.CHUNK_SIZE, overlap=settings.CHUNK_OVERLAP)
        
        if not chunks:
            print(f"No text extracted from URL {url}")
            return

        embeddings = await get_embeddings(chunks)
        ids = [str(uuid.uuid4()) for _ in chunks]
        
        metadatas = [{"source": url, "chunk_index": i} for i in range(len(chunks))]
        
        add_documents(kb_id, ids=ids, documents=chunks, embeddings=embeddings, metadatas=metadatas)
        print(f"Successfully processed URL {url} for KB {kb_id}")
        
    except Exception as e:
        print(f"Error processing URL {url} for KB {kb_id}: {e}")

@router.post("/kb/{kb_id}/ingest/url")
async def ingest_url(kb_id: str, background_tasks: BackgroundTasks, request: UrlRequest):
    """
    Ingest content from a URL into a specific KB.
    """
    background_tasks.add_task(process_url, kb_id, request.url)
    return {"message": f"URL ingestion accepted for KB {kb_id}. Processing in background."}

@router.get("/kb/{kb_id}/documents", response_model=list[str])
async def list_knowledge_base_documents(kb_id: str):
    """
    List all documents currently in the specific knowledge base.
    """
    return list_documents(kb_id)

@router.delete("/kb/{kb_id}/documents")
async def delete_knowledge_base_document(kb_id: str, filename: str):
    """
    Delete a document from the specific knowledge base by filename.
    """
    delete_document(kb_id, filename)
    return {"message": f"Document {filename} deleted successfully from KB {kb_id}."}

@router.put("/kb/{kb_id}/documents")
async def update_knowledge_base_document(kb_id: str, filename: str, background_tasks: BackgroundTasks, file: UploadFile = File(...)):
    """
    Update a document in the specific knowledge base.
    """
    delete_document(kb_id, filename)
    # Read file content before passing to background task
    file_content = await file.read()
    background_tasks.add_task(process_file, kb_id, file_content, file.filename, filename)
    return {"message": f"Document {filename} update started in background for KB {kb_id}."}

@router.delete("/kb/{kb_id}")
async def delete_knowledge_base_endpoint(kb_id: str):
    """
    Delete an entire knowledge base.
    """
    delete_knowledge_base(kb_id)
    return {"message": f"Knowledge base {kb_id} deleted successfully."}

@alru_cache(maxsize=100)
async def cached_query_embedding(query: str):
    return await get_embedding(query)

@router.post("/iot/generate-nvs")
async def generate_nvs_endpoint(request: NvsConfigRequest):
    """
    Generate an NVS binary using the official ESP-IDF NVS partition generator.
    Flashed at 0x9000; CONFIG.INI (esp_tinyuf2) and the main app read from this partition.
    Base_url should be https://api.openai.com/v1/ for OpenAI. factory_nvs.bin at 0x700000
    is the UF2 app; it only reads NVS at 0x9000 — no conflict.
    """
    from app.utils.nvs_gen import generate_nvs

    try:
        theme_type = "1" if request.theme.lower() == "light" else "0"
        nvs_bin = generate_nvs(
            ssid=request.ssid,
            password=request.password,
            openai_key=request.openai_key,
            base_url=request.base_url,
            kb_url=request.kb_url,
            tts_voice=request.tts_voice,
            theme_type=theme_type,
        )
        return Response(
            content=nvs_bin,
            media_type="application/octet-stream",
            headers={"Content-Disposition": "attachment; filename=nvs.bin"},
        )
    except Exception as e:
        print(f"Error generating NVS: {e}")
        raise HTTPException(status_code=500, detail=f"NVS Generation Error: {str(e)}")




