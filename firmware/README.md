# Firmware: GPS NMEA bridge for WT32-ETH01

The sketch `gps_nmea_bridge.ino` reads NMEA 0183 from a NEO-7M GPS on UART2 at 9600 baud and pushes each sentence over the wired LAN to a Peplink listening as the "GPS Receiver for Raw NMEA 0183 Network Stream." Transport defaults to UDP; a one-line change makes it a TCP client.

License: MIT (see `../LICENSE`).

## Toolchain

1. Install the **Arduino IDE**.
2. Add the ESP32 board package. In **Settings > Additional boards manager URLs**, add:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. In **Boards Manager**, install **"esp32 by Espressif Systems."**
4. Select the board **"WT32-ETH01 Ethernet Module"** if it is in your list, or **"ESP32 Dev Module"** otherwise.

The sketch compiles on both ESP32 Arduino core 2.x and 3.x; the `ETH.begin` call is branched on the core version, so you do not need a specific core.

## Settings to edit

At the top of the sketch, set:

- `PEPLINK_IP` — your Peplink's LAN IP, the listener. The example is `192.168.50.1` (the Peplink default); set it to your Peplink's actual LAN IP.
- `PEPLINK_PORT` — must match the port you set on the Peplink. The example is `10110`, the registered NMEA-0183-over-IP port.
- `USE_TCP` — `0` for UDP (the default and the documented-working transport), or `1` for a TCP client with auto-reconnect.

The GPS pin and baud defines (`GPS_RX_PIN 5`, `GPS_TX_PIN 17`, `GPS_BAUD 9600`) and the LAN8720 Ethernet defines are correct for the WT32-ETH01 and should be left as-is.

## FTDI wiring for flashing (VCC disconnected)

Flash through the FTDI on UART0 with the **FTDI switch set to 3.3V**:

| FTDI | -> | WT32-ETH01 |
|---|---|---|
| GND | -> | GND |
| TXD | -> | RX0  (silk "RXO") |
| RXD | -> | TX0  (silk "TXO") |

Plus the bootloader jumper **IO0 -> GND**.

> **Leave FTDI VCC disconnected.** Power the board from a 5V source (the buck or a USB charger) into the WT32 **5V** pin. Backfeeding the 3.3V rail from FTDI VCC sags it and stops the Ethernet PHY from coming up. See `../docs/troubleshooting.md`.
>
> Also **disconnect the GPS while flashing** (IO5 is a strapping pin).

## Manual bootloader steps

The WT32-ETH01 has no USB and no auto-reset, so you enter the bootloader by hand:

1. With **IO0 jumpered to GND**, click **Upload** in the Arduino IDE.
2. When the IDE prints `Connecting....`, **power-cycle the board**. It catches the chip in download mode and flashes.
3. When it finishes, **remove the IO0 jumper** and power-cycle again to run.

## Bench validation

Open the **Serial Monitor at 115200 baud**. You should see:

- the startup banner,
- a `[status]` heartbeat line every 5 seconds (so the console shows life even before a GPS fix or an Ethernet link exists),
- once the GPS is wired (UART2) and talking, `$GP...` sentences scrolling and the "sentences forwarded" counter climbing,
- once the Ethernet is plugged into any DHCP LAN, the status line flipping to `link up` with an IP, and a one-time line printing the board's **IP and MAC**.

The firmware prints **your board's MAC** at boot once it has a link; use that value if you want to set a DHCP reservation on the router. (Do not copy a MAC from anyone else's writeup; it is unique to each board.)

If the Serial Monitor shows gibberish, it is set to the wrong baud. The GPS link is 9600; the console is 115200. Do not mix them up.
