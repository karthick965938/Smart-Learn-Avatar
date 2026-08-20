from pathlib import Path

from pydantic_settings import BaseSettings, SettingsConfigDict

PROJECT_ROOT = Path(__file__).resolve().parent.parent
ENV_FILE = PROJECT_ROOT / ".env"


class Settings(BaseSettings):
    OPENAI_API_KEY: str
    CHROMA_DB_PATH: str = str(PROJECT_ROOT / "chroma_db")
    EMBEDDING_MODEL: str = "text-embedding-3-small"
    LLM_MODEL: str = "gpt-4o-mini"
    CHUNK_SIZE: int = 1000
    CHUNK_OVERLAP: int = 200
    PROJECT_NAME: str = "Smart Learn API"
    VERSION: str = "0.1.0"
    DESCRIPTION: str = "Smart Learn Avatar API application using FastAPI and ChromaDB"
    KB_URL: str | None = None
    API_BASE_URL: str = "http://172.22.200.239:5000"

    model_config = SettingsConfigDict(env_file=str(ENV_FILE), extra="ignore")


settings = Settings()
