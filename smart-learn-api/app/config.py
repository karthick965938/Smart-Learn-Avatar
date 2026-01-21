import os
from pydantic_settings import BaseSettings

class Settings(BaseSettings):
    OPENAI_API_KEY: str
    CHROMA_DB_PATH: str = "./chroma_db"
    EMBEDDING_MODEL: str = "text-embedding-3-small"
    LLM_MODEL: str = "gpt-4o-mini"
    CHUNK_SIZE: int = 1000
    CHUNK_OVERLAP: int = 200
    PROJECT_NAME: str = "Smart Learn API"
    VERSION: str = "0.1.0"
    DESCRIPTION: str = "Smart Learn Avatar API application using FastAPI and ChromaDB"

    class Config:
        env_file = ".env"

settings = Settings()
