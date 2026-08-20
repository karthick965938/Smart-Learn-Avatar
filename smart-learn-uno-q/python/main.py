import requests

from arduino.app_utils import App, Bridge

API_URL = "http://44.214.74.118:5000/api/v1/iot/rfid/scan"
TIMEOUT_SEC = 5


def _empty_result(uid: str) -> dict:
    return {"ok": "0", "assigned": "0", "kb_name": "", "uid": uid}


def rfid_detected(uid: str) -> dict:
    """Post RFID scan to API and return status for the OLED."""
    print(f"RFID detected: {uid}")

    try:
        response = requests.post(
            API_URL,
            json={"uid": uid},
            timeout=TIMEOUT_SEC,
        )
        print(f"API status: {response.status_code}")
        print(f"API response: {response.text}")

        if not response.ok:
            return _empty_result(uid)

        data = response.json()
        kb_name = data.get("kb_name") or ""
        if not isinstance(kb_name, str):
            kb_name = ""

        return {
            "ok": "1",
            "assigned": "1" if data.get("assigned") else "0",
            "kb_name": kb_name,
            "uid": uid,
        }

    except Exception as error:
        print(f"API error: {error}")
        return _empty_result(uid)


Bridge.provide("rfid_detected", rfid_detected)
App.run()
