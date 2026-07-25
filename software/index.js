const ws = new WebSocket("ws://localhost:8000/ws")

ws.onmessage = (event) => {
    const data = JSON.parse(event.data)

    document.getElementById("pitch-value").innerText = data["pitch"]
    document.getElementById("roll-value").innerText = data["roll"]
    document.getElementById("yaw-value").innerText = data["yaw"]
    document.getElementById("depth-value").innerText = data["depth"]
}