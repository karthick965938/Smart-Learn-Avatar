from fastapi import FastAPI
from app.api.routes import router
from app.config import settings

app = FastAPI(title=settings.PROJECT_NAME, description=settings.DESCRIPTION, version=settings.VERSION)

from fastapi.middleware.cors import CORSMiddleware

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # Allows all origins
    allow_credentials=True,
    allow_methods=["*"],  # Allows all methods
    allow_headers=["*"],  # Allows all headers
)

app.include_router(router, prefix="/api/v1")

@app.get("/")
async def root():
    return {"message": "RAG SecuraAI API is running"}
