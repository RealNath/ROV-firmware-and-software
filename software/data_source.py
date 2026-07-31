"""
data_source.py - Joystick -> RovCommand binary packets

Controller layout (PS-style, 0-indexed):
  Left stick  Y-axis  -> axis 1  -> Translate Y (depth)
  Right stick X-axis  -> axis 2  -> Translate X (sway)
  Right stick Y-axis  -> axis 3  -> Translate Z (surge/heave fwd-back)

  Buttons (button index on most generic gamepads):
    Button 0 (Cross/A)      -> SetGripperHold  (toggle)
    Button 3 (Triangle/Y)   -> SetLightOn       (toggle)
    Button 4 (L1)           -> Rotate: yaw left
    Button 5 (R1)           -> Rotate: yaw right
    Button 6 (L2)           -> Rotate: roll left
    Button 7 (R2)           -> Rotate: roll right

RovCommand struct (must mirror firmware exactly, packed):
    uint8_t  command          (1 byte)
    float    union_member[3]  (12 bytes, only first float used for bool/single)
    Total: 13 bytes
"""

import pygame
import struct
import math

# -- RovCommandType enum values (mirror firmware uint8_t) ---------------------
CMD_TRANSLATE      = 0
CMD_ROTATE         = 1
CMD_SET_LIGHT_ON   = 2
CMD_SET_GRIPPER    = 3
CMD_CORRECT_DEPTH  = 4
CMD_ROV_CALLBACK   = 5

# -- Deadzone threshold --------------------------------------------------------
DEADZONE = 0.08

def deadzone(v: float, t: float = DEADZONE) -> float:
    return 0.0 if abs(v) < t else v

def pack_command(cmd_type: int, f0: float, f1: float = 0.0, f2: float = 0.0) -> bytes:
    """
    Pack a RovCommand matching the C struct:
        uint8_t  command;
        union {
            struct { float x, y, z; } translationData;   // 12 bytes
            struct { float roll, pitch, yaw; } rotationData;
            bool   gripperHold;                           // 1 byte, padded
            bool   lightsOn;
            float  depthCorrection;
        };
    Total size = 1 (enum) + 12 (union, always 3 floats) = 13 bytes.
    Booleans are sent as float 0.0 / 1.0 in the first union slot.
    """
    return struct.pack("<Bfff", cmd_type, f0, f1, f2)


# -- Module-level state --------------------------------------------------------
_joystick: pygame.joystick.JoystickType | None = None
_joystick_not_found_notified = False
_gripper_on   = False
_light_on     = False
_prev_buttons = {}

def _init_joystick():
    global _joystick
    global _joystick_not_found_notified 
    
    if pygame.joystick.get_count() > 0:
        _joystick = pygame.joystick.Joystick(0)
        _joystick.init()
        _joystick_not_found_notified = False
        print(f"[JOY] Connected: {_joystick.get_name()}")

    elif not _joystick_not_found_notified:
        print("[JOY] No joystick found – running without controller")
        _joystick_not_found_notified = True

pygame.init()
pygame.joystick.init()
_init_joystick()


def get_commands() -> list[tuple[str, bytes]]:
    """
    Poll the joystick and return a list of (label, binary_packet) tuples
    for every command that should be sent this tick.

    Returns:
        list of (human_readable_label, packed_bytes)
    """
    global _joystick, _gripper_on, _light_on, _prev_buttons

    pygame.event.pump()

    # Try to reconnect joystick if lost
    if _joystick is None:
        _init_joystick()

    commands: list[tuple[str, bytes]] = []

    if _joystick is None:
        return commands

    # -- Read axes -------------------------------------------------------------
    num_axes    = _joystick.get_numaxes()
    num_buttons = _joystick.get_numbuttons()

    def axis(i: float) -> float:
        if i < num_axes:
            return deadzone(_joystick.get_axis(i))
        return 0.0

    def btn(i: int) -> bool:
        if i < num_buttons:
            return bool(_joystick.get_button(i))
        return False

    # Left stick  -> axis 0 (X, unused), axis 1 (Y, negative = up)
    # Right stick -> axis 2 (X, sway),   axis 3 (Y, negative = fwd)
    translate_x = axis(2)          # Right stick X -> sway
    translate_z = -axis(3)         # Right stick Y -> surge (invert: push fwd = +)
    translate_y = -axis(1)         # Left  stick Y -> depth (invert: push up = ascend)

    any_translate = any(v != 0.0 for v in [translate_x, translate_y, translate_z])
    if any_translate:
        pkt = pack_command(CMD_TRANSLATE, translate_x, translate_y, translate_z)
        label = f"Translate  x={translate_x:+.2f}  y={translate_y:+.2f}  z={translate_z:+.2f}"
        commands.append((label, pkt))

    # -- Rotation buttons ------------------------------------------------------
    # L1 (btn 4) -> yaw left,  R1 (btn 5) -> yaw right
    # L2 (btn 6) -> roll left, R2 (btn 7) -> roll right
    rot_yaw  = (-1.0 if btn(4) else 0.0) + (1.0 if btn(5) else 0.0)
    rot_roll = (-1.0 if btn(6) else 0.0) + (1.0 if btn(7) else 0.0)

    if rot_yaw != 0.0 or rot_roll != 0.0:
        pkt = pack_command(CMD_ROTATE, rot_roll, 0.0, rot_yaw)
        label = f"Rotate     roll={rot_roll:+.2f}  pitch=+0.00  yaw={rot_yaw:+.2f}"
        commands.append((label, pkt))

    # -- Toggle: gripper - Button 0 (Cross/A), rising edge --------------------
    if btn(0) and not _prev_buttons.get(0, False):
        _gripper_on = not _gripper_on
        pkt = pack_command(CMD_SET_GRIPPER, 1.0 if _gripper_on else 0.0)
        label = f"SetGripper {'ON' if _gripper_on else 'OFF'}"
        commands.append((label, pkt))

    # -- Toggle: lights - Button 3 (Triangle/Y), rising edge ------------------
    if btn(3) and not _prev_buttons.get(3, False):
        _light_on = not _light_on
        pkt = pack_command(CMD_SET_LIGHT_ON, 1.0 if _light_on else 0.0)
        label = f"SetLight   {'ON' if _light_on else 'OFF'}"
        commands.append((label, pkt))

    # Store button state for edge detection next tick
    for i in range(num_buttons):
        _prev_buttons[i] = btn(i)

    return commands


def pack_correct_depth(depth_value: float) -> tuple[str, bytes]:
    """Pack a CorrectDepth command from the web UI form."""
    pkt = pack_command(CMD_CORRECT_DEPTH, depth_value)
    label = f"CorrectDepth  depth={depth_value:.3f}"
    return label, pkt
