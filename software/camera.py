from onvif import ONVIFDiscovery, ONVIFClient

def discover_and_stream_camera():
    CAMERA_DISCOVERY_TIMEOUT = 5
    print(f"[CAMERA] Scanning for ONVIF camera devices (timeout: {CAMERA_DISCOVERY_TIMEOUT} seconds)")

    try:
        devices = ONVIFDiscovery(timeout=CAMERA_DISCOVERY_TIMEOUT).discover()
        if not devices or len(devices) == 0:
            print("[CAMERA] No ONVIF devices found on the network.")
            return None
        
        camera_ip = devices[0]['host']
        camera_port = devices[0]['port']
        print(f"[CAMERA] Found {len(devices)} device(s), will use the first one found at {camera_ip}:{camera_port}")
        
        # TODO: change to use env variable instead
        username = "admin"
        password = "123456"
        
        print(f"[CAMERA] Connecting to camera at {camera_ip}:{camera_port}")
    
        client = ONVIFClient(camera_ip, camera_port, username, password)
        media_service = client.media()
        
        profiles = media_service.GetProfiles()
        if not profiles:
            print("[CAMERA] No media profiles found on this camera.")
            return None
            
        print(f"[CAMERA] Found {len(profiles)} video profile(s), will use the first one.")
        
        profile_token = profiles[0].token
        profile_name = profile[0].Name
        
        stream_setup = {'Stream': 'RTP-Unicast', 'Transport': {'Protocol': 'RTSP'}}
        stream = media_service.GetStreamUri({'StreamSetup': stream_setup, 'ProfileToken': profile_token})
        
        print(f"\n[CAMERA] Using profile: {profile_name}, token: {profile_token}, RTSP URL: {stream.Uri}")
        return stream.Uri

    except Exception as e:
        print(f"[CAMERA] Unknown error: {e}")
        return None