# Wiring

There are three wiring stages. You do not need them all wired at once: stage A proves the GPS, stage B flashes the ESP32, stage C is the running configuration. The same FTDI adapter is reused across A and B.

> Throughout, set the **FTDI's 3.3V/5V switch to 3.3V**. The ESP32 and the GPS are 3.3V-logic parts.

## Pin references

**NEO-7M GPS header** (left to right):

```
VCC   GND   TXD   RXD   PPS
```

**FTDI FT232 header** (order varies by board, match by silk label). The pins you use:

```
... RXD   TXD   GND   VCC
```

Plus a **3.3V/5V slide switch** (use 3.3V).

**WT32-ETH01**, the pins this build touches:

- **UART0** (flashing and the USB debug console): `TX0` (silk "TXO"), `RX0` (silk "RXO")
- **UART2** (the GPS data link): `IO5` (silk "RXD"), `IO17` (silk "TXD")
- Power and control: `5V`, `3V3` (regulator output), `GND`, `IO0` (hold low to enter flash mode)

The IO5 / IO17 mapping for the right-side `RXD` / `TXD` header is from the Werner Rothschopf WT32-ETH01 reference. Those two pins also drive small active-low green LEDs on the board, which is harmless and gives you a handy data-activity blink.

## Stage A (optional): bench-test the GPS by itself

This proves the GPS module works before you involve the ESP32. **Set the FTDI switch to 3.3V.**

| FTDI | -> | NEO-7M |
|---|---|---|
| VCC | -> | VCC |
| GND | -> | GND |
| RXD | -> | TXD  (GPS talks, FTDI listens) |
| TXD | -> | RXD  (optional, only needed to send config) |

Leave **PPS** open. TX/RX cross over: the GPS's TXD goes to the FTDI's RXD.

Put an **active antenna on the SMA jack** (see the antenna note below), plug the FTDI into the computer, and open a serial terminal at **9600 baud**. You will see `$GPRMC,...` and friends. With no fix the RMC carries a `V` (void) with empty coordinates; once it locks, the status flips to `A` (active) with real latitude and longitude, and `$GPGGA` shows a rising satellite count.

> **Antenna, important:** do not trust the onboard ceramic patch. On many NEO-7M boards the RF is routed to the **SMA**, not the patch, so the module looks dead with nothing on the SMA even under open sky. This is a known pattern across these boards, not a one-off. Use an external active antenna on the SMA. See `troubleshooting.md` for the full story.

## Stage B: flash the WT32-ETH01

The WT32-ETH01 has **no USB port**, so you flash it through the FTDI on UART0. **Disconnect the GPS for this step** (IO5 is a strapping pin, see troubleshooting). **Set the FTDI switch to 3.3V.**

| FTDI | -> | WT32-ETH01 |
|---|---|---|
| VCC | -> | 3V3  (powers the board for flashing) |
| GND | -> | GND |
| TXD | -> | RX0  (silk "RXO") |
| RXD | -> | TX0  (silk "TXO") |

Plus a jumper: **IO0 -> GND** (this is what selects flash mode). TX and RX cross over.

> **Power the board from the FTDI to flash, then disconnect FTDI VCC when you switch to 5V for operation.** With the FTDI switch at 3.3V, feed **FTDI VCC into the WT32 3V3 pin** to power the board for flashing. This is fine because the Ethernet PHY is idle while flashing, so the modest current the FTDI supplies is enough. The critical step is the transition to operation: when you move to stage C and power the board from the buck's **5V** into the **5V** pin, you must **remove the FTDI VCC wire** (keep GND/TX/RX if you still want the serial console). Leaving FTDI VCC connected puts two supplies on the rail, the FTDI's 3.3V and the buck's 5V through the onboard regulator, and they backfeed and sag it. The power-hungry LAN8720 PHY is then starved and the Ethernet link never comes up. That is the classic "everything flashes fine but the link light never comes on" failure. See `troubleshooting.md`.

For the manual-bootloader keystrokes, see `../firmware/README.md`.

## Stage C: operation, GPS to WT32

After flashing, remove the IO0 jumper, remove the **FTDI VCC wire**, and wire the GPS to UART2.

| NEO-7M | -> | WT32-ETH01 |
|---|---|---|
| VCC | -> | 3V3  (powers the GPS at 3.3V) |
| GND | -> | GND |
| TXD | -> | IO5  (silk "RXD")  <- the data wire that matters |
| RXD | -> | IO17 (silk "TXD")  (optional, config only) |

Leave **PPS** open. TX and RX cross over here too: the GPS's TXD goes to IO5.

Power the WT32 from the buck's **5V** into the WT32 **5V** pin, with the FTDI VCC wire removed so only one supply feeds the rail. Power the GPS from the WT32's **3V3** output pin, not 5V. That keeps an active 3.3V-bias antenna at its rated voltage. The chain is: **5V in -> WT32 -> 3V3 out -> GPS.** If you keep the FTDI attached for the serial console, leave only GND/TX/RX connected, never VCC.

## Final install power

```
Coach 12V  --(inline ~1A fuse)-->  buck converter  --5V-->  WT32-ETH01 (5V pin)
                                                            WT32 3V3 out  -->  GPS VCC
                                                            WT32 Ethernet -->  Peplink LAN port
Roof antenna (SMA male)  -->  GPS SMA jack
```

- The **fuse** protects the harness against a 12V short; size it small (around 1A) since the whole stack draws very little.
- Unscrew any bundled ceramic puck from the GPS SMA jack and connect the **roof antenna** there.
- Because the GPS runs entirely off the roof antenna, the electronics can live anywhere convenient and be fully enclosed; they need no view of the sky.
