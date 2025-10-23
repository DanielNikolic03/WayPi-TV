# WayPi-TV
Turns a Raspberry Pi into a lightweight Google TV-like box. It controls Waydroid and TV apps using a numpad/keyboard. Numpad Enter starts Waydroid, Backspace stops it, number keys and +/- switch channels via a simple `channelMap`.

## File structure

```
WayPi-TV/
├─ main                 # Compiled binary (make builds this)
├─ Makefile             # Build rules
├─ README.md            # This file
├─ obj/                 # Object files (generated)
└─ src/
	├─ main.cpp          # Keyboard handling (grabs /dev/input), channel switching
	├─ waydroid.cpp/.h   # Waydroid start/stop, ADB connect, UI
	├─ channels.h        # Channel enum + utilities
	├─ App.h             # App base class
	└─ Apps/
		├─ SVT.cpp/.h     # SVT app integration
		└─ EON.cpp/.h     # EON app integration
```

## Manual build and run (no sudo)

Run these as user `daniel` (or any user that’s in the `input` group):

```bash
cd /home/daniel/Controller
make
./main
```

## Make it run on boot
mkdir -p /home/daniel/.config/systemd/user
nano /home/daniel/.config/systemd/user/controller.service

Paste:
```
[Unit]
Description=Controller in screen session

[Service]
Type=forking
WorkingDirectory=/home/daniel/Controller
ExecStart=/usr/bin/screen -S controller -dm ./main
ExecStop=/usr/bin/screen -S controller -X quit
Restart=on-failure

[Install]
WantedBy=default.target
```

sudo loginctl enable-linger daniel
systemctl --user daemon-reload
systemctl --user enable controller.service
systemctl --user start controller.service

**To disable it**
sudo loginctl disable-linger daniel

Attach to or list the session later:

```bash
screen -ls
screen -r controller
```