# Troubleshooting and gotchas

These are the snags worth expecting on this build, with the reason behind each. Most of them are easy once you know to expect them.

## 1. Listener vs client (the conceptual one)

The **Peplink is the listener (server)**: it opens the port and waits. The **ESP is the client that pushes** the stream to the router's LAN IP and port. The "Sender IP" field on the router is an *allowlist of who may push*, not a target it pulls from.

This is the root of the old "TCP did not work on the B One" reports: those setups used a TCP *server* as the GPS source, which is the wrong role for this feature. An ESP acting as a TCP *client* is correct. UDP is simpler still and is the documented-working case, so start there.

## 2. The onboard patch antenna looks dead (SMA vs patch)

Symptom: under open sky, the NEO-7M sees zero satellites, or one satellite intermittently, and never gets a fix, even after 15 minutes outdoors away from the laptop.

Cause: on many of these NEO-7M boards the RF front end is routed to the **SMA jack, not the onboard ceramic patch**. With nothing on the SMA the receiver is effectively antenna-less, and any momentary single-satellite blips are stray coupling, not real reception. This is a **known issue with these boards**, corroborated by multiple buyer reviews on the module's listings, not a defect unique to one unit.

Fix: **do not rely on the onboard patch. Use the SMA with an external active antenna.** On an RV that external antenna is your roof unit, which is what you want anyway. If you ever need to confirm the routing on the bench, put a known-good active antenna on the SMA and you should fix within a minute or two of clear sky.

## 3. No USB and no auto-reset (manual bootloader)

The WT32-ETH01 has **no USB port** and **no auto-reset circuit**, so you cannot just click Upload and walk away. Flash it through the FTDI on UART0 and enter the bootloader by hand:

1. Jumper **IO0 -> GND**.
2. In the Arduino IDE, click **Upload**.
3. When the IDE prints `Connecting....`, **power-cycle the board** (pull and restore its power). It catches the chip in download mode and flashes.
4. When the flash finishes, **remove the IO0 jumper** and power-cycle again to run normally.

## 4. FTDI VCC left connected during operation backfeeds the rail (the nasty one)

Symptom: the board flashes perfectly, the firmware runs, the serial console looks healthy, but the **Ethernet link light never comes on** and you never get a DHCP IP.

Cause: during flashing it is correct to power the board from the **FTDI's VCC into the 3V3 pin** (the Ethernet PHY is idle then, so the FTDI supplies enough current). The trouble starts at the transition to operation. When you bring in the buck's **5V** on the 5V pin but leave the **FTDI VCC wire still connected**, two supplies share the rail: the FTDI's 3.3V and the buck's 5V through the onboard regulator. They backfeed and **sag the rail**. The LAN8720 Ethernet PHY is power-hungry, and on that sagging rail it never comes up, so you get no link.

Fix: when you switch to 5V for operation, **remove the FTDI VCC wire** (keep GND/TX/RX if you want the serial console) and power the board from a solid **5V** source (the buck, or a USB charger) into the WT32's **5V** pin. With FTDI VCC removed and clean 5V in, the board pulls DHCP immediately and the Peplink logs the port link-up at 100 Mbps full duplex.

The rule is "power from the FTDI to flash, then disconnect FTDI VCC when you switch to 5V for operation," not "never connect FTDI VCC." This is the single most common "it all works except Ethernet" failure on this board. If your link light is dark after you have moved to 5V, check this first.

## 5. IO5 is a strapping pin

GPIO5 (the GPS data line, board silk "RXD") is an ESP32 **boot-strapping pin**. If the GPS is connected and driving that line while the board powers up to flash, it can interfere with entering the bootloader. **Disconnect the GPS while flashing** (stage B), and only wire it to UART2 afterward (stage C). As a bonus, IO5 and IO17 drive small onboard green LEDs, so once the GPS is connected and talking you get a free data-activity blink.

## 6. Match the antenna bias voltage (3.3V)

An active GPS antenna contains an amplifier fed DC **bias voltage** down the coax, and it has a rated voltage (commonly **3.3V**). Power the GPS module from the WT32's **3V3** output pin, not 5V, so the bias the module puts on the SMA matches the antenna's rating. For example, the Peplink Mobility 42G is a 3.3V-bias, L1-only antenna, so 3.3V is correct for it. Feeding 5V to a 3.3V antenna is the kind of thing that works for a while and then does not.

## 7. InControl lags, watch the Status-page map instead

Symptom: the build is running, the router clearly has a fix, but **InControl is not updating**.

This is almost always **normal reporting lag**, not a fault. InControl updates on a cloud reporting interval and can trail the live position by minutes.

Fix: confirm reception on the router's own **Status page**, which has a **live GPS map at the bottom** that updates within seconds and **ahead of InControl**. If the Status-page map shows your position, the feed is working; InControl will catch up on its own schedule. (Note the Event Log shows only the LAN port link-up, not the NMEA content, so it is not the place to confirm the feed.)

## 8. Two bauds, do not mix them up

**9600** is the internal GPS-to-ESP link on UART2. **115200** is the USB debug console (the Serial Monitor). If the Serial Monitor shows gibberish, it is almost always set to the wrong baud; set it to 115200.
