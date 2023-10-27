<b>ESP32-HOMEKIT</b>

<sup>Apple HomeKit Accessory Server Library for IDF V5.0. Developing apps and accessories for the home.Let people communicate with and control connected accessories in their home using your app. With the HomeKit or Matter framework, you can provide users the ability to configure accessories and create actions to control them. Group those actions together into powerful automations and trigger them using Siri. See [esp32-homekit-demo](https://github.com/AchimPieters/esp32-homekit-demo) for examples.</sup> 
<br>
<br>
<sub><sup>____________________________________________________________________________________________________________________________</sup></sub>
<br>
<br>
<b>PARTITION SETUP</b>

<sup>You need to add a custom partition of type data and subtype "homekit" and at least 4KB (0x1000) in size for HomeKit data storage. Put this into `partitions.csv` file in your project:</sup> 

```
# ESP-IDF Partition Table
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
phy_init, data, phy,     0xe000,  0x1000,
homekit,  data, homekit, 0xf000,  0x1000,
factory,  app,  factory, 0x10000, 1M,
```

<sup>Enable custom partitions in project configuration (sdkconfig):</sup> 

```
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
```
<br>
<br>
<sub><sup>____________________________________________________________________________________________________________________________</sup></sub>
<br>
<br>

<b>ACCESSORY SETUP</b>

<sup>This chapter describes how the HomeKit setup payload information is generated, stored, displayed and delivered by
the accessory to the controller for pairing purposes.</sup> 

<sup>SETUP CODE</sup>

<sup>The Setup Code must conform to the format XXXXXXXX where each X is a 0-9 digit - for example, 10148005. For the purposes of generating accessory SRP verifier, the setup code must be formatted as XXX-XX-XXX (including dashes). In this example, the format of setup code used by the SRP verifier must be 101-
48-005.</sup> 

<sup><b>SETUP ID</b></sup> 

<sup>This identifier is an alphanumeric string of 4(0-9, A-Z) characters. This identifier is persistent across reboots and factory reset of the accessory. This identifier must be different than the DeviceID, serial number, model or accessory name and must be random for each accessory instance manufactured by an accessory manufacturer.</sup> 

```
homekit_server_config_t config = {
    .accessories = accessories,
    .password = "123-45-678",
    .setupId="1QJ8",
};
```
<br>
<br>
<sub><sup>____________________________________________________________________________________________________________________________</sup></sub>
<br>
<br>
<b> QR CODE PREREQUISITES</b>

<sup>Before you can generate the QR-Code for you HomeKit device you nee to install Python and some libraries</sup>

<sup>While OS X comes with a large number of Unix utilities, those familiar with Linux systems will notice one key component missing: a package manager. Homebrew fills this void. To install Homebrew, open Terminal or your favorite OS X terminal emulator and run</sup>

```
$ /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
```
<sup>The script will explain what changes it will make and prompt you before the installation begins. Once you’ve installed Homebrew, insert the Homebrew directory at the top of your PATH environment variable. You can do this by adding the following line at the bottom of your ~/.profile file</sup>

```
export PATH="/usr/local/opt/python/libexec/bin:$PATH"
```

<sup>If you have OS X 10.12 (Sierra) or older use this line instead</sup>

```
export PATH=/usr/local/bin:/usr/local/sbin:$PATH
```

<sup>Now, we can install Python 3:</sup>

```
$ brew install python
```

<sup>This will take a minute or two.</sup>

<sup>Then install Pillow. The core image library is designed for fast access to data stored in a few basic pixel formats. It should provide a solid foundation for a general image processing tool.</sup>

```
python3 -m pip install --upgrade pip
```

```
python3 -m pip install --upgrade Pillow
```
<sup>The the last library to generate te qrcode. A standard install uses pypng to generate PNG files and can also render QR codes directly to the console.</sup>
```
pip install qrcode
```

<br>
<br>
<sub><sup>____________________________________________________________________________________________________________________________</sup></sub>
<br>
<br>
<b>GENERATE QR CODE</b>

<sup>Then you need to generate QR code using supplied script:</sup>

```
tools/gen_qrcode 5 123-45-678 1QJ8 qrcode.png
```

<sup><b>HOW TO USE</b></sup>

<sup>Above you see the line that is used to generate the QR code. It's madeup from a few parts:</sup>

<sup>`5` = HOMEKIT ACCESSORY CATEGORY (see table below)</sup>

<sup>`123-45-678` = SETUP CODE (formatted as XXX-XX-XXX)</sup>

<sup>`1QJ8` = SETUP ID (made from 4 random characters)</sup>

<sup>This wil give you this line:</sup>

<sup><b>tools/gen_qrcode `5` `123-45-678` `1QJ8` qrcode.png</b></sup>

<sup>Once you press enter the QR-code will be generated, and can be found in the `TOOLS` folder as an .png image.</sup>
<br>
<br>

| <sup><b>HOMEKIT ACCESSORY CATEGORY</b></sup> | <sup><b>NUMBER</b></sup> |
|----------------------------|--------|
| <sup>Other</sup>                      | <sup>1</sup>      |
| <sup>Bridges</sup>                    | <sup>2</sup>      |
| <sup>Fans</sup>                       | <sup>3</sup>      |
| <sup>Garage door openers</sup>        | <sup>4</sup>      |
| <sup>Lighting</sup>                   | <sup>5</sup>      |
| <sup>Locks</sup>                      | <sup>6</sup>      |
| <sup>Outlets</sup>                    | <sup>7</sup>      |
| <sup>Switches</sup>                   | <sup>8</sup>      |
| <sup>Thermostats</sup>                | <sup>9</sup>      |
| <sup>Sensors</sup>                    | <sup>10</sup>     |
| <sup>Security systems</sup>           | <sup>11</sup>     |
| <sup>Doors</sup>                      | <sup>12</sup>     |
| <sup>Windows</sup>                    | <sup>13</sup>     |
| <sup>Window coverings</sup>           | <sup>14</sup>     |
| <sup>Programmable switches</sup>      | <sup>15</sup>     |
| <sup>Range extenders</sup>            | <sup>16</sup>     |
| <sup>IP cameras</sup>                 | <sup>17</sup>     |
| <sup>Video door bells</sup>           | <sup>18</sup>     |
| <sup>Air purifiers</sup>              | <sup>19</sup>     |
| <sup>Heaters</sup>                    | <sup>20</sup>     |
| <sup>Air conditioners</sup>           | <sup>21</sup>     |
| <sup>Humidifiers</sup>                | <sup>22</sup>     |
| <sup>Dehumidifiers</sup>              | <sup>23</sup>     |
| <sup>Apple tv</sup>                   | <sup>24</sup>     |
| <sup>Speakers</sup>                   | <sup>26</sup>     |
| <sup>Airport</sup>                    | <sup>27</sup>     |
| <sup>Sprinklers</sup>                 | <sup>28</sup>     |
| <sup>Faucets</sup>                    | <sup>29</sup>     |
| <sup>Shower heads</sup>               | <sup>30</sup>     |
| <sup>Televisions</sup>                | <sup>31</sup>     |
| <sup>Target remotes</sup>             | <sup>32</sup>     |
<br>
<br>
<sub><sup>-------------------------------------------------------------------------------------------------------------------------------------</sup></sub>
<br>
<b>WORKS WITH APPLE HOME BADGE</b>

<sup>The Works with Apple Home badge can be used to visually communicate that your accessory is compatible with the Apple Home and Siri on Apple devices. If you plan to develop or manufacture a HomeKit accessory that will be distributed or sold, your company needs to be enrolled in the MFi Program.</sup> 

<img  style="float: right;" src="https://github.com/AchimPieters/ESP32-SmartPlug/blob/main/images/works-with-apple-home.svg" width="150">

<br>
<br>
<sub><sup>____________________________________________________________________________________________________________________________</sup></sub>
<br>
<br>

<sub>ORIGINAL PROJECT</sub>

<sub>MIT LICENCE</sub>

<sub>Copyright © 2017 [Maxim Kulkin | ESP-Homekit-demo](https://github.com/maximkulkin/esp-homekit-demo)</sub>

<sub>Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:</sub>

<sub>The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software</sup></sub>

<sub>THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.</sub>

<br>
<sub><sup>-------------------------------------------------------------------------------------------------------------------------------------</sup></sub>
<br>

<sub>APPLE HOME</sub>

<img  style="float: right;" src="https://github.com/AchimPieters/ESP32-SmartPlug/blob/main/images/apple_logo.png" width="10"><br> <sub>HomeKit Accessory Protocol (HAP) is Apple’s proprietary protocol that enables third-party accessories in the home (e.g., lights, thermostats and door locks) and Apple products to communicate with each other. HAP supports two transports, IP and Bluetooth LE. The information provided in the HomeKit Accessory Protocol Specification (Non-Commercial Version) describes how to implement HAP in an accessory that you create for non-commercial use and that will not be distributed or sold.</sub>

<sub>The HomeKit Accessory Protocol Specification (Non-Commercial Version) can be downloaded from the HomeKit Apple Developer page.</sub>

<sub>Copyright © 2019 Apple Inc. All rights reserved.</sub>


