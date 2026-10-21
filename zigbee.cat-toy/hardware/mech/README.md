# Printed parts

Two headless FreeCAD scripts: `pod.py` builds a bench pod for the board, `ball.py` the rolling ball.

## Bench pod

Build:

```bash
hardware/mech/pod.sh 503040 --check     # or 103450; --check loads the board STEP and reports any overlap
```

Output: `build/pod_<cell>/{cradle,lid,battery_box}.{step,stl}`. Previews: `openscad -D 'part="assembly"' view.scad`.

Frame: the board's STEP frame. X along the board's long edge, Y up (= minus the KiCad board y), Z up,
board bottom at z = 0, board top at z = 1.6, origin at the board's top-left corner.

### Parts

| Part | What | Print |
|---|---|---|
| `cradle` | Board carrier. Floor plate 2 mm under the board (JST pins protrude 1.8), two M2 bosses at the board holes, 1 mm ledges under both long edges, 1.2 mm walls to z = 7 (JST housings are 6.4 tall). Open connector bay 12 mm beyond the left edge for the three JST plugs, with two wire notches in its end wall. Notch in the top wall and floor at x 17.5…32 for the antenna lead, or a MINI-1's antenna. 1 mm faceplate on the USB-C edge with: USB-C slot 10 × 4.6, LED window 3 × 1.6, and a 0.6 mm thick flexure tab (5 × 7) hinged at its top over the side button, with a nub inside that reaches the plunger and a dome outside. | floor down, no supports |
| `lid` | 1.2 mm plate with an inner lip, friction fit. Lip relieved beside the motor JST. For a bench unit; toys may not need it. | flat side down |
| `battery_box` | Open-top box with a rim that takes the cradle's floor plate like a lid. Cavity = cell + 1 mm each way, with a stop rib so the cell sits under the board and the lead loops up through the slot in the cradle floor. `503040` (5 × 30 × 40, 500 mAh) or `103450` (10 × 34 × 50, 1000 mAh). | floor down |

Screws: M2 × 5 button head (ISO 7380) through the board into the bosses (1.7 mm thread-forming holes).

### Numbers the toy bodies need

- **Faceplate window:** 28.9 (Y) × 11.4 (Z) mm, Y −27.45…1.45, z −3.2…8.2, outer face at x = 41.15. The toy body gets this rectangular opening and the faceplate shows through it; the USB-C face sits 0.2 mm behind the faceplate's outer face.
- **Cradle envelope:** 54.95 × 31.6 (503040) or 35.6 (103450) × 11.4 mm, x −13.2…41.75 (the 0.6 mm button dome is the +x extreme).
- **Stack with battery box:** 18.6 mm (503040) or 23.6 mm (103450) from box bottom to lid top.
- **Antenna:** the FPC antenna on the u.FL lead (x 19.9, Y −2.6) leaves through the notch at x 17.5…32. Stick it to the toy's inside wall, away from the motor and the battery. With a MINI-1 instead, its antenna overhangs the top edge by 3.1 mm into the same notch; keep plastic a few mm away from it.
- **LED:** the status LED (0603) sits 2.9 mm in from the USB-C edge and lights upward, so the faceplate window shows only its spill. A light pipe or a lid window above it (x 35.5…37.1, Y −5.9…−6.7) shows it properly.
- **Wires:** motor and speaker leave the bay through the notches at Y −4 and Y −11 (3.2 wide, from z 2 up); the battery lead comes up through the floor slot at x −8…−2, Y −20…−17.

### Checks

`--check` reads `hardware/zigbee_toy/build/zigbee_toy.step` (export it with `kicad-cli pcb export step --subst-models`), expands it into placed solids and intersects every one with cradle + lid. It must print `CLEAN`; the board's own bottom copper skin is ignored. The STEP has the 1U module, so a box standing in for a MINI-1's antenna overhang gets the same check (`CLEAR`).

### Print settings assumed

FDM, 0.4 mm nozzle, PETG, 0.2 mm layers, 3 perimeters. Clearances: board outline 0.25 mm, faceplate-to-board 0.15, lid lip 0.2, battery 1.0.

## Rolling ball

Build: `hardware/mech/ball.sh --check`. Output in `build/ball_803040/` as STEP + STL per part. `ball_view.scad` only imports the exported STLs, so `openscad` can write preview PNGs headless (`openscad -o x.png -D 'part="exploded"' ball_view.scad`, parts `exploded | chassis | core | core_open | core_top | section | joint | closed`).

Frame: Z is the pole axis, dome B (the one you unscrew to charge) at +Z, the motor dome A at −Z. Gravity pulls the pendulum toward −Y. The board stands at Y −7.5 with its components facing −Y and the USB-C edge toward +Z.

### How it works

The N20 gearmotor lies on the pole axis and is the axle: its 3 mm D shaft is pressed into dome A's hub, and a Ø4 steel pin in the chassis post rides in dome B's hub. Everything else hangs below the axis as a pendulum, closed inside a printed core (cup + cap) so that with dome B off only the USB-C window and a pinhole over the pairing button show. When the motor runs, the shell turns relative to the pendulum, the pendulum swings up and the ball rolls. At 100 rpm the motor can lift the pendulum over the top, so rolling speed is set by PWM duty in firmware.

### Parts

| Part | What | Print |
|---|---|---|
| `dome_a` | Motor-side half, Ø80, 1.6 wall. Rim thickened to 9 mm with the internal M70 × 3 thread (2 turns), 45° hub cone with the D socket for the motor shaft, fabric slot + 24 stitch holes at the groove face, Ø8 pole flat. | rim down, 0.2 mm layers, no supports |
| `dome_b` | USB-side half. 8 mm lip with the external thread, 4.8 mm smooth land for the ring, 3.5 mm shoulder, hub cone with the Ø4.3 bore for the pin, fabric slot + stitch holes at the groove face, pole flat. | rim down |
| `ring` | Tail ring, ID 70.6, OD 76, 4.2 wide, lug with a cord hole. Free-spins on dome B's land, 2 mm below the rolling surface. | flat |
| `core_cup` | Ø63 cylinder ∩ Ø74 sphere, 1.6 wall, open at the top (Z 12) with a Ø28 opening at the bottom round dome A's hub. Two rails per side (1.2 mm ribs, 2.6 mm channel) take the frame plate; three M2 bosses inside the rim for the cap. | opening up |
| `core_cap` | 1.6 mm plate with a 10.6 mm skirt. USB-C window 13 × 7 (the receptacle face sits 1.7 mm behind the cap face, so the plug overmould enters the window), Ø1.3 pinhole over SW1's plunger (SIM tool), 6.8 × 5.8 hole for the pin post, three Ø2.2 screw holes. | plate down |
| `frame` | 2 mm plate (Y −4…−2) spanning the cup, motor window with cheeks whose 1.15 mm ledges carry the can, 1.5 mm face plate with the Ø4.3 boss hole and two Ø1.8 holes 9 mm apart (M1.6), post with the Ø3.9 pin bore at the +Z end, two M2 standoffs for the board (3.5 mm), two clearance holes for the tray screws. | plate down |
| `tray` | Battery tray under the board: 1.2 mm walls and floor, open toward +Z and on the long sides, two posts on the +X side screwed up into the plate, 14 × 30 pocket under the floor for a 2.5 mm steel ballast plate. | floor down |

Not printed: N20 gearmotor (6 V winding, ~100 rpm), Ø4 × 7.5 mm steel pin (dowel or a cut M4 shank), two M2 × 5 for the board, two M2 × 6 for the tray, three M2 × 6 for the cap (1.7 mm holes, thread-forming), two M1.6 × 4 for the motor face, an 803040 cell (8 × 30 × 40, 1000 mAh) with a JST-PH lead, a 14 × 30 × 2.5 steel plate (8 g, optional), a motor pigtail to J4.

### Sizes

- Ball Ø80, wall 1.6. Thread: lip core Ø70, tooth 1.6 deep, pitch 3, 2 turns, 0.3 mm radial clearance. Groove 4.8 mm wide, ring 0.3 mm each side.
- Fabric: each groove face has a 0.8 mm wide, 2.5 mm deep slot at r 38.2…39 and 24 Ø1.2 radial holes 1.5 mm behind the face. Tuck the cover's edge into the slot and stitch through the radial holes; the seam ends right at the groove, nothing flaps, and the stitches hide under the ring.
- Core clearances: 1.5 mm to dome B's lip, 1.4 mm to the domes, 0.3 mm around the chassis. The core never touches the shell.
- Only the 803040 fits under the axis; `BALL_CELL=103450` builds but fails the check.
- Pendulum (printed core + frame + tray 40 g, cell 20, motor 10, board 9): 79 g, CG 7.5 mm below the axis, 59 g·cm. With the ballast plate: 87 g, CG 9.4 mm, 82 g·cm. Printed mass of the whole ball 99 g (dome A 24, dome B 32, ring 3, cup 20, cap 9, frame 7, tray 4), total about 150 g.

### Assembly

1. Press the pin into the frame post. Screw the board to the standoffs (from the component side), plug the motor pigtail into J4 and the cell into J3. Trim J3–J5's pins short: they sit 0.7 mm above the motor can.
2. Drop the cell into the tray, slide the ballast plate into its pocket, screw the tray up into the plate.
3. Seat the motor on the cheek ledges with its boss in the face-plate hole; two M1.6 screws through the face plate.
4. Slide the chassis into the cup along the rails, plate edges in the channels, until it bottoms. Fit the cap over the post (USB window over J1) and drive the three M2 screws.
5. Push the motor shaft into dome A's D socket through the cup's bottom opening.
6. Slip the ring over dome B's lip, screw dome B onto dome A; its hub picks up the pin on the last turn.
7. Sew the fuzzy covers into the slots at each rim, gather and glue at the poles. The ring and groove stay bare.

### Checks

`--check` verifies that chassis, tray, cell, motor, ballast and every solid of the board STEP stay inside the core with 0.25 mm to spare, that the core stays inside the domes and clear of the lip, that no two parts overlap (including each board component against the chassis, motor, cup and cap), that the ring and the two domes don't intersect when assembled, and that a MINI-1's antenna overhang (not in the STEP) clears the chassis and the core. It must print `CLEAN`.
