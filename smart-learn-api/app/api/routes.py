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

# Global conversation history: {kb_id: [ {"role": "user/assistant", "content": "...", "timestamp": ...} ]}
chat_histories = {}

class CreateKBRequest(BaseModel):
    name: str

class KBResponse(BaseModel):
    id: str
    name: str
    assistant_name: str = ""
    instruction: str = ""
    custom_instruction: bool = False
    conversation_types: list[str] = []
    document_count: int = 0

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
    set_kb_metadata(kb_id, request.name, assistant_name="", instruction="", custom_instruction=False, conversation_types=[])
    # Ensure collection exists
    get_collection(kb_id)
    return KBResponse(
        id=kb_id, 
        name=request.name,
        assistant_name="",
        instruction="",
        custom_instruction=False,
        conversation_types=[]
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
    conversation_types: list[str] = []

@router.post("/kb/{kb_id}")
async def set_knowledge_base_metadata(kb_id: str, request: KBMetadataRequest):
    """
    Set metadata (e.g., name, assistant_name, instruction, custom_instruction, conversation_types) for a knowledge base.
    """
    set_kb_metadata(
        kb_id, 
        request.name, 
        assistant_name=request.assistant_name,
        instruction=request.instruction,
        custom_instruction=request.custom_instruction,
        conversation_types=request.conversation_types
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
    
    # Fetch KB metadata for system prompt
    metadata = get_kb_metadata(kb_id)
    kb_name = metadata.get("name", "Knowledge Base")
    assistant_name = metadata.get("assistant_name", kb_name)
    
    # Robust boolean check (handles bool, int 0/1, or string "true"/"false")
    raw_enabled = metadata.get("custom_instruction", False)
    custom_instruction_enabled = str(raw_enabled).lower() in ["true", "1", "t", "yes", "y"] if not isinstance(raw_enabled, bool) else raw_enabled
    custom_instruction_text = metadata.get("instruction", "")
    
    if not results['documents'] or not results['documents'][0]:
        retrieved_chunks = []
    else:
        retrieved_chunks = results['documents'][0]
    
    context = "\n\n".join(retrieved_chunks) if retrieved_chunks else ""
    
    # Check if the KB has ANY documents at all
    from app.core.database import has_documents
    kb_has_data = has_documents(kb_id)

    # Fetch and clean document names for identity/intro summary
    raw_docs = list_documents(kb_id)
    cleaned_docs = []
    for d in raw_docs:
        # 1. Handle URLs (specifically Wikipedia)
        if d.startswith("http"):
            # Take last part of path, replace underscores/dashes with spaces, title case
            clean_name = d.split("/")[-1].replace("_", " ").replace("-", " ")
        else:
            # 2. Handle filenames (remove extensions)
            clean_name = d.rsplit(".", 1)[0] if "." in d else d
        
        if clean_name.strip():
            cleaned_docs.append(clean_name.strip())

    if len(cleaned_docs) > 1:
        doc_summary = ", ".join(cleaned_docs[:-1]) + " and " + cleaned_docs[-1]
    elif len(cleaned_docs) == 1:
        doc_summary = cleaned_docs[0]
    else:
        doc_summary = "general topics"

    # Determine system instruction
    if custom_instruction_enabled and custom_instruction_text.strip():
        # Replace placeholders like {assistant_name} and {kb_name} to make instructions dynamic
        system_instruction = custom_instruction_text.replace("{assistant_name}", assistant_name).replace("{kb_name}", kb_name)
        
        # Grounding Logic:
        # 1. If KB has data: Enforce strict grounding (use context or decline)
        # 2. If KB is empty: Let the AI speak freely based on the custom instruction personality
        if kb_has_data:
            if context.strip():
                system_instruction += f"\n\nIMPORTANT: Use the provided context from the '{kb_name}' knowledge base to answer. Avoid using outside knowledge. If the answer is not in the context, politely say you don't have that specific information."
            else:
                system_instruction += f"\n\nIMPORTANT: The user is asking about a topic not found in the '{kb_name}' knowledge base. Politely explain that you can only answer questions based on the provided documents ({doc_summary}) and suggest what topics YOU CAN help with."
    else:
        # Default instruction
        if not kb_has_data:
            # KB is empty - allow free-roaming helpful assistant
            system_instruction = f"""You are {assistant_name}, a helpful, polite, and friendly AI assistant.
            Your tone should be warm, professional, and conversational.
            If asked who you are, introduce yourself as {assistant_name} and mention you are ready to help once documents are added to the '{kb_name}'.
            """
        else:
            # KB has data - enforce grounding and identity
            system_instruction = f"""You are {assistant_name}, the {kb_name} assistant. 
            I have specific knowledge about: {doc_summary}.
            
            IDENTITY GUIDELINES:
            If the user asks "Who are you?" or "What is your name?", always respond naturally:
            "I am {assistant_name}, your {kb_name} assistant. I have knowledge about {doc_summary}."
            
            Your tone should be warm, professional, and conversational.
            
            GROUNDING RULES:
            1. Answer questions ONLY using the information provided in the context from the '{kb_name}'.
            2. If context is provided, give a clear, concise answer based strictly on that context.
            3. If context is NOT relevant, politely explain that your current knowledge is limited to {doc_summary} and suggest those topics.

            NEVER use outside knowledge to answer if documents have been provided.
            """

    # Append conversation-type-specific behavior from KB metadata
    ct = metadata.get("conversation_types") or []
    extras = []
    if "Q&A" in ct:
        extras.append("Answer in a direct, concise Q&A style. Prioritize clarity and brevity.")
    if "Follow-up Question" in ct:
        extras.append("Support follow-up questions (e.g. 'Can you elaborate?', 'What about X?'). Keep conversation context in mind and welcome follow-ups.")
    if "Revision Mode" in ct:
        extras.append("If the user requests a revision, rephrasing, or says 'Actually I meant...', provide an updated answer willingly and without repeating the old one at length.")
    if extras:
        system_instruction = (system_instruction.rstrip() + "\n\nConversation behavior:\n" + "\n".join("- " + e for e in extras) + "\n")

    # Final constraint: brevity (for small device screens)
    system_instruction += "\n\nIMPORTANT: Keep your response extremely short and concise (less than 200 characters)."

    # --- Conversation History Logic ---
    now = time.time()
    if kb_id not in chat_histories:
        chat_histories[kb_id] = []
    
    # Clear history if more than 5 minutes have passed since the last interaction
    if chat_histories[kb_id] and (now - chat_histories[kb_id][-1]["timestamp"] > 300):
        chat_histories[kb_id] = []
    
    # Extract the last 6 messages (3 User-Assistant exchanges) for the LLM context
    # We only pass 'role' and 'content' to generate_response
    llm_history = [
        {"role": m["role"], "content": m["content"]} 
        for m in chat_histories[kb_id][-6:]
    ]

    answer = await generate_response(context, request.query, system_instruction, history=llm_history)
    
    # Store current interaction in history
    chat_histories[kb_id].append({"role": "user", "content": request.query, "timestamp": now})
    chat_histories[kb_id].append({"role": "assistant", "content": answer, "timestamp": time.time()})
    
    # Optional: Keep the internal history list small (last 20 messages)
    chat_histories[kb_id] = chat_histories[kb_id][-20:]

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




