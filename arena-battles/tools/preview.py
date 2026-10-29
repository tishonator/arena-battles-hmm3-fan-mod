"""
Composes a preview of the main menu from the generated assets and the
positions in Mods/arenabattles/Content/config/mainmenu.json (no need to start the game).

Usage: python arena-battles/tools/preview.py [output.png] [--2x]
"""
import json
import os
import re
import sys

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # repository root
CONTENT = os.path.join(ROOT, 'Mods', 'arenabattles', 'Content')
H3 = os.environ.get('H3_DIR', r'D:/games/HoMM 3 Complete')   # original game, for H3 buttons


def load_json(path):
    text = open(path, encoding='utf-8').read()
    text = re.sub(r'^\s*//.*$', '', text, flags=re.M)      # VCMI json allows comments
    text = re.sub(r',(\s*[}\]])', r'\1', text)              # ...and trailing commas
    return json.loads(text)


def h3_button(name, frame):
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from h3data import decode_def, get, read_lod
    f, ents = read_lod(os.path.join(H3, 'Data', 'H3sprite.lod'))
    return decode_def(get(f, ents[name.upper() + '.DEF']))[frame][2]


def main():
    out = next((a for a in sys.argv[1:] if not a.startswith('--')), os.path.join(ROOT, 'arena-battles', 'preview.png'))
    hd = '--2x' in sys.argv
    s = 2 if hd else 1
    data, sprites = ('Data2x', 'Sprites2x') if hd else ('Data', 'Sprites')

    window = load_json(os.path.join(CONTENT, 'config', 'mainmenu.json'))['window']
    img = Image.open(os.path.join(CONTENT, data, window['background'][0] + '.png')).convert('RGBA')
    for node in window.get('images', []):
        pic = Image.open(os.path.join(CONTENT, data, node['name'][0] + '.png')).convert('RGBA')
        img.alpha_composite(pic, (node['x'] * s, node['y'] * s))

    main_entry = next(i for i in window['items'] if i['name'] == 'main')
    for n, b in enumerate(main_entry['buttons']):
        frame = 3 if n == 0 else 0          # show the first button highlighted
        anim_path = os.path.join(CONTENT, 'Sprites', b['name'] + '.json')
        if os.path.exists(anim_path):
            anim = json.load(open(anim_path, encoding='utf-8'))
            f = next(i['file'] for i in anim['images'] if i['frame'] == frame)
            pic_path = os.path.join(CONTENT, sprites, anim.get('basepath', ''), f)
            if not os.path.exists(pic_path):          # no HD version -> upscale the 1x one
                pic_path = os.path.join(CONTENT, 'Sprites', anim.get('basepath', ''), f)
            pic = Image.open(pic_path).convert('RGBA')
        else:                                         # an original HoMM3 button
            pic = h3_button(b['name'], frame)
        if hd and pic.size[0] < 400 and not os.path.exists(os.path.join(CONTENT, 'Sprites2x', b['name'] + '.json')):
            pic = pic.resize((pic.width * s, pic.height * s), Image.LANCZOS)
        x, y = b['x'] * s, b['y'] * s
        if b.get('center'):
            x, y = x - pic.width // 2, y - pic.height // 2
        img.alpha_composite(pic, (x, y))

    img.convert('RGB').save(out)
    print('preview saved to', out)


if __name__ == '__main__':
    main()
