# Embedded Systems Development GitHub

This README provides instructions on setting up and using a webserver on a Raspberry Pi for embedded systems development, including how to interact with a LilyGo Hiking Smartwatch.

## Use Guide

### RPi Webserver Setup

#### Starting the Webserver on Raspberry Pi (Linux)

1. **Activate Virtual Environment:**
   - Open a terminal on your Raspberry Pi.
   - Navigate to the `webserver/rpi/` directory.
   - Run `source venv/bin/activate` to activate the Python virtual environment.

2. **Start the Webserver:**
   - Execute `python wserver.py` to start the webserver.
   - Note the IP address displayed in the terminal. This is needed to access the webserver from a web browser.

#### Accessing Hike Session Data on the Webserver

- Ensure your device is connected to the same network as the Raspberry Pi.
- Open a web browser and enter the IP address provided by the `wserver.py` script.
- You can now view past session data, including detailed statistics and session information.

### Accessing Transferred Data on the Webserver

- On a device connected to the same network as the Raspberry Pi, open a web browser.
- Enter the IP address provided by the `wserver.py` script.
- Past session data should now be accessible.

### LilyGo Hiking Smartwatch Usage

#### Starting Your Watch

- Power on the watch to see the "Hiking Watch" prompt.
- Press the side button to initiate a new hiking session. Note: This action will overwrite any untransferred session data.

#### During a Hiking Session

- The watch displays your step count and distance covered in kilometers, with updates every 10 seconds.

#### Ending Your Session

- Press the side button to end the session. The watch will save the session data locally.
- After saving, the watch will return to the initial screen, ready for a new session.

#### Transferring Data to the Webserver

- The watch automatically pairs with the webserver via Bluetooth when within range.
- It continuously searches for the webserver to transfer data.
- Upon detecting the webserver, it automatically transfers the session data.

### Troubleshooting

If you encounter issues with data transfer or accessing the webserver, ensure both the smartwatch and Raspberry Pi are powered on and within range. Check the Bluetooth settings and connectivity status on Raspberry Pi.
