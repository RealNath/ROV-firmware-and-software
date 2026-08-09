"""
app.py - ROV Ground Control Station (FastAPI)

Networking:
  - UDP send socket (non-blocking):  sends RovCommand packets to ESP32 at ~50 Hz
  - UDP recv socket (blocking, own thread): listens for callback/telemetry packets from ESP32

RovTelemetry struct (from firmware, little-endian):
    float depth
    float acc_x, acc_y, acc_z
    float roll, pitch, yaw
    int8_t temp_c
    bool  isGripperHold (1 byte)
    bool  isLightsOn    (1 byte)
    Total: 7*4 + 1 + 1 + 1 = 31 bytes

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

import logging
from pathlib import Path
import cv2
import numpy as np

from fastapi import FastAPI, WebSocket
from fastapi.responses import FileResponse, StreamingResponse
from fastapi.staticfiles import StaticFiles
from starlette.responses import HTMLResponse
from contextlib import asynccontextmanager

from camera import discover_and_stream_camera, check_opencv_environment
import rtsp
from pyzbar.pyzbar import decode as qr_decode

from data_source import get_commands, pack_correct_depth

# Directory and Logger Setup
DATA_DIR = Path("data")
VIDEO_DIR = DATA_DIR / "video"
LOG_DIR = DATA_DIR / "logs"

# Camera state setup
LATEST_FRAME = b""
SHUTDOWN_EVENT = threading.Event()
VIDEO_WRITER = None

VIDEO_DIR.mkdir(parents=True, exist_ok=True)
LOG_DIR.mkdir(parents=True, exist_ok=True)

log_filepath = LOG_DIR / f"rov_dagonaut_{time.strftime('%Y%m%d_%H%M%S')}.log"

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
    handlers=[
        logging.FileHandler(log_filepath),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger("ROV")


# -- Network config -------------------------------------------------------------
ESP32_IP   = "192.168.42.177"   # ROV static IP  (must match firmware STATIC_IP)
ESP32_PORT = 8888               # ESP32 listens for commands on this port  (firmware LOCAL_PORT)
LOCAL_PORT = 8889               # PC listens for telemetry/callbacks        (firmware REMOTE_PORT)

check_opencv_environment()
RTSP_URL = discover_and_stream_camera() or "rtsp://admin:123456@192.168.42.206:554/stream1"

# -- Struct sizes (must match firmware) ----------------------------------------
# RovTelemetry: float depth, vel[3], rot[3], int8 temperature, bool grip, bool light
TELEMETRY_SIZE = 7 * 4 + 1 + 2   # 31 bytes
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

# -- Shared state ---------------------------------------------------------------
telemetry = {
    "depth": 0.0,
    "acc_x": 0.0, "acc_y": 0.0, "acc_z": 0.0,
    "roll": 0.0, "pitch": 0.0, "yaw": 0.0,
    "temp_c": 0,
    "isGripperHold": False,
    "isLightsOn": False,
    "qr_code": "",
}

# Ring buffers for the UI logs (newest first)
command_log:  deque = deque(maxlen=200)
callback_log: deque = deque(maxlen=200)

# -- UDP send socket (non-blocking) ---------------------------------------------
send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
send_sock.setblocking(False)
send_sock.bind(('0.0.0.0', ESP32_PORT))

# -- UDP recv socket (blocking, dedicated thread) -------------------------------
recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
recv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
recv_sock.bind(("0.0.0.0", LOCAL_PORT))
recv_sock.settimeout(1.0)


def _recv_loop():
    """Listen for telemetry and callback packets from the ESP32."""
    while not SHUTDOWN_EVENT.is_set():
        try:
            data, addr = recv_sock.recvfrom(256)
        except socket.timeout:
            # Normal when ESP32 is not connected - just keep waiting silently
            continue
        except Exception as e:
            print(f"[RECV] Error: {e}")
            time.sleep(0.5)
            continue

        if len(data) == TELEMETRY_SIZE:
            # RovTelemetry
            depth, vx, vy, vz, roll, pitch, yaw, temp_int, grip_b, light_b = struct.unpack(
                "<fffffffbBB", data
            )
            telemetry["depth"]          = round(depth, 3)
            telemetry["acc_x"]          = round(vx, 3)
            telemetry["acc_y"]          = round(vy, 3)
            telemetry["acc_z"]          = round(vz, 3)
            telemetry["roll"]           = round(roll, 3)
            telemetry["pitch"]          = round(pitch, 3)
            telemetry["yaw"]            = round(yaw, 3)
            telemetry["temp_c"]         = temp_int
            telemetry["isGripperHold"]  = bool(grip_b)
            telemetry["isLightsOn"]     = bool(light_b)

            logger.info(f"[receiver] TELEMETRY RECEIVED: {telemetry}")

        elif len(data) == COMMAND_CB_SIZE:
            # RovCallback - ESP32 echoing the command it executed
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
            logger.info(f"[receiver] CALLBACK RECEIVED: {entry}")
        
        else:
            logger.info(f"[receiver] UNKNOWN DATA RECEIVED (raw bytes): {data}")


def _send_loop():
    """Poll joystick at 50 Hz, pack commands, and send to ESP32."""
    while not SHUTDOWN_EVENT.is_set():
        commands = get_commands()
        for label, pkt in commands:
            try:
                send_sock.sendto(pkt, (ESP32_IP, ESP32_PORT))
            except BlockingIOError:
                pass
            ts = time.strftime("%H:%M:%S")
            command_log.appendleft(f"[{ts}] {label}")
            logger.info(f"[sender] COMMAND SENT: {label}")
        time.sleep(0.02)  # 50 Hz


threading.Thread(target=_recv_loop, daemon=True).start()
threading.Thread(target=_send_loop, daemon=True).start()


# -- RTSP + QR video stream -----------------------------------------------------
def camera_background_task():
    global VIDEO_WRITER, LATEST_FRAME
    FRAME_DELAY = 0.033
    
    video_filename = str(VIDEO_DIR / f"rov_dagonaut_{time.strftime('%Y%m%d_%H%M%S')}.avi")
    fourcc = cv2.VideoWriter_fourcc(*'XVID')
    last_qr_time = 0.0

    while not SHUTDOWN_EVENT.is_set():
        try:
            # Open RTSP connection
            with rtsp.Client(rtsp_server_uri=RTSP_URL, verbose=False) as client:
                stall_counter = 0
                
                while not SHUTDOWN_EVENT.is_set():
                    image = client.read()
                    
                    if image is None:
                        stall_counter += 1
                        if stall_counter > 60:
                            logger.warning("[CAMERA] RTSP stalled. Reconnecting...")
                            break
                        time.sleep(FRAME_DELAY)
                        continue
                        
                    stall_counter = 0

                    # Convert to OpenCV BGR
                    frame_bgr = cv2.cvtColor(np.array(image), cv2.COLOR_RGB2BGR)

                    # Safely Initialize Video Writer
                    if VIDEO_WRITER is None:
                        height, width = frame_bgr.shape[:2]
                        VIDEO_WRITER = cv2.VideoWriter(video_filename, fourcc, 30, (width, height))

                    VIDEO_WRITER.write(frame_bgr)

                    # QR Code Scanning
                    decoded = qr_decode(image)
                    if decoded:
                        telemetry["qr_code"] = decoded[0].data.decode()
                        last_qr_time = time.time()
                        logger.info(f"[camera] QR CODE SCANNED: {telemetry['qr_code']}")
                    elif time.time() - last_qr_time > 1.0:
                        telemetry["qr_code"] = ""

                    # Compress to JPEG for Web GUI
                    success, buffer = cv2.imencode('.jpg', frame_bgr, [int(cv2.IMWRITE_JPEG_QUALITY), 80])
                    if success:
                        LATEST_FRAME = buffer.tobytes()

                    time.sleep(FRAME_DELAY)

        except TimeoutError:
            print(f"[CAMERA] Timed out, unable to connect into RTSP stream URL '{RTSP_URL}'")
            time.sleep(2.0)

        except Exception as e:
            print(f"[CAMERA] Unknown error: '{e}'")
            logger.error(f"[CAMERA] Feed error: {e}. Retrying...")
            time.sleep(2.0)
            
    # Clean up writer only once during full application shutdown
    if VIDEO_WRITER is not None:
        VIDEO_WRITER.release()
        logger.info("[CAMERA] Video writer saved and closed cleanly.")


def web_streamer():
    """Streams the latest frame from memory to the browser."""
    while not SHUTDOWN_EVENT.is_set():
        if LATEST_FRAME:
            yield (b"--frame\r\nContent-Type: image/jpeg\r\n\r\n" + LATEST_FRAME + b"\r\n")
        time.sleep(0.033)


# -- App Lifespan ----------------------------------------------------------------
@asynccontextmanager
async def lifespan(app: FastAPI):
    # App Startup: Launch the camera thread
    cam_thread = threading.Thread(target=camera_background_task, daemon=True)
    cam_thread.start()
    
    yield  # App runs here
    
    # App Shutdown: Set event to break all loops gracefully
    logger.info("[SHUTDOWN] Stopping all processes...")
    SHUTDOWN_EVENT.set()
    cam_thread.join(timeout=3.0)

    try:
        send_sock.close()
        recv_sock.close()
        logger.info("[SHUTDOWN] Network sockets closed.")
    except Exception as e:
        logger.error(f"[SHUTDOWN] Socket cleanup error: {e}")
    

# -- App Declaration
app = FastAPI(lifespan=lifespan)
app.mount("/static", StaticFiles(directory="static"), name="static")


# -- HTTP routes ----------------------------------------------------------------
@app.get("/")
async def get_interface():
    return FileResponse("templates/index.html")


@app.get('/favicon.ico', include_in_schema=False)
async def favicon():
    return FileResponse("static/favicon.ico")

@app.get("/video_feed")
async def get_video_feed():
    return StreamingResponse(web_streamer(), media_type="multipart/x-mixed-replace; boundary=frame")

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
    logger.info(f"[sender] COMMAND SENT: {label} (depth correction: {depth_val} meter)")

    return {"ok": True}


# -- WebSocket - push state to browser -----------------------------------------
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