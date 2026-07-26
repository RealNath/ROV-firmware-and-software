import pyzbar
from fastapi.responses import StreamingResponse
from data_source import get_data
import socket
import struct
from starlette.responses import HTMLResponse
import threading
import time
from fastapi import FastAPI, WebSocket
import asyncio
import rtsp
import io
from pyzbar.pyzbar import decode

app = FastAPI()

RTSP_URL = "rtsp://admin:123456@192.168.42.206:554/stream1"
HOST = '192.168.1.177'
PORT = 8888

telemetry = {
    "pitch": 0,
    "roll": 0,
    "yaw": 0,
    "depth": 0,
    "qr_code": ""
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

def generate_frames():
    last_qr_time = 0
    with rtsp.Client(rtsp_server_uri = RTSP_URL, verbose=True) as client:
        while True:
            image = client.read()
            if image is not None:
                decoded_objects = decode(image)
                if decoded_objects:
                    qr_text = decoded_objects[0].data.decode()
                    telemetry["qr_code"] = qr_text
                    last_qr_time = time.time()
                
                # Remove QR code result if no QR appears in 1 sec
                elif time.time() - last_qr_time > 1.0:
                    telemetry["qr_code"] = ""
                
                buffer = io.BytesIO()
                image.save(buffer, format="JPEG")
                frame = buffer.getvalue()
                yield(b'--frame\r\n'
                      b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
            time.sleep(0.05)

@app.get("/")
async def get_interface():
    with open("index.html", "r") as f:
        return HTMLResponse(f.read())

@app.get("/index.js")
async def get_js():
    with open("index.js", "r") as f:
        # Note: We set the media_type so the browser knows it's JavaScript
        return HTMLResponse(f.read(), media_type="application/javascript")

@app.get("/style.css")
async def get_css():
    with open("style.css", "r") as f:
        return HTMLResponse(f.read(), media_type="text/css")

@app.websocket("/ws")
async def websocket_endpoint(ws: WebSocket):
    await ws.accept()
    try:
        while True:
            await ws.send_json(telemetry)
            await asyncio.sleep(0.1)
    except Exception as e:
        print(f"Error: {e}")

@app.get("/video_feed")
async def get_video_feed():
    return StreamingResponse(generate_frames(), media_type="multipart/x-mixed-replace; boundary=frame")