# Use an official Python slim image
FROM python:3.11-slim

# Set environment variables
ENV PYTHONUNBUFFERED=1
# Suppress pygame support prompt
ENV PYGAME_HIDE_SUPPORT_PROMPT=1
# Headless dummy video driver for pygame (in case it tries to init video)
ENV SDL_VIDEODRIVER=dummy

# Install system dependencies
# libzbar0 is required for pyzbar
# libsdl2-* are required for pygame joystick/event handling
RUN apt-get update && apt-get install -y \
    libzbar0 \
    libsdl2-2.0-0 \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app

# Copy dependency file(s)
COPY pyproject.toml ./

# Install dependencies (fastapi, uvicorn, pyzbar, rtsp, pygame, websockets)
# Using pip directly to avoid needing uv inside the container, matching your pyproject.toml
RUN pip install --no-cache-dir fastapi uvicorn pyzbar rtsp pygame websockets

# Copy the rest of the application code
COPY . .

# Expose FastAPI port
EXPOSE 8000

# Expose UDP port for ESP32 telemetry (app.py binds to 8888 for sending, but if it listens it's here)
EXPOSE 8888/udp

# Command to run the application
CMD ["uvicorn", "app:app", "--host", "0.0.0.0", "--port", "8000"]
