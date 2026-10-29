"""
Generates all graphics for the Arena Battles main menu.

Everything is drawn procedurally here (no HoMM3 assets are copied),
so the output can be redistributed with the mod.

Usage:  python arena-battles/tools/build_assets.py
Output: Mods/arenabattles/Content/{Data,Data2x,Sprites,Sprites2x}/...
"""
import math
import os
import random

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont


ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # repository root
FONTS = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'fonts')
CONTENT = os.path.join(ROOT, 'Mods', 'arenabattles', 'Content')

S = 2                      # we draw at 2x and downscale for the 1x version
W, H = 800 * S, 600 * S    # VCMI main menu window is 800x600

TITLE = 'Arena Battles'
SUBTITLE = 'A HoMM3 Fan Mod'
DISCLAIMER = ('Unofficial fan-made mod for VCMI. Requires original Heroes of Might and Magic III data. '
              'Heroes of Might and Magic is a trademark of Ubisoft Entertainment.')


random.seed(3)


# ----------------------------------------------------------------- helpers

def font(name, size, variation=None):
    f = ImageFont.truetype(os.path.join(FONTS, name), size)
    if variation:
        f.set_variation_by_name(variation)
    return f


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))


def vgradient(size, stops):
    """Vertical gradient. stops = [(pos 0..1, (r,g,b)), ...]"""
    w, h = size
    col = Image.new('RGB', (1, h))
    px = col.load()
    for y in range(h):
        t = y / max(h - 1, 1)
        for i in range(len(stops) - 1):
            p0, c0 = stops[i]
            p1, c1 = stops[i + 1]
            if p0 <= t <= p1:
                px[0, y] = lerp(c0, c1, (t - p0) / max(p1 - p0, 1e-6))
                break
        else:
            px[0, y] = stops[-1][1] if t > stops[-1][0] else stops[0][1]
    return col.resize((w, h))


def noise(size, cells, seed):
    """Smooth value noise in 'L' mode."""
    rnd = random.Random(seed)
    cw, ch = max(2, size[0] // cells), max(2, size[1] // cells)
    small = Image.new('L', (cw, ch))
    small.putdata([rnd.randint(0, 255) for _ in range(cw * ch)])
    return small.resize(size, Image.BICUBIC)


def fractal_noise(size, seed, octaves=((160, .5), (60, .3), (20, .2))):
    out = Image.new('L', size, 0)
    acc = 0.0
    for i, (cells, weight) in enumerate(octaves):
        layer = noise(size, cells, seed + i)
        acc += weight
        out = Image.blend(out, layer, weight / acc)
    return out


def glow(mask, radius, color, strength=1.0):
    blurred = mask.filter(ImageFilter.GaussianBlur(radius))
    if strength != 1.0:
        blurred = blurred.point(lambda v: min(255, int(v * strength)))
    layer = Image.new('RGBA', mask.size, color + (0,))
    layer.putalpha(blurred)
    return layer


def gold_text(text, fnt, stroke, shadow=True, palette='gold', tracking=0):
    """Renders text with a metallic gradient, dark outline and drop shadow. Returns RGBA."""
    # measure
    tmp = ImageDraw.Draw(Image.new('L', (1, 1)))
    widths = [tmp.textlength(ch, font=fnt) for ch in text]
    asc, desc = fnt.getmetrics()
    tw = int(sum(widths) + tracking * (len(text) - 1))
    pad = stroke * 4 + 8
    w, h = tw + pad * 2, asc + desc + pad * 2

    def draw_text(img, fill, sw=0, sf=None):
        d = ImageDraw.Draw(img)
        x = pad
        for ch, cw in zip(text, widths):
            d.text((x, pad), ch, font=fnt, fill=fill, stroke_width=sw, stroke_fill=sf)
            x += cw + tracking

    body = Image.new('L', (w, h), 0)
    draw_text(body, 255)
    outline = Image.new('L', (w, h), 0)
    draw_text(outline, 255, stroke, 255)

    palettes = {
        'gold': [(0, (255, 250, 214)), (.38, (247, 214, 112)), (.52, (196, 140, 40)),
                 (.62, (232, 186, 82)), (1, (120, 72, 16))],
        'bright': [(0, (255, 255, 240)), (.38, (255, 236, 150)), (.52, (226, 172, 64)),
                   (.62, (255, 214, 110)), (1, (160, 100, 30))],
        'menu': [(0, (255, 255, 255)), (.35, (246, 238, 206)), (.5, (226, 182, 78)),
                 (.62, (246, 210, 110)), (1, (128, 78, 14))],
        'grey': [(0, (220, 220, 220)), (.5, (150, 150, 150)), (1, (90, 90, 90))],
    }
    # gradient spans the glyph body (cap height area) rather than the whole box
    grad = vgradient((w, h), [(0, palettes[palette][0][1])] +
                     [((pad + asc * 0.15 + p * asc * 0.9) / h, c) for p, c in palettes[palette]] +
                     [(1, palettes[palette][-1][1])])

    out = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    if shadow:
        sh = Image.new('RGBA', (w, h), (0, 0, 0, 0))
        sh.putalpha(outline.filter(ImageFilter.GaussianBlur(stroke * 1.2)).point(lambda v: int(v * .85)))
        out.alpha_composite(sh, (stroke // 2, stroke))
    dark = Image.new('RGBA', (w, h), (28, 14, 4, 0))
    dark.putalpha(outline)
    out.alpha_composite(dark)
    fill = grad.convert('RGBA')
    fill.putalpha(body)
    out.alpha_composite(fill)
    # thin highlight on the upper edge of glyphs
    hi = ImageChops.subtract(body, ImageChops.offset(body, 0, max(1, stroke // 3)))
    hl = Image.new('RGBA', (w, h), (255, 255, 235, 0))
    hl.putalpha(hi.point(lambda v: int(v * .55)))
    out.alpha_composite(hl)
    return out.crop(out.getbbox())


def save_pair(img2x, rel_1x, rel_2x):
    """Saves 2x image and its 1x downscale into the mod Content folder."""
    for rel, im in ((rel_2x, img2x), (rel_1x, img2x.resize((img2x.width // S, img2x.height // S), Image.LANCZOS))):
        path = os.path.join(CONTENT, rel)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        im.save(path, optimize=True)
        print('wrote', os.path.relpath(path, ROOT), im.size)


# -------------------------------------------------------------- background

def sky():
    img = vgradient((W, H), [
        (0.00, (14, 8, 30)),
        (0.30, (44, 16, 58)),
        (0.48, (110, 30, 62)),
        (0.60, (186, 70, 48)),
        (0.68, (232, 136, 64)),
        (1.00, (60, 20, 24)),
    ]).convert('RGBA')

    # setting sun behind the arena
    sx, sy = int(W * .40), int(H * .62)
    sun = Image.new('L', (W, H), 0)
    ImageDraw.Draw(sun).ellipse((sx - 150 * S // 2, sy - 150 * S // 2, sx + 150 * S // 2, sy + 150 * S // 2), fill=255)
    img.alpha_composite(glow(sun, 160 * S // 2, (255, 170, 80), 1.0))
    img.alpha_composite(glow(sun, 50 * S // 2, (255, 214, 140), 1.0))
    disc = Image.new('RGBA', (W, H), (255, 236, 190, 0))
    disc.putalpha(sun.filter(ImageFilter.GaussianBlur(3 * S)).point(lambda v: int(v * .92)))
    img.alpha_composite(disc)

    # cloud bands: stretched noise, dark purple on top, warm-lit near the horizon
    n = fractal_noise((W, H), 11, ((90, .55), (35, .3), (12, .15)))
    n = n.resize((W, H // 3)).resize((W, H), Image.BICUBIC)   # horizontal streaks
    band = vgradient((1, H), [(0, (90,)), (.25, (200,)), (.55, (150,)), (.66, (0,)), (1, (0,))]) \
        .convert('L').resize((W, H))
    cloud_mask = ImageChops.multiply(n.point(lambda v: max(0, min(255, (v - 118) * 4))), band)
    clouds_dark = Image.new('RGBA', (W, H), (34, 10, 40, 0))
    clouds_dark.putalpha(cloud_mask.point(lambda v: int(v * .8)))
    img.alpha_composite(clouds_dark)
    rim = ImageChops.subtract(cloud_mask, ImageChops.offset(cloud_mask, 0, -4 * S))
    warm = vgradient((1, H), [(0, (0,)), (.35, (40,)), (.6, (255,)), (1, (0,))]).convert('L').resize((W, H))
    rim_layer = Image.new('RGBA', (W, H), (255, 150, 90, 0))
    rim_layer.putalpha(ImageChops.multiply(rim, warm).filter(ImageFilter.GaussianBlur(1.5 * S)))
    img.alpha_composite(rim_layer)

    # a few stars at the very top
    d = ImageDraw.Draw(img)
    for _ in range(120):
        x, y = random.randrange(W), random.randrange(int(H * .28))
        a = random.randint(60, 200)
        r = random.choice((1, 1, 1, 2))
        d.ellipse((x, y, x + r, y + r), fill=(255, 240, 220, a))
    return img


def ridge(y_base, amp, rough, seed):
    rnd = random.Random(seed)
    pts, y = [], y_base
    for x in range(0, W + 8 * S, 8 * S):
        y += rnd.uniform(-rough, rough)
        y = y_base + max(-amp, min(amp, y - y_base))
        pts.append((x, y))
    return pts


def mountains(img):
    for y_base, amp, rough, col, seed in (
            (H * .60, 34 * S, 9 * S, (96, 34, 60), 5),
            (H * .64, 24 * S, 7 * S, (64, 20, 42), 9)):
        poly = ridge(y_base, amp, rough, seed) + [(W, H), (0, H)]
        layer = Image.new('RGBA', (W, H), (0, 0, 0, 0))
        ImageDraw.Draw(layer).polygon(poly, fill=col + (255,))
        img.alpha_composite(layer)


def colosseum(img):
    """Front facade of an elliptical amphitheatre, back-lit by the sun."""
    cx = W * .40
    a = W * .66                 # half width of the ellipse (wider than screen)
    top_c, top_b = H * .55, H * .09
    bot_c, bot_b = H * .80, H * .10
    tiers = 3

    def y_at(c, b, u):
        return c + b * math.sqrt(max(0.0, 1 - u * u))

    # facade silhouette
    mask = Image.new('L', (W, H), 0)
    pts_top, pts_bot = [], []
    for x in range(0, W + 1, 4):
        u = (x - cx) / a
        pts_top.append((x, y_at(top_c, top_b, u)))
        pts_bot.append((x, y_at(bot_c, bot_b, u)))
    ImageDraw.Draw(mask).polygon(pts_top + pts_bot[::-1], fill=255)

    # wall shading: darker at the bottom, stone texture, warm rim on the top edge
    wall = vgradient((W, H), [(0, (70, 30, 40)), (.55, (58, 24, 36)), (.8, (30, 12, 22)), (1, (20, 8, 14))]).convert('RGBA')
    tex = fractal_noise((W, H), 21, ((40, .5), (12, .3), (4, .2)))
    wall = Image.composite(wall, Image.new('RGBA', (W, H), (18, 6, 12, 255)), tex.point(lambda v: 170 + v // 4))
    facade = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    facade.paste(wall, (0, 0), mask)

    d = ImageDraw.Draw(facade)
    # arches, uniformly spaced around the ellipse angle -> natural foreshortening
    n_arch = 30
    for tier in range(tiers):
        t0, t1 = tier / tiers, (tier + 1) / tiers
        for k in range(n_arch):
            th = -math.pi / 2 + (k + .5) * math.pi / n_arch
            u = math.sin(th)
            x = cx + a * u
            if x < -40 * S or x > W + 40 * S:
                continue
            yt, yb = y_at(top_c, top_b, u), y_at(bot_c, bot_b, u)
            h = yb - yt
            ty0, ty1 = yt + h * t0, yt + h * t1
            half_w = (a * math.pi / n_arch) * math.cos(th) * .30
            if half_w < 1.5 * S:
                continue
            open_top = ty0 + (ty1 - ty0) * .22
            open_bot = ty1 - (ty1 - ty0) * .06
            if tier == 0:
                col = (255, 170, 96, 255)        # top tier: sky glow seen through the arches
            elif random.random() < .22:
                col = (255, 132, 40, 255)        # torch-lit opening
            else:
                col = (12, 4, 10, 255)
            d.rectangle((x - half_w, open_top + half_w, x + half_w, open_bot), fill=col)
            d.ellipse((x - half_w, open_top, x + half_w, open_top + 2 * half_w), fill=col)
        # cornice line between tiers
        line = [(x, y_at(top_c, top_b, (x - cx) / a) + (y_at(bot_c, bot_b, (x - cx) / a) - y_at(top_c, top_b, (x - cx) / a)) * t1)
                for x in range(0, W + 1, 4)]
        if tier < tiers - 1:
            d.line(line, fill=(96, 44, 48, 255), width=2 * S)

    # warm rim light along the very top edge
    edge = ImageChops.subtract(mask, ImageChops.offset(mask, 0, 3 * S))
    img.alpha_composite(glow(mask.filter(ImageFilter.MaxFilter(1)), 18 * S, (255, 140, 70), .45))
    img.alpha_composite(facade)
    rim = Image.new('RGBA', (W, H), (255, 190, 120, 0))
    rim.putalpha(edge)
    img.alpha_composite(rim)

    # banners on poles along the top
    for k in range(-6, 7):
        th = k * math.pi / 15
        u = math.sin(th)
        x = cx + a * u
        if not 0 < x < W:
            continue
        y = y_at(top_c, top_b, u)
        hgt = (28 + 10 * math.cos(th)) * S
        wd = max(2, int(8 * math.cos(th) * S))
        dd = ImageDraw.Draw(img)
        dd.line((x, y, x, y - hgt), fill=(30, 12, 18, 255), width=max(1, S))
        flag_col = (150, 24, 30, 255) if k % 2 else (40, 50, 120, 255)
        dd.polygon([(x, y - hgt), (x + wd * 2.2, y - hgt + wd * .6), (x, y - hgt + wd * 1.5)], fill=flag_col)


def foreground(img):
    # ground in front of the arena
    g = vgradient((W, H), [(0, (40, 16, 20)), (.8, (40, 16, 20)), (.86, (30, 12, 16)), (1, (8, 3, 6))]).convert('RGBA')
    gm = Image.new('L', (W, H), 0)
    ImageDraw.Draw(gm).polygon(ridge(H * .865, 6 * S, 3 * S, 31) + [(W, H), (0, H)], fill=255)
    img.paste(g, (0, 0), gm)
    # dust haze where the ground meets the wall
    haze = Image.new('L', (W, H), 0)
    ImageDraw.Draw(haze).rectangle((0, int(H * .84), W, int(H * .88)), fill=200)
    img.alpha_composite(glow(haze, 20 * S, (200, 90, 60), .6))


def vignette(img):
    m = Image.new('L', (W, H), 0)
    ImageDraw.Draw(m).ellipse((-W * .25, -H * .30, W * 1.25, H * 1.25), fill=255)
    m = m.filter(ImageFilter.GaussianBlur(120 * S))
    dark = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    dark.putalpha(ImageChops.invert(m).point(lambda v: int(v * .85)))
    img.alpha_composite(dark)
    # soft darkening behind the button column on the right for readability
    col = Image.new('L', (W, H), 0)
    ImageDraw.Draw(col).rectangle((int(W * .66), 0, W, H), fill=110)
    dark2 = Image.new('RGBA', (W, H), (8, 2, 10, 0))
    dark2.putalpha(col.filter(ImageFilter.GaussianBlur(70 * S)))
    img.alpha_composite(dark2)


def disclaimer(img):
    f = font('Cinzel-Variable.ttf', 9 * S, 'Regular')
    d = ImageDraw.Draw(img)
    # wrap into two lines
    words, lines, cur = DISCLAIMER.split(), [], ''
    for w_ in words:
        test = (cur + ' ' + w_).strip()
        if d.textlength(test, font=f) > 470 * S:
            lines.append(cur)
            cur = w_
        else:
            cur = test
    lines.append(cur)
    y = H - (len(lines) * 12 + 8) * S
    for line in lines:
        d.text((14 * S, y), line, font=f, fill=(210, 180, 160, 200))
        y += 12 * S


def build_background():
    img = sky()
    mountains(img)
    colosseum(img)
    foreground(img)
    vignette(img)
    disclaimer(img)
    return img.convert('RGB')


# ------------------------------------------------------------------- title

def build_title():
    t1 = gold_text(TITLE, font('CinzelDecorative-Black.ttf', 50 * S), 4 * S, tracking=1 * S)
    t2 = gold_text(SUBTITLE, font('Cinzel-Variable.ttf', 27 * S, 'Bold'), 3 * S, tracking=2 * S)
    gap = -6 * S
    w = max(t1.width, t2.width + 40 * S)
    h = t1.height + gap + t2.height + 18 * S
    out = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    out.alpha_composite(t1, (0, 0))
    # ornamental rule under the subtitle
    y2 = t1.height + gap
    sx = (w - t2.width) // 2               # subtitle centred under the title
    out.alpha_composite(t2, (sx, y2))
    d = ImageDraw.Draw(out)
    ry = y2 + t2.height + 8 * S
    d.line((sx, ry, sx + t2.width, ry), fill=(40, 20, 6, 220), width=4 * S)
    d.line((sx, ry, sx + t2.width, ry), fill=(230, 186, 90, 255), width=2 * S)
    mid = sx + t2.width / 2
    d.polygon([(mid - 8 * S, ry), (mid, ry - 6 * S), (mid + 8 * S, ry), (mid, ry + 6 * S)], fill=(250, 220, 130, 255),
              outline=(40, 20, 6, 255))
    # pad so the image dimensions are even (clean 1x downscale)
    out = out.crop((0, 0, w + w % 2, h + h % 2))
    return out


if __name__ == '__main__':
    save_pair(build_background(), os.path.join('Data', 'ArenaMenuBackground.png'),
              os.path.join('Data2x', 'ArenaMenuBackground.png'))
    save_pair(build_title(), os.path.join('Data', 'ArenaMenuTitle.png'),
              os.path.join('Data2x', 'ArenaMenuTitle.png'))
