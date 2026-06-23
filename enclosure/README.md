# Enclosure

3D-printable case and lid for the electronics. License: CC BY-SA 4.0 (see `../LICENSE-hardware`).

## Printable files

- **`Case_wide_30mm_clean.stl`** — the case.
- **`Lid_wide_30mm.stl`** — the lid.

These are the final, intended files. Print these two.

## What this is

The starting point was a small box sized for the WT32-ETH01 plus an FTDI adapter, with the connector openings on one long wall. That box had **no room for the NEO-7M GPS**. This case is that box **widened by 30mm along the axis the connector wall runs on**, which opens a clean bay at the far end for the GPS while keeping every existing connector opening in its original position. The interior is a single open volume (an internal partition wall from the original design was removed), so you have room to mount the GPS and route the SMA cable.

The GPS can be **fully enclosed** with no opening to the sky, because it runs off the roof antenna over the SMA, not its onboard patch. You only need a cable exit for the SMA pigtail and the power/Ethernet leads.

## Printing

- **Orientation:** print the case open-side up (lid opening facing up) so the walls and connector openings come out clean and the floor is on the bed. The lid prints flat, outer face down. Supports are generally not needed for the case in this orientation.
- These are **mesh-only STLs.** There is **no parametric CAD source**, so edits have to be done at the mesh level (a mesh editor or a boolean/CSG tool), not by tweaking parameters. If you want a different layout, expect to remix the mesh.

## What is intentionally omitted

The original narrower case and the intermediate widened versions are **not included on purpose**, to avoid confusion about which file to print. The two files above supersede all of them. If you specifically want the GPS-less original box for a different build, it is straightforward to print just the WT32 + FTDI footprint, but this repo ships only the final motorhome-GPS enclosure.
