## Cardputer-ADV Installation
1. Download, install, and open the M5Burner tool from the official M5Stack website: https://docs.m5stack.com/en/download
2. Under `Device Type`, select `Cardputer`.
3. Type in `M5Launcher` in the search bar, then click `Download` next to the M5Launcher firmware.
4. Turn off your Cardputer, then hold the `Go` button on your Cardputer while connecting it to your computer using a USB-C cable until your computer recognizes a new storage device.
5. In M5Burner, click `Burn`, then `Continue`, then change the `COM Port` to the one that corresponds to your Cardputer, and click `Start`.
6. Wait until the flashing process is complete, then disconnect your Cardputer from your computer and turn it on. M5Launcher should now be installed and ready to use!
7. Download the `flipper_http_merged_cardputer.bin` file from the `Cardputer-ADV` directory of this repository and copy it to the root of your SD card.
8. With your Cardputer off, insert the SD card into your Cardputer, then turn on your device.
9. Once the app is open, click `SD` then scroll down and select the `flipper_http_merged_cardputer.bin` file, then click `Install`, and lastly scroll down and click `SPIFFS Yes`.
10. Wait until the installation process is complete and your Cardputer will reboot into FlipperHTTP!

For wiring information, click [here](https://github.com/jblanked/FlipperHTTP?tab=readme-ov-file#wiring).
