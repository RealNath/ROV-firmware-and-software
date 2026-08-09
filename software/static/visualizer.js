class ROV3DEngine {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        if (!this.container) {
            console.error(`[3D Visualizer] Container #${containerId} not found!`);
            return;
        }

        this.maxTrailPoints = 800;
        this.trailPositions = [];
        
        // Kinematic State
        this.posX = 0.0;
        this.posY = 0.0;
        this.velX = 0.0;
        this.velY = 0.0;
        this.lastTime = null;
        this.isMocking = false;

        this.initScene();
        this.buildProceduralROV();
        this.initEnvironment();
        this.initTrail();
        
        window.addEventListener('resize', () => this.onWindowResize());
        
        // Handle initial dimension delays
        setTimeout(() => this.onWindowResize(), 200);

        this.animate();
        console.log("%c[3D Visualizer] Initialized successfully.", "color: #38ef7d; font-weight: bold;");
    }

    initScene() {
        this.scene = new THREE.Scene();
        this.scene.background = new THREE.Color(0x060b19);
        this.scene.fog = new THREE.FogExp2(0x060b19, 0.04);

        const width = this.container.clientWidth || 400;
        const height = this.container.clientHeight || 300;

        this.camera = new THREE.PerspectiveCamera(55, width / height, 0.1, 100);
        this.camera.position.set(3, 2.5, 4);

        this.renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
        this.renderer.setSize(width, height);
        this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
        this.container.appendChild(this.renderer.domElement);

        if (typeof THREE.OrbitControls !== 'undefined') {
            this.controls = new THREE.OrbitControls(this.camera, this.renderer.domElement);
            this.controls.enableDamping = true;
            this.controls.dampingFactor = 0.05;
        }
    }

    initEnvironment() {
        const ambientLight = new THREE.AmbientLight(0xffffff, 0.9);
        this.scene.add(ambientLight);

        const dirLight = new THREE.DirectionalLight(0x38bdf8, 1.2);
        dirLight.position.set(5, 15, 5);
        this.scene.add(dirLight);

        const seaGrid = new THREE.GridHelper(40, 40, 0x00f2fe, 0x1e293b);
        seaGrid.position.y = -8;
        this.scene.add(seaGrid);
    }

    buildProceduralROV() {
        this.rovGroup = new THREE.Group();
        // Set marine Euler order: Yaw (Y) -> Pitch (X) -> Roll (Z)
        this.rovGroup.rotation.reorder('YXZ');

        // Main Body Box
        const bodyMat = new THREE.MeshStandardMaterial({ color: 0x00f2fe, roughness: 0.3, metalness: 0.5 });
        const body = new THREE.Mesh(new THREE.BoxGeometry(0.6, 0.3, 0.9), bodyMat);
        this.rovGroup.add(body);

        // Heading Nose Indicator
        const noseMat = new THREE.MeshBasicMaterial({ color: 0xff0055 });
        const nose = new THREE.Mesh(new THREE.ConeGeometry(0.12, 0.3, 4), noseMat);
        nose.rotation.x = -Math.PI / 2;
        nose.position.set(0, 0, -0.55);
        this.rovGroup.add(nose);

        this.scene.add(this.rovGroup);
    }

    initTrail() {
        this.trailGeometry = new THREE.BufferGeometry();
        const initialPositions = new Float32Array(this.maxTrailPoints * 3);
        this.trailGeometry.setAttribute('position', new THREE.BufferAttribute(initialPositions, 3));

        const trailMaterial = new THREE.LineBasicMaterial({ color: 0x38ef7d, linewidth: 2 });
        this.trailLine = new THREE.Line(this.trailGeometry, trailMaterial);
        this.scene.add(this.trailLine);
    }

    updateTelemetry(telemetry, fromMock = false) {
        if (this.isMocking && !fromMock) return;
        if (!telemetry || !this.rovGroup) return;

        const now = performance.now();
        if (!this.lastTime) this.lastTime = now;
        const dt = Math.min((now - this.lastTime) / 1000.0, 0.1);
        this.lastTime = now;

        // 1. Double Integration for Accelerations (with friction decay)
        const ax = parseFloat(telemetry.acc_x) || 0.0;
        const ay = parseFloat(telemetry.acc_y) || 0.0;

        // Apply deadband threshold to filter sensor jitter
        const cleanAx = Math.abs(ax) > 0.05 ? ax : 0.0;
        const cleanAy = Math.abs(ay) > 0.05 ? ay : 0.0;

        this.velX = (this.velX + cleanAx * dt) * 0.92; // 0.92 damping prevents infinite drift
        this.velY = (this.velY + cleanAy * dt) * 0.92;

        this.posX += this.velX * dt;
        this.posY += this.velY * dt;

        // 2. Depth mapping
        const depth = parseFloat(telemetry.depth) || 0.0;
        const posZ = -depth;

        // 3. Update Mesh Position & Orientation
        this.rovGroup.position.set(this.posX, posZ, this.posY);

        const roll  = THREE.MathUtils.degToRad(parseFloat(telemetry.roll) || 0.0);
        const pitch = THREE.MathUtils.degToRad(parseFloat(telemetry.pitch) || 0.0);
        const yaw   = THREE.MathUtils.degToRad(parseFloat(telemetry.yaw) || 0.0);

        this.rovGroup.rotation.set(pitch, yaw, roll);

        // 4. Update Trail Buffer
        this.trailPositions.push(this.posX, posZ, this.posY);
        if (this.trailPositions.length > this.maxTrailPoints * 3) {
            this.trailPositions.splice(0, 3);
        }

        const posAttr = this.trailGeometry.attributes.position;
        posAttr.array.set(this.trailPositions);
        posAttr.needsUpdate = true;
        this.trailGeometry.setDrawRange(0, this.trailPositions.length / 3);
    }

    clearTrail() {
        this.trailPositions = [];
        this.trailGeometry.setDrawRange(0, 0);
        this.posX = 0.0;
        this.posY = 0.0;
        this.velX = 0.0;
        this.velY = 0.0;
    }

    // To test without actual ROV
    doMockSimulation() {
        if (this.isMocking) return; 
        
        this.isMocking = true;
        let angle = 0;
        console.log("%c[3D Visualizer] Mock simulation STARTED. Real telemetry ignored.", "color: #00f2fe;");
        
        this.mockInterval = setInterval(() => {
            if (!this.isMocking) {
                clearInterval(this.mockInterval);
                return;
            }
            angle += 0.05;
            
            // Note the 'true' flag passed as the second argument here
            this.updateTelemetry({
                depth: 1.5 + Math.sin(angle) * 0.5,
                roll: Math.sin(angle) * 15,
                pitch: Math.cos(angle) * 10,
                yaw: (angle * 20) % 360,
                acc_x: Math.cos(angle) * 0.3,
                acc_y: Math.sin(angle) * 0.3
            }, true);
        }, 50);
    }

    stopMockSimulation() {
        this.isMocking = false;
        this.clearTrail();
        console.log("%c[3D Visualizer] Mock simulation STOPPED. Listening to hardware.", "color: #ff9900;");
    }

    onWindowResize() {
        if (!this.container || !this.renderer || !this.camera) return;
        const w = this.container.clientWidth;
        const h = this.container.clientHeight;
        if (w === 0 || h === 0) return;

        this.camera.aspect = w / h;
        this.camera.updateProjectionMatrix();
        this.renderer.setSize(w, h);
    }

    animate() {
        requestAnimationFrame(() => this.animate());
        if (this.controls) this.controls.update();
        this.renderer.render(this.scene, this.camera);
    }
}

// Instantiation handle
window.addEventListener("DOMContentLoaded", () => {
    window.rov3DEngine = new ROV3DEngine("rov-3d-container");

    const resetBtn = document.getElementById("reset-trail-btn");
    if (resetBtn) {
        resetBtn.addEventListener("click", () => {
            if (window.rov3DEngine) window.rov3DEngine.clearTrail();
        });
    }
});