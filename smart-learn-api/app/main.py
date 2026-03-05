from fastapi import FastAPI
from app.api.routes import router
from app.api.xiaozhi_ws import router as xiaozhi_router
from app.api.ota import router as ota_router
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
app.include_router(xiaozhi_router)          # WebSocket: ws://host/ws/xiaozhi
app.include_router(ota_router, prefix="/xiaozhi")  # OTA: POST /xiaozhi/ota/

@app.get("/")
async def root():
    return {"message": "Smart Learn API is running"}

