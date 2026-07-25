from starlette.responses import HTMLResponse
import threading
import time
from fastapi import FastAPI, WebSocket
import asyncio

app = FastAPI()

telemetry = {
    "pitch": 0,
    "roll": 0,
    "yaw": 0,
    "depth": 0
}

def background_loop():
    while True:
        # print("Background loop running")
        # TODO: update pitch, roll, yaw, depth

        telemetry["pitch"] += 1 # Example
        time.sleep(0.1)

threading.Thread(target=background_loop, daemon=True).start()

@app.get("/")
async def get_interface():
    with open("index.html", "r") as f:
        return HTMLResponse(f.read())

@app.get("/index.js")
async def get_js():
    with open("index.js", "r") as f:
        # Note: We set the media_type so the browser knows it's JavaScript
        return HTMLResponse(f.read(), media_type="application/javascript")

@app.websocket("/ws")
async def websocket_endpoint(ws: WebSocket):
    await ws.accept()
    try:
        while True:
            await ws.send_json(telemetry)
            await asyncio.sleep(0.1)
    except Exception as e:
        print(f"Error: {e}")