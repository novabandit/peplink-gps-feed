# Peplink configuration

Tested on a Peplink B One 5G running firmware 8.5.x. Other Peplink models that expose the same feature should behave the same way.

## Enable the NMEA stream listener

Open the router's web admin and go to:

**Advanced > "GPS Receiver for Raw NMEA 0183 Network Stream"**

Enable it and set:

- **Protocol: UDP.** This matches the firmware default and is the documented-working transport on the B One 5G. TCP also works if you set `USE_TCP 1` in the firmware and reflash; in that case the ESP connects as a TCP *client*. Do not configure the Peplink as a TCP client pulling from the ESP, the router is always the listener.
- **Port: 10110.** This is the registered NMEA-0183-over-IP port and is a sensible public default. It must match `PEPLINK_PORT` in the firmware. Any free port works as long as both sides agree.
- **Access Control / Sender IP allowlist:** allow the ESP's IP to push to the router. Two approaches both work:
  - **Single host (/32):** reserve a DHCP address for the ESP (by its MAC) and allowlist exactly that one IP. Tightest, but you have to set up the reservation.
  - **Subnet (/24):** allowlist `<ESP_IP> / 255.255.255.0`, which tolerates the ESP's IP drifting anywhere inside `192.168.50.0/24`. This is what this build used in practice, and it means a DHCP reservation is optional.

The firmware prints the board's IP and MAC on the USB serial console at boot, so you have the values you need for either approach. Use **your** Peplink's LAN IP for `PEPLINK_IP` in the firmware (the examples here use `192.168.50.1`, the Peplink default).

## What the router wants

The router is happy with **RMC and GGA** (together they cover position, time, fix quality and satellite count). The firmware forwards every NMEA sentence the GPS emits, which is harmless; the router simply ignores what it does not use.

## Confirming reception

There are two readouts, and they update at very different speeds:

- **The Status-page GPS map (fast).** On the B One 5G, the main **Status** page has a **live GPS location map at the bottom**. Scroll down to find it. This map reflects the incoming fix within seconds of a good stream arriving, and it updates **before** InControl does. This is the right place to confirm the router is receiving your feed.
- **InControl (slow).** InControl shows the same position eventually, but it lags by minutes because of normal cloud reporting intervals. Do not stand at the rig waiting on InControl to decide whether the build works. Watch the Status-page map.

The Peplink **Event Log** records the LAN port coming up (for example "Port link up, 100 Mbps full duplex") but does **not** log NMEA stream content, so the Event Log confirms the cable and link, not the feed. Use the Status-page map for the feed itself.
