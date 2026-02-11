# ESP32 HomeKit Development with IDF

## Overview
This repository provides a step-by-step guide to developing HomeKit accessories using ESP32 and the ESP-IDF framework.

## Prerequisites
Ensure you have the necessary software installed before proceeding:

### 1. Setup Development Environment
#### Docker
- Install [Docker Desktop](https://www.docker.com/products/docker-desktop) for Mac.
- Sign in to your Docker account.
- Pull the latest ESP-IDF version:
  ```sh
  docker pull espressif/idf:v5.5.2
  ```

#### Python Installation
Check if Python 3 is installed:
```sh
python3 --version
```
If not installed:
```sh
xcode-select --install  # Install Xcode
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"  # Install Homebrew
brew install python3  # Install Python 3
```

#### PIP Installation
```sh
python3 -m ensurepip
```

#### ESP Tool Installation
Ensure Python 3.10.1 or newer is installed, then run:
```sh
pip3 install esptool
```

---

## Getting Started with ESP32 HomeKit - LED

### 1. Clone the Repository
```sh
git clone --recursive https://github.com/AchimPieters/esp32-homekit-demo.git
```

### 2. Set Up Docker Environment
```sh
docker run -it -v ~/esp32-homekit-demo:/project -w /project espressif/idf:v5.5.2
```

### 3. Configuration
```sh
cd examples/led
idf.py set-target esp32
idf.py menuconfig
```
Configure Wi-Fi credentials under `StudioPieters`:
- Wi-Fi SSID: `(mysid)`
- Wi-Fi Password: `(mypassword)`

Save and exit.

### 4. Build and Flash ESP32
```sh
idf.py build
esptool.py erase_flash
python -m esptool --chip esp32 -b 460800 --before default_reset --after hard_reset write_flash \
--flash_mode dio --flash_size 4MB --flash_freq 40m \
0x1000 build/bootloader/bootloader.bin \
0x8000 build/partition_table/partition-table.bin \
0x10000 build/main.bin
```

### 5. View Serial Output
```sh
screen /dev/tty.usbserial-01FD1166 115200
```
Replace `/dev/tty.usbserial-01FD1166` with your USB port.

---

## Connecting ESP32 to HomeKit

### 1. Open HomeKit App
Launch your HomeKit app on your smartphone.

### 2. Scan QR Code
Scan the generated QR code to add your ESP32 device.

### 3. Add Device
Accept the uncertified accessory prompt and add the device.

### 4. Control the LED
Toggle the LED on/off directly from the HomeKit app.

### 5. Customize Automations (Optional)
Set up HomeKit automations for advanced functionality.

---

## Generating a QR Code for ESP32 HomeKit
### Setup Code
The HomeKit setup code must be an 8-digit number, formatted as `XXX-XX-XXX` (e.g., `101-48-005`).

### Setup ID
Each accessory needs a unique 4-character setup ID (e.g., `1QJ8`).

```c
homekit_server_config_t config = {
     .accessories = accessories,
     .password = "123-45-678",
     .setupId = "1QJ8",
     .protocol_version = "1.0", // optional: mDNS TXT key "pv"
};
```

### Generate the QR Code
Ensure Python and required libraries are installed:
```sh
python3 -m pip install --upgrade pip Pillow qrcode
```
Generate the QR code:
```sh
tools/gen_qrcode 5 123-45-678 1QJ8 qrcode.png
```


- `5`: HomeKit accessory category (Lighting)
- `123-45-678`: Setup code
- `1QJ8`: Setup ID
- `qrcode.png`: Output file

---


## Pair-Setup rate limiting
Pair-Setup throttling uses a single windowed mechanism:
- Max attempts in window: `HOMEKIT_PAIR_SETUP_MAX_TRIES`
- Attempt window: `HOMEKIT_PAIR_SETUP_WINDOW_MS`
- Cooldown after limit hit: `HOMEKIT_PAIR_SETUP_COOLDOWN_MS`

Failed attempts (for example invalid PIN/SRP proof failures or encrypted payload failures) count towards the limit.
Parallel Pair-Setup sessions are rejected while another client owns an active Pair-Setup flow.

## HomeKit Accessory Categories
| Category              | Number |
|----------------------|--------|
| Other                | 1      |
| Bridges             | 2      |
| Fans                | 3      |
| Garage Door Openers | 4      |
| Lighting            | 5      |
| Locks               | 6      |
| Outlets             | 7      |
| Switches            | 8      |
| Thermostats         | 9      |
| Sensors             | 10     |
| Security systems    | 11     |
| Doors              | 12     |
| Windows            | 13     |
| Window coverings   | 14     |
| Programmable switches | 15  |
| Range extenders    | 16     |
| IP cameras         | 17     |
| Video doorbells    | 18     |
| Air purifiers      | 19     |
| Heaters            | 20     |
| Air conditioners   | 21     |
| Humidifiers        | 22     |
| Dehumidifiers      | 23     |
| Apple TV           | 24     |
| Speakers           | 26     |
| Airport            | 27     |
| Sprinklers         | 28     |
| Faucets            | 29     |
| Shower heads       | 30     |
| Televisions        | 31     |
| Target remotes     | 32     |

For the full list, refer to the official HomeKit documentation.

---

## Conclusion
By following this guide, you've successfully set up an ESP32-based HomeKit accessory using ESP-IDF. Happy coding!
<br>
<br>
<br>
<br>

<sub>__________________________________________________________________________________________</sub>

###### MIT License

<sub>Copyright © 2025 StudioPieters© | Achim Pieters</sub>

<sub>Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:</sub>

<sub>The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.</sub>

<sub>THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.</sub>

<img src="https://raw.githubusercontent.com/AchimPieters/esp32-homekit/refs/heads/main/images/MIT| SOFTWARE WHITE.svg" width="150">

<sub>__________________________________________________________________________________________</sub>

###### WORKS WITH APPLE HOME BADGE
<sub>The Works with Apple Home badge can be used to visually communicate that your accessory is compatible with the Apple Home and Siri on Apple devices. If you plan to develop or manufacture a HomeKit accessory that will be distributed or sold, your company needs to be enrolled in the MFi Program.</sub>

<img src="https://raw.githubusercontent.com/AchimPieters/esp32-homekit/refs/heads/main/images/works-with-apple-home.svg" width="150">

<sub>__________________________________________________________________________________________</sub>

###### APPLE HOME
<sub>HomeKit Accessory Protocol (HAP) is Apple’s proprietary protocol that enables third-party accessories in the home (e.g., lights, thermostats and door locks) and Apple products to communicate with each other. HAP supports two transports, IP and Bluetooth LE. The information provided in the HomeKit Accessory Protocol Specification (Non-Commercial Version) describes how to implement HAP in an accessory that you create for non-commercial use and that will not be distributed or sold.</sub>

<sub>The HomeKit Accessory Protocol Specification (Non-Commercial Version) can be downloaded from the HomeKit Apple Developer page.</sub>

<sub align="left"> <img src="https://raw.githubusercontent.com/AchimPieters/esp32-homekit/refs/heads/main/images/apple_logo.png" width="10"> Copyright © 2019 Apple Inc. All rights reserved.</sub>
