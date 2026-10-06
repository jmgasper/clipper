#!/usr/bin/env python3
"""Generate Clipper's layered clipboard artwork as native HVIF and SVG.

The 64-unit drawing uses a blue board, warm paper and a metal clip. Details
have a minimum level of detail so the silhouette remains clear in the Deskbar.
"""
from pathlib import Path
import hvif

C = hvif.hex_color
styles, paths, shapes = [], [], []


def solid(color):
    styles.append({'color': C(color)})
    return len(styles) - 1


def gradient(start, end, colors):
    styles.append(hvif.linear_gradient(start, end,
        [(i / (len(colors) - 1), C(color)) for i, color in enumerate(colors)]))
    return len(styles) - 1


def polygon(style, points, detail=False):
    paths.append({'closed': True, 'points': points})
    shape = {'style': style, 'paths': [len(paths) - 1]}
    if detail:
        shape['lod'] = (0.5, 4.0)
    shapes.append(shape)


outline = solid('183e52')
side = gradient((44, 12), (53, 57), ['28768d', '16485e'])
board = gradient((13, 9), (44, 54), ['65c3cb', '2c829b'])
paper_edge = solid('b8bbb0')
paper = gradient((19, 18), (43, 51), ['fffef3', 'e6e6d4'])
back_paper = solid('cce3e2')
clip_dark = solid('465c69')
clip = gradient((25, 4), (33, 19), ['f2f5ee', 'b7c9ca', '839ba5'])
ink = solid('557c87')
accent = gradient((37, 34), (47, 52), ['ffc961', 'e99535'])
shine = solid('9ddade')

# Three-quarter board, with a dark silhouette and a visible right edge.
polygon(outline, [(10, 13), (43, 6), (54, 14), (54, 53), (21, 62), (10, 53)])
polygon(side, [(43, 9), (51, 15), (51, 51), (21, 59), (13, 52)])
polygon(board, [(13, 15), (44, 9), (44, 49), (13, 56)])
polygon(shine, [(14, 15), (42, 10), (42, 12), (14, 18)], True)

# Two sheets convey clipboard history; the second shows at the lower edge.
polygon(paper_edge, [(19, 20), (47, 14), (47, 48), (19, 55)])
polygon(back_paper, [(20, 20), (46, 15), (46, 47), (20, 53)])
polygon(paper_edge, [(16, 17), (42, 12), (42, 48), (16, 54)])
polygon(paper, [(17, 18), (41, 13), (41, 47), (17, 52)])

# A substantial metal clip reads at 16 px too.
polygon(clip_dark, [(22, 8), (26, 7), (26, 4), (33, 2), (37, 4), (37, 7),
                    (40, 9), (40, 19), (22, 23)])
polygon(clip, [(24, 9), (28, 8), (28, 5), (33, 4), (35, 5), (35, 9),
               (38, 10), (38, 17), (24, 20)])
polygon(clip_dark, [(28, 12), (34, 11), (34, 14), (28, 15)], True)

polygon(ink, [(21, 29), (36, 26), (36, 29), (21, 32)])
polygon(ink, [(21, 36), (36, 33), (36, 36), (21, 39)])
polygon(ink, [(21, 43), (30, 41), (30, 44), (21, 46)], True)
# Bookmark distinguishes a saved/pinned clip without relying on tiny detail.
polygon(outline, [(36, 38), (47, 35), (47, 56), (41, 53), (36, 59)])
polygon(accent, [(38, 39), (45, 37), (45, 52), (41, 50), (38, 54)])

icon = {'styles': styles, 'paths': paths, 'shapes': shapes}
if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1] / 'resources'
    data = hvif.encode(icon)
    hvif.decode(data)
    (root / 'clipper.hvif').write_bytes(data)
    (root / 'clipper.svg').write_text(hvif.to_svg(icon))
    hvif.preview(icon, 256).save(root / 'clipper-preview.png')
    print(f'Clipper: {len(data)} bytes, HVIF + SVG + preview')
