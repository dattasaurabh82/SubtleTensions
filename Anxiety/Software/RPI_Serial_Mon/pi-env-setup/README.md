# PI SETUP

This is a repository for [ansible](https://www.ansible.com/) notebook (agent less system configuration tool) to be configure raspberry pi 3B+.

When run from a client machine, it sshes into the PI, installs bunch of things, edits the configuration, etc. without using any installation shell script.

## Applied Persisant Changes

- Sets a custom ASCII art Message of the Day (MOTD)
- Updates the apt cache
- Hides the Raspberry Pi logo during boot and disables it on the top left corner
- Installs required packages (git, tmux, neofetch, vim, screen, minicom, lsof, net-tools)
- Modifies .bashrc to launch prompt based serial monitor (screen session) whenever a fresh bash session (terminal) is launched.

## Pre-requisites

- The image we used to burn on the micro SD card (32GB) is the   `Raspberry Pi OS Lite (64 bit) without desktop`.
- While using [PI-Imager](https://www.raspberrypi.com/software/) to install this we also made settings changes such as:
  - Give it WiFi credentials (not 5GHz) to auto-connect on the first boot
  - Enable and give it as suitable hostname
  - Give it a suitable user name (I renamed to `serialmonitor`)
  - Give it a suitable password
  - Enable Provide ssh key of the client development machine from which we will often shh into the pi or run the ansible notebooks from.
- Before running the ansible notebook, goes unsaid, make sure that you have [ansible installed](https://docs.ansible.com/ansible/latest/installation_guide/intro_installation.html) on your development client machine.  
- Make Sure to edit the [inventory.ini](inventory.ini) to match pi's host name and user name. For example, my pi's (the one I'm using for the komorebi project) user is `serialmonitor` and the hostname is `serialmonitor.local` (see below).

```ini
[raspberry_pi3_A_PLUS]
   serialmonitor ansible_host=serialmonitor.local ansible_user=serialmonitor
```

## Post Installation

### Configure git

TBD

### Run the notebook

```bash
ANSIBLE_VERBOSITY=1 ansible-playbook -i inventory.ini setup_raspberry_pi.yml
pi.yml
```

## Display

Waveshare 7"

```bash
sudo nano /boot/firmware/config.txt
#DSI1 Use
dtoverlay=vc4-kms-dsi-7inch
#DSI0 Use
#dtoverlay=vc4-kms-dsi-7inch,dsi0
```bash

```bash
sudo nano /boot/firmware/cmdline.txt
video=DSI-1:800x480M@60,rotate=180
```

## Installing arduino-cli

The `arduino-cli` gets installed anyways if you ran the ansible notebook. But now let's install the core and libs

```bash
# Install core, board amd libs
arduino-cli core list
arduino-cli core install arduino:avr
arduino-cli lib install Keyboard

# For ARDUINO MICRO (Main Motor controller of the Installation)
# COMPILE
arduino-cli compile --fqbn arduino:avr:micro SubtleTensions/Anxiety/Software/ArduinoStepperController_ARDUINO_IDE/ -v
#UPLOAD
# First Find your board and note your port
arduino-cli board list
# Then use that port to upload to your Arduino Micro 
arduino-cli upload -t -p [YOUR_PORT] --fqbn arduino:avr:micro SubtleTensions/Anxiety/Software/ArduinoStepperController_ARDUINO_IDE/ -v

# For ARDUINO LEONARDO (Yes/No Keyboard Emulator for PI)
# COMPILE
arduino-cli compile --fqbn arduino:avr:leonardo SubtleTensions/Anxiety/Software/RPI_Serial_Mon/yes_no_selector/ -v
#UPLOAD
# First Find your board and note your port
arduino-cli board list
# Then use that port to upload to your Arduino Leonardo 
arduino-cli upload -t -p [YOUR_PORT] --fqbn arduino:avr:leonardo SubtleTensions/Anxiety/Software/RPI_Serial_Mon/yes_no_selector/ -v
```
