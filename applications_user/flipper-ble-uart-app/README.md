# flipper-ble-uart-app
Flipper Zero application to control Adafruit BLE Friend UART device. The BLE Friend is used to wirelessly transmit a UART connection via BLE to be received by another BLE-capable device. The goal of this application is to be able to connect a BLE Friend to the Flipper Zero to communicate with another BLE Friend that is connected to a Raspberry Pi Zero. This will allow for remotely running scripts on the Raspberry Pi. This project is heavily a work in progress as I am learning C as I work on the application and am not working on it regularly.

Version 0.5: Created basic app layout by following the guide from this repo: 
https://github.com/instantiator/flipper-zero-tutorial-app.git

Does not have any functionality beyond a basic app menu. The next commit should (hopefully) have a functional terminal and/or functional BLE pairing via the BLE Friend board.