import io
import pandas as pd
import httpx
from bs4 import BeautifulSoup
from pypdf import PdfReader
from fastapi import UploadFile, HTTPException

async def extract_text(file: UploadFile) -> str:
    content = await file.read()
    filename = file.filename.lower()
    
    if filename.endswith(".pdf"):
        return extract_text_from_pdf(content)
    elif filename.endswith(".csv"):
        return extract_text_from_csv(content)
    elif filename.endswith(".txt"):
        return extract_text_from_txt(content)
    elif filename.endswith(".docx"):
        return extract_text_from_docx(content)
    else:
        raise HTTPException(status_code=400, detail="Unsupported file type")

def extract_text_from_pdf(content: bytes) -> str:
    try:
        reader = PdfReader(io.BytesIO(content))
        text = ""
        for page in reader.pages:
            text += page.extract_text() + "\n"
        return text
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Error extracting text from PDF: {str(e)}")

def extract_text_from_csv(content: bytes) -> str:
    try:
        df = pd.read_csv(io.BytesIO(content))
        return df.to_string(index=False)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Error extracting text from CSV: {str(e)}")

def extract_text_from_txt(content: bytes) -> str:
    try:
        return content.decode("utf-8")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Error extracting text from TXT: {str(e)}")

def extract_text_from_docx(content: bytes) -> str:
    try:
        import docx
        doc = docx.Document(io.BytesIO(content))
        return "\n".join([para.text for para in doc.paragraphs])
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Error extracting text from DOCX: {str(e)}")

async def extract_text_from_url(url: str) -> str:
    try:
        # Wikipedia Bot Policy: Use a descriptive User-Agent with contact info.
        # Do NOT mimic a browser, or you'll get a 403.
        headers = {
            "User-Agent": "SmartLearnAvatar/1.0 (https://github.com/TODO_USER_GITHUB_PATH; mailto:educational-project@example.com) Educational-AI-Bot",
            "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
            "Accept-Encoding": "gzip, deflate, br",
            "Connection": "keep-alive"
        }
        
        async with httpx.AsyncClient(follow_redirects=True) as client:
            # Note: Wikipedia prefers honest identification over browser impersonation
            response = await client.get(url, headers=headers, timeout=30.0)
            if response.status_code >= 400:
                # Log the error and raise an HTTPException with the specific status code
                print(f"HTTP Error fetching URL {url}: {response.status_code}")
                raise HTTPException(status_code=response.status_code, detail=f"Error fetching URL: Status {response.status_code}")
            
        soup = BeautifulSoup(response.text, 'html.parser')
        
        # Remove script and style elements
        for script in soup(["script", "style"]):
            script.decompose()
            
        text = soup.get_text(separator='\n')
        
        # Clean up whitespace
        lines = (line.strip() for line in text.splitlines())
        chunks = (phrase.strip() for line in lines for phrase in line.split("  "))
        text = '\n'.join(chunk for chunk in chunks if chunk)
        
        return text
    except HTTPException:
        # Re-raise HTTP exceptions (like the one we raised above) so they aren't caught by the generic handler
        raise
    except Exception as e:
        # Catch other unexpected errors
        print(f"Unexpected error extracting text from URL {url}: {e}")
        raise HTTPException(status_code=500, detail=f"Error extracting text from URL: {str(e)}")

def chunk_text(text: str, chunk_size: int = 1000, overlap: int = 200) -> list[str]:
    chunks = []
    start = 0
    text_len = len(text)
    
    while start < text_len:
        end = start + chunk_size
        chunk = text[start:end]
        chunks.append(chunk)
        start += chunk_size - overlap
        
    return chunks
