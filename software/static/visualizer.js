/*
 * ROV 3D Spatial Trajectory and Pose Visualizer using Three.js
 */

class ROV3DEngine {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        if (!this.container) return;

        this.maxTrailPoints = 1000;
        this.trailPositions = [];
        
        // Internal dead reckoning state
        this.posX = 0.0;
        this.posY = 0.0;
        this.lastTime = performance.now();

        this.initScene();
        this.buildProceduralROV();
        this.initEnvironment();
        this.initTrail();
        
        // Window resize handler
        window.addEventListener('resize', () => this.onWindowResize());
        
        // Start animation loop
        this.animate();
    }

    initScene() {
        this.scene = new THREE.Scene();
        // Underwater deep-blue fog
        this.scene.background = new THREE.Color(0x060b19);
        this.scene.fog = new THREE.FogExp2(0x060b19, 0.04);

        // Perspective Camera
        this.camera = new THREE.PerspectiveCamera(
            55,
            this.container.clientWidth / this.container.clientHeight,
            0.1,
            100
        );
        this.camera.position.set(4, 3, 5);

        // WebGL Renderer
        this.renderer = new THREE.WebGLRenderer({ antialias: true });
        this.renderer.setSize(this.container.clientWidth, this.container.clientHeight);
        this.renderer.setPixelRatio(window.devicePixelRatio);
        this.renderer.shadowMap.enabled = true;
        this.container.appendChild(this.renderer.domElement);

        // Mouse Orbit Controls
        this.controls = new THREE.OrbitControls(this.camera, this.renderer.domElement);
        this.controls.enableDamping = true;
        this.controls.dampingFactor = 0.05;
        this.controls.maxDistance = 30;
        this.controls.minDistance = 1;
    }

    initEnvironment() {
        // Lighting
        const ambientLight = new THREE.AmbientLight(0x1e293b, 1.5);
        this.scene.add(ambientLight);

        const surfaceLight = new THREE.DirectionalLight(0x38bdf8, 1.2);
        surfaceLight.position.set(0, 20, 0);
        this.scene.add(surfaceLight);

        // Seabed / Water Plane Reference Grids
        const seaGrid = new THREE.GridHelper(50, 50, 0x00f2fe, 0x1e293b);
        seaGrid.position.y = -10; // Fixed deep reference floor
        this.scene.add(seaGrid);

        // Surface Grid
        const surfaceGrid = new THREE.GridHelper(50, 50, 0x38bdf8, 0x0f172a);
        surfaceGrid.position.y = 0;
        this.scene.add(surfaceGrid);
    }

    /*
     * Constructs a procedural open-frame ROV model using Three.js primitives
     */
    buildProceduralROV() {
        this.rovGroup = new THREE.Group();

        // 1. Aluminum/Carbon Frame Structure
        const frameMat = new THREE.MeshStandardMaterial({ color: 0x334155, roughness: 0.5, metalness: 0.8 });
        const frameGeo = new THREE.BoxGeometry(0.8, 0.5, 1.2);
        const frameWire = new THREE.WireframeGeometry(frameGeo);
        const frameLine = new THREE.LineSegments(frameWire, new THREE.LineBasicMaterial({ color: 0x64748b, linewidth: 2 }));
        this.rovGroup.add(frameLine);

        // 2. Yellow Buoyancy Foam Blocks (Top)
        const foamMat = new THREE.MeshStandardMaterial({ color: 0xfacc15, roughness: 0.3 });
        const foamLeft = new THREE.Mesh(new THREE.BoxGeometry(0.25, 0.2, 1.1), foamMat);
        foamLeft.position.set(-0.25, 0.22, 0);
        const foamRight = foamLeft.clone();
        foamRight.position.set(0.25, 0.22, 0);
        this.rovGroup.add(foamLeft, foamRight);

        // 3. Electronics Pressure Canister (Center Cylinder)
        const tubeMat = new THREE.MeshStandardMaterial({ color: 0x94a3b8, metalness: 0.9, roughness: 0.2 });
        const tube = new THREE.Mesh(new THREE.CylinderGeometry(0.18, 0.18, 0.8, 16), tubeMat);
        tube.rotation.x = Math.PI / 2;
        tube.position.set(0, -0.05, 0);
        this.rovGroup.add(tube);

        // 4. Camera Dome (Front Hemispherical Glass)
        const domeMat = new THREE.MeshPhysicalMaterial({ color: 0x00f2fe, transmission: 0.9, opacity: 1, transparent: true, roughness: 0 });
        const dome = new THREE.Mesh(new THREE.SphereGeometry(0.12, 16, 16, 0, Math.PI * 2, 0, Math.PI / 2), domeMat);
        dome.rotation.x = -Math.PI / 2;
        dome.position.set(0, -0.05, -0.4);
        this.rovGroup.add(dome);

        // 5. Headlight Spotlights
        const lightMat = new THREE.MeshStandardMaterial({ color: 0xffffff, emissive: 0xffffff });
        const lightLeft = new THREE.Mesh(new THREE.CylinderGeometry(0.04, 0.06, 0.1, 12), lightMat);
        lightLeft.rotation.x = Math.PI / 2;
        lightLeft.position.set(-0.3, 0.05, -0.55);
        
        const lightRight = lightLeft.clone();
        lightRight.position.set(0.3, 0.05, -0.55);
        this.rovGroup.add(lightLeft, lightRight);

        // Add actual Three.js Spotlights projecting forward
        this.spotlightLeft = new THREE.SpotLight(0xffffff, 2, 8, Math.PI / 6, 0.5);
        this.spotlightLeft.position.set(-0.3, 0.05, -0.55);
        this.spotlightLeft.target.position.set(-0.3, 0.05, -3);
        
        this.spotlightRight = new THREE.SpotLight(0xffffff, 2, 8, Math.PI / 6, 0.5);
        this.spotlightRight.position.set(0.3, 0.05, -0.55);
        this.spotlightRight.target.position.set(0.3, 0.05, -3);

        this.rovGroup.add(this.spotlightLeft, this.spotlightLeft.target);
        this.rovGroup.add(this.spotlightRight, this.spotlightRight.target);

        // 6. Thrusters (4 Horizontal Corner Vectored + 2 Vertical)
        const thrusterMat = new THREE.MeshStandardMaterial({ color: 0x0f172a, roughness: 0.8 });
        const thrusterGeo = new THREE.CylinderGeometry(0.06, 0.06, 0.15, 12);
        
        const tPositions = [
            { pos: [-0.35, -0.1, -0.4], rot: [0, Math.PI / 4, 0] },  // FL
            { pos: [0.35, -0.1, -0.4],  rot: [0, -Math.PI / 4, 0] }, // FR
            { pos: [-0.35, -0.1, 0.4],  rot: [0, -Math.PI / 4, 0] }, // BL
            { pos: [0.35, -0.1, 0.4],   rot: [0, Math.PI / 4, 0] },  // BR
            { pos: [-0.38, 0.05, 0],    rot: [Math.PI / 2, 0, 0] },  // ML
            { pos: [0.38, 0.05, 0],     rot: [Math.PI / 2, 0, 0] },  // MR
        ];

        tPositions.forEach(cfg => {
            const t = new THREE.Mesh(thrusterGeo, thrusterMat);
            t.position.set(...cfg.pos);
            t.rotation.set(...cfg.rot);
            this.rovGroup.add(t);
        });

        this.scene.add(this.rovGroup);
    }

    initTrail() {
        this.trailGeometry = new THREE.BufferGeometry();
        const initialPositions = new Float32Array(this.maxTrailPoints * 3);
        this.trailGeometry.setAttribute('position', new THREE.BufferAttribute(initialPositions, 3));

        const trailMaterial = new THREE.LineBasicMaterial({
            color: 0x38ef7d,
            linewidth: 3,
            transparent: true,
            opacity: 0.85
        });

        this.trailLine = new THREE.Line(this.trailGeometry, trailMaterial);
        this.scene.add(this.trailLine);
    }

    /*
     * Ingests telemetry, updates dead-reckoning position, pose orientation, and trail
     */
    updateTelemetry(telemetry) {
        const now = performance.now();
        const dt = Math.min((now - this.lastTime) / 1000.0, 0.1); // Cap max dt delta
        this.lastTime = now;

        // Dead reckoning integration for X/Y position
        this.posX += (telemetry.vel_x || 0.0) * dt;
        this.posY += (telemetry.vel_y || 0.0) * dt;
        
        // Depth mapping (Depth positive down -> In Three.js Y is up, so Y = -depth)
        const posZ = -(telemetry.depth || 0.0);

        // Update ROV Mesh Position
        this.rovGroup.position.set(this.posX, posZ, this.posY);

        // Update ROV Orientation (Degrees -> Radians)
        // Adjust coordinate mapping: Pitch (X), Yaw (Y), Roll (Z)
        this.rovGroup.rotation.x = THREE.MathUtils.degToRad(telemetry.pitch || 0);
        this.rovGroup.rotation.y = THREE.MathUtils.degToRad(telemetry.yaw || 0);
        this.rovGroup.rotation.z = THREE.MathUtils.degToRad(telemetry.roll || 0);

        // Toggle headlights according to telemetry state
        const lightState = !!telemetry.isLightsOn;
        this.spotlightLeft.visible = lightState;
        this.spotlightRight.visible = lightState;

        // Append coordinates to dynamic trail geometry buffer
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
    }

    onWindowResize() {
        if (!this.container) return;
        this.camera.aspect = this.container.clientWidth / this.container.clientHeight;
        this.camera.updateProjectionMatrix();
        this.renderer.setSize(this.container.clientWidth, this.container.clientHeight);
    }

    animate() {
        requestAnimationFrame(() => this.animate());
        this.controls.update();
        this.renderer.render(this.scene, this.camera);
    }
}

// Global instance handle
let rov3DEngine = null;

document.addEventListener("DOMContentLoaded", () => {
    rov3DEngine = new ROV3DEngine("rov-3d-container");
    
    const resetBtn = document.getElementById("reset-trail-btn");
    if (resetBtn) {
        resetBtn.addEventListener("click", () => {
            if (rov3DEngine) rov3DEngine.clearTrail();
        });
    }
});