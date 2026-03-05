"""
OTA endpoint that tells the ESP32 which WebSocket server to connect to.

The ESP32 firmware (ota.cc CheckVersion) POSTs its device info here and
expects a JSON response with a "websocket" section containing {"url": "ws://..."}
"""

import json
import time
from fastapi import APIRouter, Request
from app.config import settings

router = APIRouter()


@router.post("/ota/")
@router.get("/ota/")
async def ota_check(request: Request):
    """
    OTA check endpoint — tells the ESP32 to connect to our local WebSocket server.
    The firmware will store the websocket.url in NVS and use it on next AudioChannel open.
    """
    # Build the WebSocket URL from the server's own host
    # You can override this with WS_PUBLIC_URL in .env for production
    ws_url = settings.WS_PUBLIC_URL or f"ws://{request.headers.get('host', 'localhost:8000')}/ws/xiaozhi"

    return {
        # No new firmware — skip OTA upgrade
        "firmware": {
            "version": "0.0.0",
            "url": "",
        },
        # Server time so device clock stays in sync
        "server_time": {
            "timestamp": int(time.time() * 1000),
            "timezone_offset": 330,  # IST = UTC+5:30 = 330 minutes
        },
        # Tell the ESP32 to use OUR WebSocket server
        "websocket": {
            "url": ws_url,
            "token": "",
        },
    }
