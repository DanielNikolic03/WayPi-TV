# WayPi-TV
Turns a Raspberry Pi (or other Linux device) into a lightweight Google TV-like box. It controls Waydroid and TV apps using a numpad/keyboard. Numpad Enter starts Waydroid, Backspace stops it, and number keys and +/- switch channels via a simple `channelMap`.

## File structure

```
WayPi-TV/
├─ main                 # Compiled binary (make builds this)
├─ Makefile             # Build rules
├─ README.md            # This file
├─ obj/                 # Object files (generated)
└─ src/
	├─ main.cpp         # Keyboard handling (grabs /dev/input), channel switching
	├─ waydroid.cpp/.h  # Waydroid start/stop, ADB connect, UI
	├─ channels.h       # Channel enum + utilities
	├─ App.h            # App base class
	└─ Apps/
		├─ SVT.cpp/.h   # SVT app integration
		└─ EON.cpp/.h   # EON app integration
```

## Manual build and run (no sudo)

Run these as a regular user that is a member of the `input` group (so the program can read input devices):

```bash
# from the repository root
make
./main
```

If your user is not in the `input` group, add it (replace <your-username>):

```bash
sudo usermod -aG input <your-username>
# or on some systems:
sudo adduser <your-username> input
```

Log out and log back in after changing groups so the new group membership takes effect.

## Make it run on boot (systemd user service)

Create a systemd user service so the controller can start in a background `screen` session on login. Use the current user's home directory and the repository path.

1. Create the user service directory:

```bash
mkdir -p ~/.config/systemd/user
nano ~/.config/systemd/user/controller.service
```

2. Paste the following into `~/.config/systemd/user/controller.service` and update the WorkingDirectory path to your repository folder (for example `/home/<your-username>/WayPi-TV`):

```
[Unit]
Description=WayPi-TV controller in screen session

[Service]
Type=forking
WorkingDirectory=/path/to/WayPi-TV
ExecStart=/usr/bin/screen -S controller -dm ./main
ExecStop=/usr/bin/screen -S controller -X quit
Restart=on-failure

[Install]
WantedBy=default.target
```

3. Enable user lingering (so the user instance can run when not logged in) and enable the service. Replace `<your-username>` with your actual username when running the `loginctl` command:

```bash
sudo loginctl enable-linger <your-username>
systemctl --user daemon-reload
systemctl --user enable controller.service
systemctl --user start controller.service
```

To disable lingering for the user:

```bash
sudo loginctl disable-linger <your-username>
```

To stop or inspect the `screen` session later:

```bash
screen -ls
screen -r controller
```

Notes
- The instructions above avoid hard-coding any personal paths or usernames. Replace `/path/to/WayPi-TV` and `<your-username>` with values appropriate for your system.
- This repo assumes access to Waydroid and ADB where applicable. See the source comments in `src/` for implementation details.

## Install Android TV on Raspberry Pi 5

This repository includes a step-by-step installation guide for running Android TV on a Raspberry Pi 5. The guide was copied into the repo at:

```
install-android-tv-raspberry-pi-5/
```

Follow the files in that directory for detailed instructions, prerequisites, and troubleshooting tips. Completing the steps in this guide is required to get Waydroid and Android TV working correctly on the device — without a working Android environment the controller program in this repo will have limited usefulness.

If you already have the guide in another location, you can also place its contents under `install-android-tv-raspberry-pi-5/` in the repository root. After following the installation guide, return here for instructions on running the controller and integrating it with Waydroid.