# Restoring GPS to a Peplink B One 5G (and similar) over raw NMEA

A cheap, reliable way to give a Peplink router a live GPS position again by feeding it raw NMEA 0183 over the LAN. A small ESP32-with-Ethernet board reads a real GPS receiver and pushes the stream to the router. Total parts cost is roughly $40, less if you already own a USB-to-serial adapter.

> **Licenses:** firmware in `firmware/` is MIT (`LICENSE`). The 3D models in `enclosure/`, the documentation in `docs/`, and this README are CC BY-NC-SA 4.0 (`LICENSE-hardware`). The enclosure is a remix of upstream NonCommercial work, so the NonCommercial terms apply to the models and docs; the firmware is original and stays MIT.

## The problem

Some Peplink installs used to get their GPS position from an upstream source (for example a Starlink network GPS feed). When that feed goes away, Peplink InControl no longer shows where the rig is or where it has been. The Peplink B One 5G has **no internal GPS chip**, but it will accept a **raw NMEA 0183 stream over the LAN**. This project is a tiny device that reads a real GPS receiver and pushes that NMEA to the router. It is built and tested on a B One 5G (firmware 8.5.x) and should apply to other Peplink models that expose the same "GPS Receiver for Raw NMEA 0183 Network Stream" feature.

## How it works

```
Roof GPS antenna (active, SMA)
        |
        v
NEO-7M GPS receiver  --UART / NMEA 9600-->  WT32-ETH01 (ESP32 + wired Ethernet)
                                                  |
                                                  |  NMEA over UDP (or TCP)
                                                  v
                                          Peplink LAN port
```

**The #1 conceptual gotcha: who listens and who pushes.**

- The **Peplink is the listener (the server).** It opens a port and waits for a stream to arrive. The "Sender IP" box in its config is an *allowlist of who may push*, not a target it reaches out to.
- The **ESP32 is the client that pushes.** It opens the connection and sends NMEA to the Peplink's LAN IP and port.

Get this backwards and nothing works. This also explains the older "TCP did not work" reports floating around: those used a TCP *server* as the source, which is the wrong role. An ESP acting as a TCP *client* is correct, and UDP is simpler still and is the documented-working case on the B One 5G.

## Bill of materials

Listed by part type. No vendor links on purpose; buy from whoever you trust. Costs are rough, in USD.

| Part type | Role | Approx cost |
|---|---|---|
| **WT32-ETH01** (ESP32 + onboard LAN8720 wired Ethernet) | The brains. Reads the GPS, pushes NMEA over the LAN. | ~$16 |
| **u-blox NEO-7M GPS module** with an SMA jack | The GPS receiver. Outputs NMEA over UART. | ~$11 |
| **Active GPS antenna with SMA** | The real antenna. On an RV this is your roof unit. | varies |
| **12V to 5V buck converter, ~3A** | Powers the stack from the coach 12V. | ~$5 to $13 |
| **Dupont jumper wires (female-to-female)** | Join the boards (both have male pins). | ~$8 kit |
| **USB-to-TTL serial adapter** (FTDI FT232 or similar, with a 3.3V/5V switch) | Flashing the ESP32 and bench-testing the GPS. | ~$8 |
| **Inline fuse holder + ~1A fuse** | Safety on the 12V tap for a permanent install. | ~$5 |
| **Short Ethernet patch cable** | WT32 to the Peplink LAN. | likely on hand |

Notes on choices:

- The **NEO-7M** is plenty for a moving-map feed (GPS + GLONASS, around 2.5 m accuracy). A fancier multi-band receiver buys nothing here, because most RV roof antennas are L1-only. Clear sky and a good antenna matter far more than the chipset.
- **Do not rely on the NEO-7M's onboard ceramic patch antenna.** On many of these boards the RF front end is routed to the **SMA jack**, not the patch, so with nothing on the SMA the module looks deaf (zero or one satellite, no fix) even under open sky. This is a known pattern, corroborated by multiple buyer reviews on the module's listings, not a one-off defect. Plan on using the **SMA with an external active antenna** from the start. On an RV, that external antenna is your roof unit, which is what you want anyway.
- Your **active roof antenna** contains an amplifier and expects a DC **bias voltage** down the coax (commonly 3.3V). Match the GPS module's antenna voltage to your antenna's rating. This build feeds the GPS at 3.3V to keep a 3.3V-bias antenna happy. A worked example: the **Peplink Mobility 42G** is L1-only with a 3.3V bias.

## Tools and software

- A computer with the **Arduino IDE** (free).
- The **ESP32 board package** for Arduino (install steps in `firmware/README.md`).
- A serial terminal for watching NMEA during bring-up (the Arduino Serial Monitor is fine). Note: u-blox u-center is Windows-only, so on macOS or Linux do antenna/config tweaks in firmware rather than in u-center.
- Basic hand tools and a fused 12V source for the final install.

## Wiring

Full detail, pin by pin and stage by stage, is in **`docs/wiring.md`**. In brief:

- There are three wiring stages and you do not need them all at once: **(A)** bench-test the GPS by itself over the FTDI, **(B)** flash the WT32-ETH01 over the FTDI on UART0, **(C)** operation, with the GPS on UART2.
- The GPS talks to the WT32 on **UART2**: GPS **TXD -> IO5** (board silk "RXD"), and optionally GPS **RXD -> IO17** (board silk "TXD"). The IO5 wire is the one that carries data; IO17 is only needed if you want to send config to the GPS.
- Power the GPS from the WT32's **3V3** output pin, not 5V, so an active 3.3V-bias antenna sees its rated voltage.
- During flashing (stage B) you power the board from the **FTDI's VCC into the WT32 3V3 pin**, with the FTDI switch at 3.3V; the Ethernet PHY is idle then, so this is fine. When you switch to operation (stage C) you power the WT32 from the buck's **5V** into the WT32 **5V** pin, and you **remove the FTDI VCC wire**. The PHY is power-hungry and needs a solid, single 5V source, so two supplies on the rail are what to avoid.
- Tap coach 12V through an **inline ~1A fuse** into the buck.

## Firmware and flashing

The sketch lives in **`firmware/`** (`gps_nmea_bridge.ino`), with flashing-specific instructions in **`firmware/README.md`**.

Two things trip people up:

- **The WT32-ETH01 has no USB port and no auto-reset circuit.** You flash it through the FTDI on UART0, and you enter the bootloader by hand: jumper **IO0 -> GND**, click Upload, and when the IDE prints `Connecting....`, **power-cycle the board**. It catches it in download mode. Remove the IO0 jumper and power-cycle again to run.
- **Two different bauds, do not mix them up.** **9600** is the internal GPS-to-ESP link (UART2). **115200** is the USB debug console (the Serial Monitor). Gibberish on the monitor almost always means it is set to the wrong baud.

Power during flashing comes from the FTDI: with the FTDI switch at 3.3V, run **FTDI VCC into the WT32 3V3 pin**. That is fine while flashing because the Ethernet PHY is idle. When you move to operation and feed the buck's 5V into the 5V pin, **remove the FTDI VCC wire** so two supplies are not fighting on the rail. Details are in `firmware/README.md`.

Settings you edit at the top of the sketch: `PEPLINK_IP` (your Peplink's LAN IP), `PEPLINK_PORT`, and `USE_TCP` (0 for UDP, the default; 1 for a TCP client).

## Peplink configuration

Router-side setup is in **`docs/peplink-config.md`**. In brief: **Advanced > "GPS Receiver for Raw NMEA 0183 Network Stream"**, enable it, set **Protocol UDP** and **Port 10110**, and add the ESP's IP to the sender allowlist. Then confirm reception on the router's **Status page**, which has a live GPS map at the bottom that updates **before** InControl does.

## Bring-up and troubleshooting

A walkthrough of the snags worth expecting, and why, is in **`docs/troubleshooting.md`**. The short list: listener-vs-client roles, the SMA-vs-patch antenna trap, the manual bootloader, the **FTDI VCC backfeed that starves the Ethernet PHY and leaves you with no link light** if you leave it connected after switching to 5V, the IO5 strapping pin, antenna bias voltage, and the fact that **InControl lags by minutes, which is normal, so confirm on the Status-page map instead**.

## Enclosure and STLs

Printable STLs are in **`enclosure/`**. The starting point was a small box for the WT32-ETH01 plus the FTDI; the GPS would not fit. The published case is **widened by 30mm along one axis** to open a bay for the NEO-7M next to the existing port wall, so every connector keeps its position.

- Final printable files: `enclosure/Case_wide_30mm_clean.stl` and `enclosure/Lid_wide_30mm.stl`.
- The GPS can be **fully enclosed** with no view of the sky, because it runs off the roof antenna over the SMA, not its onboard patch.
- See `enclosure/README.md` for print orientation, the credit lineage, and a note that these are mesh-only (no parametric CAD source).

## License

- **Firmware** (`firmware/`): MIT, see `LICENSE`.
- **3D models and documentation** (`enclosure/`, `docs/`, this README): Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0), see `LICENSE-hardware`. The enclosure is a remix of upstream NonCommercial work, so these may not be used commercially.

## Credits and sources

- **WT32-ETH01 pinout and Ethernet bring-up**: Werner Rothschopf's excellent WT32-ETH01 reference, which nails the second-UART pins (IO5 / IO17) and the LAN8720 setup. https://werner.rothschopf.net/microcontroller/202401_esp32_wt32_eth01_en.htm
- **The UDP-working case on the B One 5G**: a Peplink community forum thread documenting a working raw-NMEA-over-UDP feed to a B One 5G, which confirmed UDP as the simplest reliable transport and the listener-vs-client roles.
- **Enclosure lineage**: the case is a remix of **Jahara**'s "WT32-ETH01 FT232 Snap Fit Case" on MakerWorld (https://makerworld.com/en/models/1920953-wt32-eth01-ft232-snap-fit-case), which is itself a remix of **Hans** (@Hans_193091)'s "Esp32 WT32-ETH01 USB Type C Snap Fit Case" on Printables (https://www.printables.com/model/770916-esp32-wt32-eth01-usb-type-c-snap-fit-case). Both are licensed CC BY-NC-SA 4.0. See `enclosure/README.md` and `LICENSE-hardware`.
- Built and documented for a motorhome install. Shared in case it saves the next person the same afternoon of debugging.
