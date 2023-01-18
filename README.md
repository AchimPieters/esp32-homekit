<b>ESP32-HOMEKIT</b>

Apple HomeKit Accessory Server Library for IDF V5.0. Developing apps and accessories for the home.Let people communicate with and control connected accessories in their home using your app. With the HomeKit or Matter framework, you can provide users the ability to configure accessories and create actions to control them. Group those actions together into powerful automations and trigger them using Siri.
<br>
<br>
<b>WORKS WITH APPLE HOME BADGE</b>

<img  style="float: right;" src="https://github.com/AchimPieters/ESP32-SmartPlug/blob/main/images/works-with-apple-home.svg" width="150">

The Works with Apple Home badge can be used to visually communicate that your accessory is compatible with the Apple Home and Siri on Apple devices. If you plan to develop or manufacture a HomeKit accessory that will be distributed or sold, your company needs to be enrolled in the MFi Program.


See [esp32-homekit-demo](https://github.com/AchimPieters/esp32-homekit-demo) for examples.
<br>
<br>
<sub><sup>-------------------------------------------------------------------------------------------------------------------------------------</sup></sub>
<br>
<br>
<b>QR CODE PAIRING</b>

You can use a QR code to pair with accessories. To enable that feature, you need to configure accessory to use static password and set some setup ID:
```
homekit_server_config_t config = {
    .accessories = accessories,
    .password = "123-45-678",
    .setupId="1QJ8",
};
```
<b>GENERATE QR CODE</b>

Then you need to generate QR code using supplied script:

tools/gen_qrcode `5` `123-45-678` `1QJ8` qrcode.png


| homekit accessory category | Number |
|----------------------------|--------|
| other                      | 1      |
| bridges                    | 2      |
| fans                       | 3      |
| garage door openers        | 4      |
| lighting                   | 5      |
| locks                      | 6      |
| outlets                    | 7      |
| switches                   | 8      |
| thermostats                | 9      |
| sensors                    | 10     |
| security systems           | 11     |
| doors                      | 12     |
| windows                    | 13     |
| window coverings           | 14     |
| programmable switches      | 15     |
| range extenders            | 16     |
| ip cameras                 | 17     |
| video door bells           | 18     |
| air purifiers              | 19     |
| heaters                    | 20     |
| air conditioners           | 21     |
| humidifiers                | 22     |
| dehumidifiers              | 23     |
| apple tv                   | 24     |
| speakers                   | 26     |
| airport                    | 27     |
| sprinklers                 | 28     |
| faucets                    | 29     |
| shower heads               | 30     |
| televisions                | 31     |
| target remotes             | 32     |


