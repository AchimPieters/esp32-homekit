# ESP32-Homekit-QRCode

Easily generate **HomeKit QR code labels** for your ESP32 accessories!  
This Python tool automatically creates a **print-ready label** including:

- HomeKit Setup Code (with QR code)
- Setup ID
- Device Code
- MAC Address
- Serial Number (with barcode)
-  CSN (with barcode)
-  Neatly aligned and aesthetic layout

Perfect for **professionally labeling your DIY HomeKit projects**.

![label example](images/Example.png)

---

## Requirements

- Python 3.10+
- Pillow  
  ```bash
  pip install pillow
  ```
- qrcode  
  ```bash
  pip install qrcode
  ```
- `Barcode39.ttf` (included in the repo)
- High-resolution label template (`qrcode_ext.png` is provided)

---

## Installation

Clone the repository:

```bash
git clone https://github.com/AchimPieters/esp32-homekit-qrcode.git
cd esp32-homekit-qrcode
```

---

## Usage

To generate a label, run:

```bash
./gen_qrcode 5 123-45-678 1QJ8 4E19CB2E368D new/qrcode.png
```

Where:

- `5` = HomeKit accessory category (e.g., **Lighting**)
- `123-45-678` = Setup Code (**XXX-XX-XXX** format)
- `1QJ8` = Setup ID (**4-character unique ID**)
- `4E19CB2E368D` = MAC address
- `new/qrcode.png` = Output file path

Each run creates a unique label in the `/new/` directory, containing:

- A valid HomeKit setup code
- A unique setup ID
- Auto-generated Serial Number and CSN
- Matching barcodes (Code39)
- A QR code that follows **Apple HomeKit** standards

---

## Printing

The generated PNG file is ready for printing on **white label stickers**.  
For best results, use a **laser printer** or a **high-resolution inkjet printer**.

---

## How It Works

1. Loads the high-resolution label template (`qrcode_ext.png`)
2. Generates or accepts the HomeKit Setup Code and Setup ID
3. Creates the HomeKit-compliant QR code
4. Generates device metadata (MAC, Serial Number, CSN)
5. Adds all info and barcodes with aesthetic alignment
6. Exports a finished label as PNG

---

## HomeKit Accessory Categories

| Category         | Number |
|------------------|--------|
| Other            | 1      |
| Bridges          | 2      |
| Fans             | 3      |
| Garage Doors     | 4      |
| Lighting         | 5      |
| Locks            | 6      |
| Outlets          | 7      |
| Switches         | 8      |
| Thermostats      | 9      |
| Sensors          | 10     |
| Security Systems | 11     |
| Doors            | 12     |
| Windows          | 13     |
| Window Coverings | 14     |
| Programmable Sw. | 15     |
| Range Extenders  | 16     |
| IP Cameras       | 17     |
| Video Doorbells  | 18     |
| Air Purifiers    | 19     |
| Heaters          | 20     |
| Air Conditioners | 21     |
| Humidifiers      | 22     |
| Dehumidifiers    | 23     |
| Apple TV         | 24     |
| Speakers         | 26     |
| Airport          | 27     |
| Sprinklers       | 28     |
| Faucets          | 29     |
| Shower Heads     | 30     |
| Televisions      | 31     |
| Target Remotes   | 32     |

For the full list, refer to the official Apple HomeKit documentation.

---

## Integration

Use together with your own ESP32 HomeKit firmware:

- [ESP32-Homekit](https://github.com/AchimPieters/esp32-homekit)
- [ESP32-Homekit-demo](https://github.com/AchimPieters/esp32-homekit-demo)

---

## Credits

- [Pillow](https://python-pillow.org/)
- [qrcode](https://github.com/lincolnloop/python-qrcode)
