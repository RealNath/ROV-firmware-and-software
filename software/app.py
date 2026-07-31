"""
app.py — ROV Ground Control Station (FastAPI)

Networking:
  - UDP send socket (non-blocking):  sends RovCommand packets to ESP32 at ~50 Hz
  - UDP recv socket (blocking, own thread): listens for callback/telemetry packets from ESP32

RovTelemetry struct (from firmware, little-endian):
    float depth
    float vel_x, vel_y, vel_z
    float roll, pitch, yaw
    bool  isGripperHold (1 byte)
    bool  isLightsOn    (1 byte)
    Total: 7*4 + 2 = 30 bytes

RovCommand callback struct (firmware echoes the received command):
    uint8_t command
    float   f0, f1, f2
    Total: 13 bytes
"""

import asyncio
import io
import struct
import threading
import time
import socket
from collections import deque

from fastapi import FastAPI, WebSocket
from fastapi.responses import StreamingResponse
from starlette.responses import HTMLResponse

try:
    import rtsp
    RTSP_AVAILABLE = True
except Exception:
    RTSP_AVAILABLE = False

try:
    from pyzbar.pyzbar import decode as qr_decode
    QR_AVAILABLE = True
except Exception:
    QR_AVAILABLE = False

from data_source import get_commands, pack_correct_depth

app = FastAPI()

# ── Network config ─────────────────────────────────────────────────────────────
ESP32_IP   = "192.168.42.177"   # ROV static IP  (must match firmware STATIC_IP)
ESP32_PORT = 8888               # ESP32 listens for commands on this port  (firmware LOCAL_PORT)
LOCAL_PORT = 8889               # PC listens for telemetry/callbacks        (firmware REMOTE_PORT)

RTSP_URL = "rtsp://admin:123456@192.168.42.206:554/stream1"

# ── Struct sizes (must match firmware) ────────────────────────────────────────
# RovTelemetry: float depth, vel[3], rot[3], bool grip, bool light
TELEMETRY_SIZE = 7 * 4 + 2   # 30 bytes
# RovCommand:   uint8 cmd, float[3]
COMMAND_CB_SIZE = 1 + 3 * 4   # 13 bytes

CMD_NAMES = {
    0: "Translate",
    1: "Rotate",
    2: "SetLightOn",
    3: "SetGripperHold",
    4: "CorrectDepth",
    5: "RovCallback",
}

# ── Shared state ───────────────────────────────────────────────────────────────
telemetry = {
    "depth": 0.0,
    "vel_x": 0.0, "vel_y": 0.0, "vel_z": 0.0,
    "roll": 0.0, "pitch": 0.0, "yaw": 0.0,
    "isGripperHold": False,
    "isLightsOn": False,
    "qr_code": "",
}

# Ring buffers for the UI logs (newest first)
command_log:  deque = deque(maxlen=200)
callback_log: deque = deque(maxlen=200)

# ── UDP send socket (non-blocking) ─────────────────────────────────────────────
send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
send_sock.setblocking(False)

# ── UDP recv socket (blocking, dedicated thread) ───────────────────────────────
recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
recv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
recv_sock.bind(("0.0.0.0", LOCAL_PORT))
recv_sock.settimeout(1.0)


def _recv_loop():
    """Listen for telemetry and callback packets from the ESP32."""
    while True:
        try:
            data, addr = recv_sock.recvfrom(256)
        except socket.timeout:
            # Normal when ESP32 is not connected — just keep waiting silently
            continue
        except Exception as e:
            print(f"[RECV] Error: {e}")
            time.sleep(0.5)
            continue

        if len(data) == TELEMETRY_SIZE:
            # RovTelemetry
            depth, vx, vy, vz, roll, pitch, yaw, grip_b, light_b = struct.unpack(
                "<fffffffBB", data
            )
            telemetry["depth"]        = round(depth, 3)
            telemetry["vel_x"]        = round(vx, 3)
            telemetry["vel_y"]        = round(vy, 3)
            telemetry["vel_z"]        = round(vz, 3)
            telemetry["roll"]         = round(roll, 3)
            telemetry["pitch"]        = round(pitch, 3)
            telemetry["yaw"]          = round(yaw, 3)
            telemetry["isGripperHold"]= bool(grip_b)
            telemetry["isLightsOn"]   = bool(light_b)

        elif len(data) == COMMAND_CB_SIZE:
            # RovCallback — ESP32 echoing the command it executed
            cmd_byte, f0, f1, f2 = struct.unpack("<Bfff", data)
            name = CMD_NAMES.get(cmd_byte, f"Unknown({cmd_byte})")
            ts = time.strftime("%H:%M:%S")
            if cmd_byte == 0:   # Translate
                entry = f"[{ts}] CB Translate  x={f0:+.2f} y={f1:+.2f} z={f2:+.2f}"
            elif cmd_byte == 1: # Rotate
                entry = f"[{ts}] CB Rotate     roll={f0:+.2f} pitch={f1:+.2f} yaw={f2:+.2f}"
            else:
                entry = f"[{ts}] CB {name}  val={f0:.3f}"
            callback_log.appendleft(entry)


def _send_loop():
    """Poll joystick at 50 Hz, pack commands, and send to ESP32."""
    while True:
        commands = get_commands()
        for label, pkt in commands:
            try:
                send_sock.sendto(pkt, (ESP32_IP, ESP32_PORT))
            except BlockingIOError:
                pass
            ts = time.strftime("%H:%M:%S")
            command_log.appendleft(f"[{ts}] {label}")
        time.sleep(0.02)  # 50 Hz


threading.Thread(target=_recv_loop, daemon=True).start()
threading.Thread(target=_send_loop, daemon=True).start()


# ── RTSP + QR video stream ─────────────────────────────────────────────────────
def generate_frames():
    if not RTSP_AVAILABLE:
        return
    last_qr_time = 0.0
    with rtsp.Client(rtsp_server_uri=RTSP_URL, verbose=False) as client:
        while True:
            image = client.read()
            if image is None:
                time.sleep(0.05)
                continue
            if QR_AVAILABLE:
                decoded = qr_decode(image)
                if decoded:
                    telemetry["qr_code"] = decoded[0].data.decode()
                    last_qr_time = time.time()
                elif time.time() - last_qr_time > 1.0:
                    telemetry["qr_code"] = ""
            buf = io.BytesIO()
            image.save(buf, format="JPEG")
            frame = buf.getvalue()
            yield (b"--frame\r\nContent-Type: image/jpeg\r\n\r\n" + frame + b"\r\n")
            time.sleep(0.05)


# ── HTTP routes ────────────────────────────────────────────────────────────────
@app.get("/")
async def get_interface():
    with open("index.html", "r") as f:
        return HTMLResponse(f.read())

@app.get("/index.js")
async def get_js():
    with open("index.js", "r") as f:
        return HTMLResponse(f.read(), media_type="application/javascript")

@app.get("/style.css")
async def get_css():
    with open("style.css", "r") as f:
        return HTMLResponse(f.read(), media_type="text/css")

@app.get("/video_feed")
async def get_video_feed():
    return StreamingResponse(
        generate_frames(), media_type="multipart/x-mixed-replace; boundary=frame"
    )

@app.post("/correct_depth")
async def correct_depth(payload: dict):
    """Endpoint called by the web UI to send a CorrectDepth command."""
    depth_val = float(payload.get("depth", 0.0))
    label, pkt = pack_correct_depth(depth_val)
    try:
        send_sock.sendto(pkt, (ESP32_IP, ESP32_PORT))
    except Exception as e:
        return {"ok": False, "error": str(e)}
    ts = time.strftime("%H:%M:%S")
    command_log.appendleft(f"[{ts}] {label}")
    return {"ok": True}


# ── WebSocket — push state to browser ─────────────────────────────────────────
@app.websocket("/ws")
async def websocket_endpoint(ws: WebSocket):
    await ws.accept()
    try:
        while True:
            await ws.send_json({
                "telemetry": telemetry,
                "commands":  list(command_log),
                "callbacks": list(callback_log),
            })
            await asyncio.sleep(0.1)
    except Exception:
        pass