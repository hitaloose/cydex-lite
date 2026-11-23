# CYDex Lite

A limited version of [CYDex](https://github.com/hitaloose/cydex), focused on using the Claude Desktop **Hardware Buddy** BLE protocol.

Desk showcase for the CYD (ESP32-2432S028) that also talks to **Claude Desktop over Bluetooth LE**, using the [Hardware Buddy protocol](https://github.com/anthropics/claude-desktop-buddy).

- Shows one species at a time from the CYDex SD card (`/cydex/species.bin` + `sprites_96.bin`).
- The footer shows Claude's state (disconnected / idle / running / waiting), output tokens today and a level that goes up every 50K tokens.
- Permission prompts pop up on the device: tap **Approve** or **Deny**.
- Secure pairing: LE Secure Connections with a 6-digit passkey shown on screen.

It is a slimmed-down sibling of CYDex: no Wi-Fi, no saves, no catalog screens, so the BLE stack fits in RAM.

## Build

```
pio run -e cyd -t upload      # firmware
pio test -e native_test       # logic tests (no board)
```

## Connect

In Claude Desktop: **Help → Troubleshooting → Enable Developer Mode**, then **Developer → Open Hardware Buddy… → Connect**, pick `Claude-CYDex-XXXX` and type the passkey shown on the device.

No Claude Desktop handy? `tools/buddy_fake_desktop.py` (needs `bleak`) plays a scripted session.

See `CLAUDE.md` for design notes, memory budget and protocol details.
