"""
Draws the Arena Battles main-menu buttons from scratch (no HoMM3 pixels), so they can be published.

Each button is an emblem in the style of the HoMM3 main menu with two lines of gold lettering:
- New Arena PvE: crossed swords on a green banner
- New Arena PvP: crossed swords on a red banner
- Custom Arena:  a sword behind a heater shield
- Credits:       a parchment scroll with a quill
- Quit:          a skull on a red banner

Usage: python arena-battles/tools/build_buttons.py
Output: Mods/arenabattles/Content/Sprites/ArenaBtn*.json + Sprites/arena/ab_*.png (4 frames: normal, pressed, hover, hover)
"""
import json
import math
import os

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # repository root
CONTENT = os.path.join(ROOT, 'Mods', 'arenabattles', 'Content')
FONTS = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'fonts')

Q = 4                       # supersampling factor
W, H = 248, 140             # button canvas (1x)


def font(size):
    for path in (r'C:/Windows/Fonts/timesbd.ttf', '/usr/share/fonts/truetype/msttcorefonts/Times_New_Roman_Bold.ttf'):
        if os.path.exists(path):
            return ImageFont.truetype(path, size)
    f = ImageFont.truetype(os.path.join(FONTS, 'Cinzel-Variable.ttf'), size)
    f.set_variation_by_name('Bold')
    return f


# ------------------------------------------------------------------ shading helpers

def blank():
    return Image.new('RGBA', (W * Q, H * Q), (0, 0, 0, 0))


def mask_of(draw_fn):
    m = Image.new('L', (W * Q, H * Q), 0)
    draw_fn(ImageDraw.Draw(m))
    return m


def gradient(size, top, bottom, angle=90):
    """linear gradient image, angle 90 = top->bottom"""
    w, h = size
    g = Image.linear_gradient('L').resize((w, h)) if angle == 90 else \
        Image.linear_gradient('L').rotate(-(angle - 90), expand=False).resize((w, h))
    a = Image.new('RGB', size, top)
    b = Image.new('RGB', size, bottom)
    return Image.composite(b, a, g)


def paint(canvas, mask, top, bottom, outline=(30, 18, 8), outline_w=3, angle=90, highlight=0.35):
    """fill a mask with a gradient, a dark outline and a soft inner highlight (top-left light)"""
    size = canvas.size
    fill = gradient(size, top, bottom, angle).convert('RGBA')
    # inner highlight: light where the mask, shifted down-right, is not
    shifted = ImageChops.offset(mask, 3 * Q, 3 * Q)
    rim = ImageChops.subtract(mask, shifted).filter(ImageFilter.GaussianBlur(2 * Q))
    light = Image.new('RGBA', size, (255, 250, 230, 0))
    light.putalpha(rim.point(lambda v: int(v * highlight)))
    # inner shadow: dark where the mask, shifted up-left, is not
    shifted2 = ImageChops.offset(mask, -3 * Q, -3 * Q)
    rim2 = ImageChops.subtract(mask, shifted2).filter(ImageFilter.GaussianBlur(2 * Q))
    dark = Image.new('RGBA', size, (0, 0, 0, 0))
    dark.putalpha(rim2.point(lambda v: int(v * 0.45)))
    if outline_w:
        grown = mask.filter(ImageFilter.MaxFilter(2 * outline_w * Q // 2 * 2 + 1))
        ol = Image.new('RGBA', size, outline + (255,))
        canvas.alpha_composite(Image.composite(ol, Image.new('RGBA', size, (0, 0, 0, 0)), grown))
    body = Image.composite(fill, Image.new('RGBA', size, (0, 0, 0, 0)), mask)
    body.alpha_composite(Image.composite(light, Image.new('RGBA', size, (0, 0, 0, 0)), mask))
    body.alpha_composite(Image.composite(dark, Image.new('RGBA', size, (0, 0, 0, 0)), mask))
    canvas.alpha_composite(body)


def P(x, y):
    return (x * Q, y * Q)


def rot(points, cx, cy, deg):
    a = math.radians(deg)
    out = []
    for x, y in points:
        dx, dy = x - cx, y - cy
        out.append((cx + dx * math.cos(a) - dy * math.sin(a), cy + dx * math.sin(a) + dy * math.cos(a)))
    return out


def poly(points):
    return [P(x, y) for x, y in points]


# ------------------------------------------------------------------ emblem parts

def banner(c, top, bottom, cx=124, cy=68, w=116, h=78):
    """a pennant hanging from a gold rod, with gently waving sides and a notched (swallow-tail) bottom"""
    y0, y1 = cy - h / 2 + 6, cy + h / 2
    n = 16
    left = [(cx - w / 2 + 3 * math.sin(i / n * math.pi * 1.5), y0 + (y1 - y0 - 10) * i / n) for i in range(n + 1)]
    right = [(cx + w / 2 + 3 * math.sin(i / n * math.pi * 1.5 + 0.8), y0 + (y1 - y0 - 10) * i / n) for i in range(n + 1)]
    bottom_pts = [(cx + w / 2, y1), (cx + w / 4, y1 - 6), (cx, y1 - 18), (cx - w / 4, y1 - 6), (cx - w / 2, y1)]
    pts = left + bottom_pts[::-1][:0] + [(cx - w / 2, y1)] + bottom_pts[::-1][1:-1] + [(cx + w / 2, y1)] + right[::-1]
    m = mask_of(lambda d: d.polygon(poly(pts), fill=255))
    paint(c, m, top, bottom, outline=(28, 14, 8), outline_w=2)
    folds = blank()
    d = ImageDraw.Draw(folds)
    for k in range(1, 5):
        x = cx - w / 2 + w * k / 5
        d.line([P(x, y0 + 4), P(x + 3, y1 - 12)], fill=(0, 0, 0, 80), width=6 * Q)
        d.line([P(x + 6, y0 + 4), P(x + 9, y1 - 12)], fill=(255, 255, 255, 40), width=3 * Q)
    folds = folds.filter(ImageFilter.GaussianBlur(3 * Q))
    c.alpha_composite(Image.composite(folds, Image.new('RGBA', c.size, (0, 0, 0, 0)), m))
    # gold fringe along the notched bottom
    fringe = mask_of(lambda d: d.line(poly([(cx - w / 2, y1)] + bottom_pts[::-1][1:-1] + [(cx + w / 2, y1)]), fill=255, width=3 * Q))
    paint(c, fringe, (255, 226, 130), (160, 104, 30), outline_w=0)
    # the rod with ball ends
    rod = mask_of(lambda d: d.rounded_rectangle([P(cx - w / 2 - 8, y0 - 5), P(cx + w / 2 + 8, y0 + 2)], radius=3 * Q, fill=255))
    paint(c, rod, (255, 226, 130), (140, 88, 22), outline_w=1)
    for x in (cx - w / 2 - 10, cx + w / 2 + 10):
        ball = mask_of(lambda d, x=x: d.ellipse([P(x - 5, y0 - 7), P(x + 5, y0 + 3)], fill=255))
        paint(c, ball, (255, 232, 150), (150, 96, 26), outline_w=1, highlight=0.7)


def sword(c, cx, cy, length, deg, blade=(236, 240, 246), blade2=(120, 130, 150)):
    """a straight sword pointing up (deg rotates around its centre)"""
    half = length / 2
    bw = length * 0.055
    tip = (cx, cy - half)
    blade_pts = [(cx - bw, cy + half * 0.28), (cx - bw, cy - half + bw * 3), tip, (cx + bw, cy - half + bw * 3),
                 (cx + bw, cy + half * 0.28)]
    guard = [(cx - bw * 4.2, cy + half * 0.28), (cx + bw * 4.2, cy + half * 0.28),
             (cx + bw * 4.2, cy + half * 0.28 + bw * 1.6), (cx - bw * 4.2, cy + half * 0.28 + bw * 1.6)]
    grip = [(cx - bw * 0.8, cy + half * 0.28 + bw * 1.6), (cx + bw * 0.8, cy + half * 0.28 + bw * 1.6),
            (cx + bw * 0.8, cy + half * 0.86), (cx - bw * 0.8, cy + half * 0.86)]
    pr = bw * 1.0
    pommel_c = rot([(cx, cy + half * 0.86 + pr * 0.7)], cx, cy, deg)[0]
    paint(c, mask_of(lambda d: d.polygon(poly(rot(blade_pts, cx, cy, deg)), fill=255)), blade, blade2, angle=0)
    # fuller (groove) along the blade
    fuller = rot([(cx, cy + half * 0.25), (cx, cy - half + bw * 4)], cx, cy, deg)
    groove = blank()
    ImageDraw.Draw(groove).line(poly(fuller), fill=(90, 100, 120, 160), width=max(1, int(bw * 0.5)) * Q)
    c.alpha_composite(groove)
    paint(c, mask_of(lambda d: d.polygon(poly(rot(grip, cx, cy, deg)), fill=255)), (110, 60, 30), (50, 24, 10))
    paint(c, mask_of(lambda d: d.polygon(poly(rot(guard, cx, cy, deg)), fill=255)), (255, 222, 120), (150, 96, 26))
    paint(c, mask_of(lambda d: d.ellipse([P(pommel_c[0] - pr, pommel_c[1] - pr), P(pommel_c[0] + pr, pommel_c[1] + pr)],
                                          fill=255)), (255, 222, 120), (150, 96, 26))


def shield(c, cx, cy, w, h, field_top, field_bottom):
    """a heater shield with a gold rim and a field colour"""
    pts = []
    for i in range(0, 41):
        t = i / 40
        y = cy - h / 2 + h * 0.42 * t
        pts.append((cx - w / 2, y))
    for i in range(0, 41):                       # curved lower part to the point
        a = math.pi / 2 * i / 40
        pts.append((cx - w / 2 * math.cos(a), cy - h / 2 + h * 0.42 + (h * 0.58) * math.sin(a)))
    for i in range(40, -1, -1):
        a = math.pi / 2 * i / 40
        pts.append((cx + w / 2 * math.cos(a), cy - h / 2 + h * 0.42 + (h * 0.58) * math.sin(a)))
    pts.append((cx + w / 2, cy - h / 2))
    m = mask_of(lambda d: d.polygon(poly(pts), fill=255))
    paint(c, m, (255, 224, 128), (140, 88, 22))
    inner = m.filter(ImageFilter.MinFilter(9 * Q // 2 * 2 + 1))
    paint(c, inner, field_top, field_bottom, outline_w=0, highlight=0.5)
    # a silver chevron
    chev = mask_of(lambda d: d.line(poly([(cx - w * 0.36, cy + h * 0.05), (cx, cy - h * 0.22), (cx + w * 0.36, cy + h * 0.05)]),
                                    fill=255, width=int(w * 0.14 * Q)))
    chev = ImageChops.multiply(chev, inner)
    paint(c, chev, (245, 248, 252), (140, 150, 170), outline_w=1)


def scroll(c, cx, cy, w, h):
    """an unrolled parchment with rolled ends and a few lines of writing"""
    body = mask_of(lambda d: d.rounded_rectangle([P(cx - w / 2, cy - h / 2), P(cx + w / 2, cy + h / 2)], radius=4 * Q, fill=255))
    paint(c, body, (250, 236, 196), (206, 170, 110), outline=(70, 44, 18), outline_w=2)
    lines = blank()
    d = ImageDraw.Draw(lines)
    for i in range(5):
        y = cy - h / 2 + 12 + i * (h - 22) / 4.5
        d.line([P(cx - w / 2 + 12, y), P(cx + w / 2 - 12 - (i % 2) * 14, y)], fill=(110, 70, 30, 170), width=2 * Q)
    c.alpha_composite(lines)
    for x in (cx - w / 2, cx + w / 2):            # rolled ends
        roll = mask_of(lambda d, x=x: d.rounded_rectangle([P(x - 7, cy - h / 2 - 6), P(x + 7, cy + h / 2 + 6)], radius=7 * Q, fill=255))
        paint(c, roll, (240, 220, 170), (150, 110, 60), outline=(70, 44, 18), outline_w=2, angle=0)


def quill(c, x0, y0, x1, y1):
    """a feather quill from (x0,y0) tip to (x1,y1)"""
    dx, dy = x1 - x0, y1 - y0
    L = math.hypot(dx, dy)
    nx, ny = -dy / L, dx / L
    pts = [(x0, y0)]
    for i in range(1, 21):
        t = i / 20
        wdt = 9 * math.sin(math.pi * min(1, t * 1.25)) if t > 0.15 else 1
        pts.append((x0 + dx * t + nx * wdt, y0 + dy * t + ny * wdt))
    for i in range(20, 0, -1):
        t = i / 20
        wdt = 6 * math.sin(math.pi * min(1, t * 1.25)) if t > 0.15 else 1
        pts.append((x0 + dx * t - nx * wdt, y0 + dy * t - ny * wdt))
    paint(c, mask_of(lambda d: d.polygon(poly(pts), fill=255)), (255, 255, 255), (170, 180, 200), outline=(40, 40, 60), outline_w=1)
    shaft = blank()
    ImageDraw.Draw(shaft).line([P(x0, y0), P(x1, y1)], fill=(120, 100, 70, 255), width=2 * Q)
    c.alpha_composite(shaft)


def skull(c, cx, cy, s):
    """a stylised skull of size s (height)"""
    bone_t, bone_b = (250, 246, 228), (176, 160, 120)
    head = mask_of(lambda d: (d.ellipse([P(cx - s * 0.42, cy - s * 0.5), P(cx + s * 0.42, cy + s * 0.22)], fill=255),
                              d.rounded_rectangle([P(cx - s * 0.26, cy), P(cx + s * 0.26, cy + s * 0.48)], radius=int(s * 0.1 * Q), fill=255)))
    paint(c, head, bone_t, bone_b, outline=(40, 26, 14), outline_w=2, highlight=0.6)
    holes = blank()
    d = ImageDraw.Draw(holes)
    for sx in (-1, 1):                            # eye sockets
        ex = cx + sx * s * 0.17
        d.ellipse([P(ex - s * 0.12, cy - s * 0.1), P(ex + s * 0.12, cy + s * 0.1)], fill=(30, 14, 10, 255))
    d.polygon(poly([(cx, cy + s * 0.08), (cx - s * 0.06, cy + s * 0.2), (cx + s * 0.06, cy + s * 0.2)]), fill=(30, 14, 10, 255))
    for i in range(-2, 3):                        # teeth gaps
        x = cx + i * s * 0.09
        d.line([P(x, cy + s * 0.3), P(x, cy + s * 0.46)], fill=(60, 40, 20, 255), width=max(1, int(s * 0.025)) * Q)
    d.line([P(cx - s * 0.24, cy + s * 0.3), P(cx + s * 0.24, cy + s * 0.3)], fill=(60, 40, 20, 255), width=max(1, int(s * 0.02)) * Q)
    c.alpha_composite(holes.filter(ImageFilter.GaussianBlur(0.6 * Q)))


# ------------------------------------------------------------------ emblems

def emblem_swords(c, banner_top, banner_bottom):
    banner(c, banner_top, banner_bottom)
    sword(c, 124, 62, 104, -34)
    sword(c, 124, 62, 104, 34)


def emblem_custom(c):
    sword(c, 124, 62, 112, 0)
    shield(c, 124, 72, 70, 80, (60, 96, 190), (22, 36, 96))


def emblem_credits(c):
    scroll(c, 124, 70, 112, 62)
    quill(c, 148, 98, 184, 30)


def emblem_quit(c):
    banner(c, (150, 30, 30), (70, 10, 10), w=104, h=80)
    skull(c, 124, 46, 64)


# ------------------------------------------------------------------ lettering and states

def lettering(lines, size, state):
    """two lines of HoMM3-style gold lettering with a dark outline, on a transparent layer (supersampled)"""
    f = font(size * Q)
    layer = blank()
    d = ImageDraw.Draw(layer)
    heights = []
    for t in lines:
        b = d.textbbox((0, 0), t, font=f)
        heights.append((b, t))
    gap = int(size * Q * 0.05)
    total = sum(b[3] - b[1] for b, _ in heights) + gap * (len(lines) - 1)
    y = (H * Q - total) // 2 + (Q if state == 1 else 0)
    text_mask = Image.new('L', layer.size, 0)
    md = ImageDraw.Draw(text_mask)
    for b, t in heights:
        x = (W * Q - (b[2] - b[0])) // 2 - b[0] + (Q if state == 1 else 0)
        md.text((x, y - b[1]), t, font=f, fill=255)
        y += b[3] - b[1] + gap
    # glow / shadow behind the letters
    halo = text_mask.filter(ImageFilter.MaxFilter(7 * Q // 2 * 2 + 1)).filter(ImageFilter.GaussianBlur(3 * Q))
    halo_col = (255, 196, 90) if state >= 2 else (16, 6, 10)
    hl = Image.new('RGBA', layer.size, halo_col + (0,))
    hl.putalpha(halo.point(lambda v: min(255, int(v * (0.75 if state >= 2 else 1.4)))))
    layer.alpha_composite(hl)
    # dark outline
    ol = Image.new('RGBA', layer.size, (40, 20, 6, 255))
    layer.alpha_composite(Image.composite(ol, Image.new('RGBA', layer.size, (0, 0, 0, 0)),
                                          text_mask.filter(ImageFilter.MaxFilter(2 * Q + 1))))
    # gold body: bright top, deep bottom on each line
    gold = gradient(layer.size, (255, 246, 196), (196, 128, 32)).convert('RGBA')
    bands = Image.new('RGBA', layer.size, (0, 0, 0, 0))
    bd = ImageDraw.Draw(bands)
    y = (H * Q - total) // 2
    for b, _ in heights:
        hgt = b[3] - b[1]
        for k in range(hgt):
            t = k / max(1, hgt - 1)
            col = (int(255 - 60 * t), int(240 - 120 * t), int(170 - 140 * t), 255)
            bd.line([(0, y + k), (W * Q, y + k)], fill=col)
        y += hgt + gap
    if state == 1:
        bands = ImageChops.offset(bands, 0, Q)
    gold = bands
    if state >= 2:
        gold = Image.eval(gold.convert('RGB'), lambda v: min(255, int(v * 1.05))).convert('RGBA')
    layer.alpha_composite(Image.composite(gold, Image.new('RGBA', layer.size, (0, 0, 0, 0)), text_mask))
    return layer


def build(name, emblem, label, size=31):
    frames = []
    for state in (0, 1, 2, 3):
        c = blank()
        emblem(c)
        if state == 1:                            # pressed: darker, nudged down-right
            c = ImageChops.offset(c, Q, Q)
            c = Image.eval(c.convert('RGBA'), lambda v: v)
            dark = Image.new('RGBA', c.size, (0, 0, 0, 60))
            c.alpha_composite(Image.composite(dark, Image.new('RGBA', c.size, (0, 0, 0, 0)), c.getchannel('A')))
        if state >= 2:                            # hover: a little brighter
            r, g, b, a = c.split()
            c = Image.merge('RGBA', [ch.point(lambda v: min(255, int(v * 1.08))) for ch in (r, g, b)] + [a])
        # soft drop shadow under the emblem so it reads on any background
        sh = Image.new('RGBA', c.size, (0, 0, 0, 0))
        sh.putalpha(c.getchannel('A').filter(ImageFilter.GaussianBlur(4 * Q)).point(lambda v: int(v * 0.55)))
        sh = ImageChops.offset(sh, 2 * Q, 3 * Q)
        sh.alpha_composite(c)
        sh.alpha_composite(lettering(label, size, state))
        frames.append(sh.resize((W, H), Image.LANCZOS))
    out_dir = os.path.join(CONTENT, 'Sprites', 'arena')
    os.makedirs(out_dir, exist_ok=True)
    anim = {'basepath': 'arena/', 'images': []}
    for i, im in enumerate(frames):
        fname = f'ab_{name}_{i}.png'
        im.save(os.path.join(out_dir, fname), optimize=True)
        anim['images'].append({'frame': i, 'file': fname})
    with open(os.path.join(CONTENT, 'Sprites', name + '.json'), 'w', encoding='utf-8') as fh:
        json.dump(anim, fh, indent='\t')
    print('wrote', name)


def main():
    build('ArenaBtnNewPvE', lambda c: emblem_swords(c, (70, 150, 70), (20, 66, 26)), ['NEW ARENA', 'PvE'])
    build('ArenaBtnNewPvP', lambda c: emblem_swords(c, (200, 44, 34), (96, 12, 10)), ['NEW ARENA', 'PvP'])
    build('ArenaBtnCustom', emblem_custom, ['CUSTOM', 'ARENA'])
    build('ArenaBtnCredits', emblem_credits, ['CREDITS'], size=34)
    build('ArenaBtnQuit', emblem_quit, ['QUIT'], size=36)


if __name__ == '__main__':
    main()
