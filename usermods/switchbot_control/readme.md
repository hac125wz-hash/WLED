# SwitchBot Control Usermod

This usermod allows you to control SwitchBot devices (smart switches) via button press on your WLED controller.

## Features

- Control multiple SwitchBot devices via HTTP API
- Button-triggered actions (toggle, on, off)
- Per-button SwitchBot device mapping
- Support for SwitchBot Hub with local API
- Configurable via usermod settings

## Installation

1. Copy this folder to `usermods/switchbot_control/`
2. Add `switchbot_control` to `custom_usermods` in your PlatformIO environment
3. Configure the SwitchBot devices and button mappings in the Usermod Settings page

## Configuration

### Required Settings

- **SwitchBot Hub IP**: Local IP address of your SwitchBot Hub
- **SwitchBot API Token**: Your SwitchBot API token (if using cloud API)

### Per-Button Settings

For each button:
- **Button Number**: Which button triggers the action (0-based index)
- **SwitchBot Device ID**: The device ID of the SwitchBot to control
- **Action**: Type of action (toggle, on, off)

## Usage

1. Press the configured button to send a command to the SwitchBot device
2. The usermod will send an HTTP request to your SwitchBot Hub
3. The SwitchBot device will execute the configured action

## API Communication

The usermod communicates with SwitchBot devices using:
- Local Control API (recommended) via SwitchBot Hub
- or Cloud API via SwitchBot servers

## Notes

- Requires WiFi connection to be active
- Works best with SwitchBot Hub for local control (faster response)
- Button responses may be delayed if using cloud API
