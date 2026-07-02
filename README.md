# WinSerial

A lightweight and efficient tool for connecting to serial ports on Windows, uniquely designed to be integrated seamlessly as a tab within the Windows Terminal.

## Usage

### Installation

1. Download the latest compiled executable (`WinSerial.exe`) from the Releases page.
2. Place the executable in a preferred directory on your machine.
3. **Windows Terminal Integration:** Open your Windows Terminal `settings.json` file and add a new profile pointing to the `WinSerial.exe` path. For example:
    `C:\Users\[UserName]\AppData\Local\Packages\Microsoft.WindowsTerminal_8wekyb3d8bbwe\LocalState\settings.json`
    ```json
    {
        "closeOnExit": "never",
        "commandline": "C:\\Path\\To\\Your\\WinSerial.exe",
        "guid": "{91f33bb2-1a76-4d6d-9b70-12f5954657b3}",
        "hidden": false,
        "icon": "C:\\Path\\To\\Your\\WinSerial.ico",
        "name": "WinSerial"
    }

    ```

### Connecting to a Serial Port

When you launch WinSerial (either directly or via Windows Terminal), a configuration dialog will prompt you to set up the connection parameters:

* **Port:** Select the target COM port from the dropdown list.
* **Baud Rate:** Choose the communication speed (e.g., 9600, 115200).
* **Data Bits, Stop Bits, Parity, Flow Control:** Configure these based on your target device's requirements.
* **Encoding:** Select between `UTF-8` and `GBK` for proper character display.
* **Echo Mode:** Toggle local echo `On` or `Off`.

Click **OK** to establish the connection and start the terminal session.

### Hotkeys

WinSerial features built-in hotkeys for quick on-the-fly configurations during your session.

**Note:** You must press `Ctrl + A` first to enter command mode, followed by the action key:

* `Ctrl + A` then `Ctrl + C`: Toggle console encoding format (switches between UTF-8 and GBK).
* `Ctrl + A` then `Ctrl + E`: Toggle Echo mode (switches between Off / On).
* `Ctrl + A` then `Ctrl + X`: Safely exit the application.

## Development

[WIP] - Work In Progress. Information regarding building the project from source, dependencies (such as `boost::asio`), and contribution guidelines will be updated soon.

## License

This project is licensed under the Apache License Version 2.0. See the `LICENSE` file for more details.

