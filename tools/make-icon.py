#!/usr/bin/env python3
"""Generate the original Clipper clipboard icon as a native Haiku vector icon."""
from pathlib import Path
import hvif
C=hvif.hex_color
styles=[{'color':C(c)} for c in ['173f50','328a98','eefafb','c5e8e8','287280']]
def rect(l,t,r,b): return {'closed':True,'points':[(l,t),(r,t),(r,b),(l,b)]}
paths=[rect(10,10,54,60),rect(13,13,51,57),rect(18,19,46,52),rect(23,4,41,22),rect(27,8,37,12),rect(24,30,40,33),rect(24,39,40,42)]
shapes=[{'style':s,'paths':[p]} for s,p in [(0,0),(1,1),(2,2),(0,3),(3,4),(4,5),(4,6)]]
icon={'styles':styles,'paths':paths,'shapes':shapes}
root=Path(__file__).resolve().parents[1]/'resources'
(root/'clipper.hvif').write_bytes(hvif.encode(icon))
(root/'clipper.svg').write_text(hvif.to_svg(icon))
hvif.preview(icon,256).save(root/'clipper-preview.png')
