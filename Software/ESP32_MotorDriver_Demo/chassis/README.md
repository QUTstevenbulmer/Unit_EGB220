# EGB220 robot chassis (laser cut)

| File | What |
|---|---|
| `chassis_cut.dxf` | Cut file, mm. Layer **CUT** = cut, layer **ENGRAVE** = optional labels |
| `chassis_cut.svg` | Same cut lines as SVG (red, 0.1 mm), for Inkscape / Glowforge / LightBurn |
| `chassis_preview.png` | Top view with motors, wheels, BOB and battery drawn in |
| `make_chassis.py` | Generator: change the parameters at the top and re-run |

Sheet: 3 mm acrylic or MDF/ply, about 162 × 178 mm. Parts: 1 base, 4 motor clamps, 3 caster spacers.

**BOB holes are placeholders** (56 × 85 mm centres). Check them against the real PCB and change
`BOB_HOLE_DX`, `BOB_HOLE_DY` and `BOB_CENTRE_Y` before cutting. Then run `python make_chassis.py`
(`pip install ezdxf shapely matplotlib`).

## Assembly

1. **Test-cut a clamp first.** The N20 should press into the clamp notch snugly, and the clamp tabs
   should push firmly into the base slots. Adjust `NOTCH_FIT` / `SLOT_FIT` for your cutter's kerf.
2. **Motors on top.** Slide two clamps onto each N20: one on the gearbox, one on the motor can. Push the
   tabs into the slots, with shafts pointing outwards. Glue the tabs (acrylic cement / CA) or add
   a zip tie through the small slots over each motor. Motor **A = left** when looking forwards.
3. **Wheels** push onto the shafts outside the plate.
4. **Caster underneath** at the rear. With 43 mm wheels the underside of the plate is **13.5 mm**
   above the floor (34 mm wheels: about 9 mm). Make up the caster height with the spacer plates
   and/or M3 standoffs. The slots fit caster hole spacings of 12–32 mm.
5. **18650 holder** on top at the rear: screw through the slots and/or use a 20 mm velcro strap
   through the two strap slots.
6. **BOB** on M3 standoffs, 20 mm or taller, so it clears the motors (clamps stand 13.2 mm high).
7. **Line sensor** under the front tongue on short standoffs, 3–8 mm off the floor. The two slot rows fit
   hole spacings of 16–60 mm. Bring the cable up through the oval slot.
8. Spare M3 holes in the corners are for bumpers, switches and so on.

After assembly: Test & debug page → **Check motors & encoders** → **Measure top speed**.
