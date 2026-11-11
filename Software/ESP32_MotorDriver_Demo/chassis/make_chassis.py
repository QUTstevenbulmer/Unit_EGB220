#!/usr/bin/env python3
"""
EGB220 robot chassis - laser-cut parts generator.

Writes:
  chassis_cut.dxf     parts for the laser cutter (mm). Layer CUT = cut,
                      layer ENGRAVE = optional labels (delete if not wanted).
  chassis_preview.png top view with the motors, wheels, board and battery
                      drawn in (preview only - not in the DXF).

Change the PARAMETERS below (e.g. real BOB hole positions) and run:
    pip install ezdxf shapely matplotlib
    python make_chassis.py

Design (top view, Y = forward, X = right, origin = rear centre of plate):
  * N20 motors sit ON TOP of the base, each held by two slot-in clamps that
    grip the 12 x 10 mm gearbox / motor body. Shafts stick out past the
    sides, wheels outside the plate.
  * BOB on M3 standoffs (20 mm+) above the motors.
  * 18650 holder on top at the rear, held with a strap (and/or screws).
  * Ball caster underneath at the rear (spacer plates set its height).
  * Line-sensor bar under the front tongue on short standoffs (slots fit
    many hole spacings).
"""
import math
import ezdxf
from shapely.geometry import box, Point, LineString, Polygon, MultiPolygon
from shapely.ops import unary_union
from shapely import affinity

# ============================================================== PARAMETERS
T          = 3.0     # sheet thickness (mm)
SLOT_FIT   = 0.15    # extra width on tab slots (tune to your cutter's kerf)
M3         = 3.3     # M3 clearance hole

PLATE_W    = 90.0    # main plate width (wheels sit outside this)
PLATE_L    = 135.0   # main plate length (rear edge -> start of tongue)
TONGUE_W   = 70.0    # front tongue (line sensor) width
TONGUE_L   = 43.0
CORNER_R   = 6.0     # rounding of all plate corners

# BOB mounting holes (centre-to-centre). Real values from KiCad later!
BOB_HOLE_DX = 56.0   # across the robot (X)
BOB_HOLE_DY = 85.0   # along the robot  (Y)
BOB_CENTRE_Y = 85.0  # where the middle of the hole pattern sits

AXLE_Y     = 66.0    # motor shaft line (Y)
WHEEL_D    = 43.0    # wheel diameter (only used for the height note/preview)

# N20 motor (gearbox and motor can are both ~12 wide x 10 high)
N20_W, N20_H = 12.0, 10.0
NOTCH_FIT  = 0.2     # clearance on the clamp notch
CLAMP_W    = 36.0    # clamp plate width (along Y)
CLAMP_TOP  = 3.0     # material above the motor
TAB_W      = 7.0
CLAMP_X_FRONT = PLATE_W / 2 - 6.0    # clamp on the gearbox (X from centre)
CLAMP_X_REAR  = PLATE_W / 2 - 18.0   # clamp on the motor can

CASTER_Y   = 20.0    # ball caster centre (underneath)
SENSOR_ROWS_Y = (158.0, 168.0)       # line-sensor slot rows
N_SPACERS  = 3       # caster height spacer plates
GAP        = 6.0     # gap between parts on the sheet

# ================================================================ helpers
def slot(x1, y1, x2, y2, w):
    """Rounded slot (stadium) from (x1,y1) to (x2,y2), width w."""
    return LineString([(x1, y1), (x2, y2)]).buffer(w / 2, quad_segs=16)

def hole(x, y, d=M3):
    return Point(x, y).buffer(d / 2, quad_segs=24)

def rrect(cx, cy, w, h, r):
    return box(cx - w / 2 + r, cy - h / 2 + r, cx + w / 2 - r, cy + h / 2 - r).buffer(r, quad_segs=12)

def round_all(poly, r):
    """Round convex AND concave corners."""
    return poly.buffer(r, quad_segs=12).buffer(-2 * r, quad_segs=12).buffer(r, quad_segs=12)

def mirror_x(geoms):
    return geoms + [affinity.scale(g, xfact=-1, origin=(0, 0)) for g in geoms]

# ============================================================== base plate
outline = unary_union([
    box(-PLATE_W / 2, 0, PLATE_W / 2, PLATE_L),
    box(-TONGUE_W / 2, PLATE_L - 10, TONGUE_W / 2, PLATE_L + TONGUE_L),
])
outline = round_all(outline, CORNER_R)

cutouts = []
# BOB holes
by0 = BOB_CENTRE_Y - BOB_HOLE_DY / 2
by1 = BOB_CENTRE_Y + BOB_HOLE_DY / 2
for sx in (-1, 1):
    for y in (by0, by1):
        cutouts.append(hole(sx * BOB_HOLE_DX / 2, y))

# Motor clamp tab slots (tabs at +/-11.5 mm from the axle line)
tab_off = (CLAMP_W / 2) - 3.0 - TAB_W / 2          # = 11.5 for the defaults
side = []
for cx in (CLAMP_X_FRONT, CLAMP_X_REAR):
    for dy in (-tab_off, tab_off):
        side.append(box(cx - (T + SLOT_FIT) / 2, AXLE_Y + dy - (TAB_W + SLOT_FIT) / 2,
                        cx + (T + SLOT_FIT) / 2, AXLE_Y + dy + (TAB_W + SLOT_FIT) / 2))
# zip-tie slots either side of each motor, between the clamps
zx = (CLAMP_X_FRONT + CLAMP_X_REAR) / 2
for dy in (-(N20_W / 2 + 2.5), N20_W / 2 + 2.5):
    side.append(rrect(zx, AXLE_Y + dy, 4.0, 2.2, 1.0))
cutouts += mirror_x(side)

# Ball caster slots (fit hole spacings 12..32 mm)
cutouts += mirror_x([slot(6, CASTER_Y, 16, CASTER_Y, M3)])

# 18650 holder: screw slots + strap slots (20 mm velcro strap)
cutouts += mirror_x([slot(18, 26, 36, 26, M3)])
cutouts.append(rrect(0, 9.0, 22.0, 3.0, 1.2))
cutouts.append(rrect(0, by0 - 3.0, 22.0, 3.0, 1.2))

# Line sensor slots (two rows, hole spacings 16..60 mm) + cable slot
for y in SENSOR_ROWS_Y:
    cutouts += mirror_x([slot(8, y, 30, y, M3)])
cutouts.append(rrect(0, PLATE_L + 10, 20.0, 6.0, 3.0))

# Spare M3 holes in the corners for bumpers / accessories
cutouts += mirror_x([hole(PLATE_W / 2 - 6, 6), hole(PLATE_W / 2 - 6, PLATE_L - 8)])

base = outline.difference(unary_union(cutouts))

# ============================================================ motor clamps
nw, nh = N20_W + NOTCH_FIT, N20_H + NOTCH_FIT
ch = nh + CLAMP_TOP
clamp = box(0, 0, CLAMP_W, ch)
clamp = clamp.difference(box(CLAMP_W / 2 - nw / 2, -1, CLAMP_W / 2 + nw / 2, nh))
clamp = round_all(clamp, 0.8)       # soften corners (tabs added after, stay square)
for u in (3.0, CLAMP_W - 3.0 - TAB_W):
    clamp = clamp.union(box(u, -T, u + TAB_W, 0.01))
clamp = clamp.buffer(0)

# ========================================================= caster spacers
spacer = rrect(0, 0, 40.0, 14.0, 3.0).difference(unary_union(mirror_x([slot(6, 0, 16, 0, M3)])))

# ===================================================== lay out on a sheet
parts = []
bx0, by0_, bx1, by1_ = base.bounds
parts.append(("base", base, 0, 0))
x = bx1 + 32          # leave room for the wheel in the preview
y = by0_
cminx, cminy, cmaxx, cmaxy = clamp.bounds
for i in range(4):
    parts.append((f"clamp{i+1}", clamp, x - cminx, y - cminy + i * (cmaxy - cminy + GAP)))
y2 = y + 4 * (cmaxy - cminy + GAP) + GAP
sminx, sminy, smaxx, smaxy = spacer.bounds
for i in range(N_SPACERS):
    parts.append((f"spacer{i+1}", spacer, x - sminx, y2 - sminy + i * (smaxy - sminy + GAP)))

# ============================================================== DXF out
doc = ezdxf.new("R2010", setup=True)
doc.units = ezdxf.units.MM
doc.header["$INSUNITS"] = 4
doc.layers.add("CUT", color=1)       # red
doc.layers.add("ENGRAVE", color=5)   # blue
msp = doc.modelspace()

def rings(g):
    polys = g.geoms if isinstance(g, MultiPolygon) else [g]
    for p in polys:
        yield p.exterior
        yield from p.interiors

placed = []
for name, g, dx, dy in parts:
    g2 = affinity.translate(g, dx, dy)
    placed.append((name, g2))
    for ring in rings(g2):
        pts = list(ring.coords)
        msp.add_lwpolyline(pts[:-1], close=True, dxfattribs={"layer": "CUT"})

def label(text, x, y, h=3.5):
    t = msp.add_text(text, height=h, dxfattribs={"layer": "ENGRAVE"})
    t.set_placement((x, y), align=ezdxf.enums.TextEntityAlignment.MIDDLE_CENTER)

label("FRONT", 0, PLATE_L + 18.5, 4)
label("A (left)", -(CLAMP_X_FRONT + CLAMP_X_REAR) / 2, AXLE_Y + 22, 3)
label("B (right)", (CLAMP_X_FRONT + CLAMP_X_REAR) / 2, AXLE_Y + 22, 3)
label("EGB220", 0, 100, 5)

doc.saveas("chassis_cut.dxf")

# ============================================================== SVG out
# Same cut lines in an SVG (mm units) for Inkscape / Glowforge / LightBurn.
allx = [c for _, g in placed for c in (g.bounds[0], g.bounds[2])]
ally = [c for _, g in placed for c in (g.bounds[1], g.bounds[3])]
ox, oy = min(allx) - 2, max(ally) + 2           # SVG y points down: flip
W, H = max(allx) - min(allx) + 4, max(ally) - min(ally) + 4
paths = []
for _, g in placed:
    for ring in rings(g):
        d = "M " + " L ".join(f"{x - ox:.3f},{oy - y:.3f}" for x, y in ring.coords) + " Z"
        paths.append(f'<path d="{d}"/>')
with open("chassis_cut.svg", "w") as f:
    f.write(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W:.2f}mm" height="{H:.2f}mm" '
            f'viewBox="0 0 {W:.3f} {H:.3f}">\n<g fill="none" stroke="#ff0000" stroke-width="0.1">\n'
            + "\n".join(paths) + "\n</g>\n</svg>\n")

# ============================================================ preview PNG
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Circle

fig, ax = plt.subplots(figsize=(11, 8.5), dpi=130)
for name, g in placed:
    for ring in rings(g):
        xs, ys = ring.xy
        ax.plot(xs, ys, color="#c62828", lw=0.8)

# reference parts on the base plate (preview only)
ref = dict(fill=False, ls="--", lw=0.9)
for sx in (-1, 1):
    gx0 = sx * (PLATE_W / 2 - 1)                       # gearbox front face
    # motor body (gearbox + can + encoder) from the side edge inwards
    x_in = gx0 - sx * 34
    ax.add_patch(Rectangle((min(gx0, x_in), AXLE_Y - N20_W / 2), 34, N20_W, ec="#1565c0", **ref))
    # wheel (seen from above) outside the plate
    ax.add_patch(Rectangle((sx * (PLATE_W / 2 + 4) - (18 if sx < 0 else 0), AXLE_Y - WHEEL_D / 2), 18, WHEEL_D,
                           ec="#2e7d32", **ref))
# BOB outline guess (hole pattern + 4 mm)
ax.add_patch(Rectangle((-BOB_HOLE_DX / 2 - 4, by0 - 4), BOB_HOLE_DX + 8, BOB_HOLE_DY + 8, ec="#6a1b9a", **ref))
# 18650 holder
ax.add_patch(Rectangle((-38.5, 15.5), 77, 21, ec="#ef6c00", **ref))
ax.add_patch(Circle((0, CASTER_Y), 8, ec="#555", **ref))
ax.text(0, by1 - 10, "BOB (outline guessed)\non 20 mm standoffs", ha="center", fontsize=7, color="#6a1b9a")
ax.text(0, 26, "18650 holder", ha="center", fontsize=7, color="#ef6c00")
ax.text(0, CASTER_Y - 12, "caster (under)", ha="center", fontsize=6, color="#555")
ax.text(0, AXLE_Y + 2, "N20 motors on top", ha="center", fontsize=7, color="#1565c0")
ax.text(0, PLATE_L + 16, "line sensor (under)", ha="center", fontsize=7, color="#555")
ax.set_aspect("equal")
ax.set_title(f"EGB220 chassis - {T:g} mm sheet  (red = cut, dashed = parts, preview only)", fontsize=10)
ax.grid(True, lw=0.3, alpha=0.4)
ax.set_xlabel("mm")
fig.tight_layout()
fig.savefig("chassis_preview.png")

# ================================================================ summary
plate_under = WHEEL_D / 2 - N20_H / 2 - T
minx = min(g.bounds[0] for _, g in placed); maxx = max(g.bounds[2] for _, g in placed)
miny = min(g.bounds[1] for _, g in placed); maxy = max(g.bounds[3] for _, g in placed)
print(f"Sheet needed: {maxx - minx:.0f} x {maxy - miny:.0f} mm")
print(f"Base plate {base.bounds[2]-base.bounds[0]:.0f} x {base.bounds[3]-base.bounds[1]:.0f} mm, "
      f"{len(list(rings(base))) - 1} cut-outs")
print(f"Floor -> underside of plate with {WHEEL_D:g} mm wheels: {plate_under:.1f} mm "
      f"(caster + spacers must make this)")
print(f"BOB holes at X +/-{BOB_HOLE_DX/2:g}, Y {by0:g} and {by1:g}")
