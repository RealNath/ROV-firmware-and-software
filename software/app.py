from send_lan import PORT
from send_lan import HOST
from data_source import get_data
import socket
import struct
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

udp_client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp_client.setblocking(False)

def background_loop():
    while True:
        # print("Background loop running")
        rov_data = get_data()
        message = ",".join(str(x) for x in rov_data)
        udp_client.sendto(message.encode(), (HOST, PORT))
        try:
            data, address_info = udp_client.recvfrom(1024)
            if len(data) == 16:
                depth, yaw, roll, pitch = struct.unpack("<ffff", data)
                telemetry["depth"] = depth
                telemetry["yaw"] = yaw
                telemetry["roll"] = roll
                telemetry["pitch"] = pitch
        except BlockingIOError:
            pass

        # telemetry["pitch"] += 1 # Example
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