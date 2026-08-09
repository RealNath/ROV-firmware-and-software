// index.js --- WebSocket client for ROV Ground Control

const ws = new WebSocket(`ws://${location.host}/ws`);

const wsStatus   = document.getElementById("ws-status");
const cmdLog     = document.getElementById("cmd-log");
const cbLog      = document.getElementById("cb-log");
const cmdCount   = document.getElementById("cmd-count");
const cbCount    = document.getElementById("cb-count");

function initLiveClock() {
  const timeElement = document.getElementById('live-datetime');
  
  function updateClock() {
    if (!timeElement) return;
    
    const now = new Date();
    
    const dateOptions = { 
      weekday: 'long', 
      year: 'numeric', 
      month: 'long', 
      day: 'numeric' 
    };
    
    const dateStr = now.toLocaleDateString('id-ID', dateOptions);
    const timeStr = now.toLocaleTimeString('id-ID', { hour12: false });
    
    timeElement.textContent = `${dateStr} --- ${timeStr} WIB`;
  }

  updateClock();
  setInterval(updateClock, 1000);
}


const formatTelemetry = (val, decimals = 3, fallback = "0.000") => {
  return (typeof val === "number" && !isNaN(val)) ? val.toFixed(decimals) : fallback;
};

// Initialize clock when DOM is loaded
document.addEventListener('DOMContentLoaded', initLiveClock);


// Initialize RTSP stream source
document.addEventListener("DOMContentLoaded", () => {
  // Wait until the window fully fires the 'load' event
  window.addEventListener("load", () => {
    const videoEl = document.getElementById("videoStream");
    if (videoEl) {
      // Set the src ONLY after page load is 100% complete
      videoEl.src = "/video_feed";
    }
  });
});


// -- WebSocket status -------------------------------------------------------
ws.onopen = () => {
  wsStatus.textContent = "Connected";
  wsStatus.className = "badge badge-online";
};
ws.onclose = () => {
  wsStatus.textContent = "Disconnected";
  wsStatus.className = "badge badge-offline";
};

// -- Main message handler ---------------------------------------------------
ws.onmessage = (event) => {
  const payload = JSON.parse(event.data);

  // Telemetry
  const t = payload.telemetry;
  if(t){
    document.getElementById("t-depth").textContent            = formatTelemetry(t.depth);
    document.getElementById("t-roll").textContent             = formatTelemetry(t.roll);
    document.getElementById("t-pitch").textContent            = formatTelemetry(t.pitch);
    document.getElementById("t-yaw").textContent              = formatTelemetry(t.yaw);
    document.getElementById("t-ax").textContent               = formatTelemetry(t.acc_x);
    document.getElementById("t-ay").textContent               = formatTelemetry(t.acc_y);
    document.getElementById("t-az").textContent               = formatTelemetry(t.acc_z);
    document.getElementById("t-temperature").textContent      = formatTelemetry(t.temp_c, 1, "--");
    document.getElementById("qr-value").textContent           = t.qr_code || "---";
  }

  const gripEl  = document.getElementById("t-grip");
  const lightEl = document.getElementById("t-light");
  gripEl.textContent  = t.isGripperHold ? "ON" : "OFF";
  gripEl.className    = "telem-value badge " + (t.isGripperHold ? "badge-on" : "badge-off");
  lightEl.textContent = t.isLightsOn ? "ON" : "OFF";
  lightEl.className   = "telem-value badge " + (t.isLightsOn ? "badge-on" : "badge-off");

  // Visualizer section
  if(window.rov3DEngine && t) {
      window.rov3DEngine.updateTelemetry(t);
  }

  // Command log
  if(payload.commands){
    renderLog(cmdLog, payload.commands, "cmd", cmdCount);
  }

  // Callback log
  if(payload.callbacks){
    renderLog(cbLog, payload.callbacks, "cb", cbCount);
  }
};

// -- Log renderer -----------------------------------------------------------
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

// -- CorrectDepth form ------------------------------------------------------
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