import requests

from arduino.app_utils import Bridge, App


# ============================================
# API CONFIGURATION
# ============================================

API_URL = "http://44.214.74.118:5000/api/v1/iot/rfid/scan"


# ============================================
# RFID HANDLER
# ============================================

def rfid_detected(uid):

    print("RFID detected:", uid)
    print("Calling API:", API_URL)

    try:

        response = requests.post(
            API_URL,
            json={
                "uid": uid
            },
            timeout=5
        )

        print("API status:", response.status_code)
        print("API response:", response.text)

        if response.ok:
            return True

        return False

    except Exception as e:

        print("API error:", e)

        return False


# Register the function so Arduino can call it
Bridge.provide("rfid_detected", rfid_detected)


# Keep the Python application running
App.run()
