// index.js — WebSocket client for ROV Ground Control

const ws = new WebSocket(`ws://${location.host}/ws`);

const wsStatus   = document.getElementById("ws-status");
const cmdLog     = document.getElementById("cmd-log");
const cbLog      = document.getElementById("cb-log");
const cmdCount   = document.getElementById("cmd-count");
const cbCount    = document.getElementById("cb-count");

// ── WebSocket status ───────────────────────────────────────────────────────
ws.onopen = () => {
  wsStatus.textContent = "Connected";
  wsStatus.className = "badge badge-online";
};
ws.onclose = () => {
  wsStatus.textContent = "Disconnected";
  wsStatus.className = "badge badge-offline";
};

// ── Main message handler ───────────────────────────────────────────────────
ws.onmessage = (event) => {
  const payload = JSON.parse(event.data);

  // Telemetry
  const t = payload.telemetry;
  document.getElementById("t-depth").textContent  = t.depth.toFixed(3);
  document.getElementById("t-roll").textContent   = t.roll.toFixed(3);
  document.getElementById("t-pitch").textContent  = t.pitch.toFixed(3);
  document.getElementById("t-yaw").textContent    = t.yaw.toFixed(3);
  document.getElementById("t-vx").textContent     = t.vel_x.toFixed(3);
  document.getElementById("t-vy").textContent     = t.vel_y.toFixed(3);
  document.getElementById("t-vz").textContent     = t.vel_z.toFixed(3);
  document.getElementById("qr-value").textContent = t.qr_code || "—";

  const gripEl  = document.getElementById("t-grip");
  const lightEl = document.getElementById("t-light");
  gripEl.textContent  = t.isGripperHold ? "ON" : "OFF";
  gripEl.className    = "telem-value badge " + (t.isGripperHold ? "badge-on" : "badge-off");
  lightEl.textContent = t.isLightsOn ? "ON" : "OFF";
  lightEl.className   = "telem-value badge " + (t.isLightsOn ? "badge-on" : "badge-off");

  // Command log
  renderLog(cmdLog, payload.commands, "cmd", cmdCount);

  // Callback log
  renderLog(cbLog, payload.callbacks, "cb", cbCount);
};

// ── Log renderer ───────────────────────────────────────────────────────────
function renderLog(container, entries, cssClass, countEl) {
  if (!entries || entries.length === 0) return;
  countEl.textContent = entries.length;

  const frag = document.createDocumentFragment();
  entries.forEach(text => {
    const div = document.createElement("div");
    div.className = `log-entry ${cssClass}`;
    div.textContent = text;
    frag.appendChild(div);
  });

  // Only re-render if the newest entry changed
  const firstChild = container.firstChild;
  if (firstChild && firstChild.textContent === entries[0]) return;

  container.innerHTML = "";
  container.appendChild(frag);
}

// ── CorrectDepth form ──────────────────────────────────────────────────────
window.sendCorrectDepth = async function () {
  const input  = document.getElementById("depth-input");
  const status = document.getElementById("depth-form-status");
  const depth  = parseFloat(input.value);

  if (isNaN(depth)) {
    status.style.color = "#f85149";
    status.textContent = "Enter a valid number.";
    return;
  }

  status.style.color = "#8b949e";
  status.textContent = "Sending…";

  try {
    const resp = await fetch("/correct_depth", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ depth }),
    });
    const data = await resp.json();
    if (data.ok) {
      status.style.color = "#3fb950";
      status.textContent = `Sent: ${depth.toFixed(3)} m`;
    } else {
      status.style.color = "#f85149";
      status.textContent = `Error: ${data.error}`;
    }
  } catch (e) {
    status.style.color = "#f85149";
    status.textContent = `Network error: ${e.message}`;
  }
};