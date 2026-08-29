#!/usr/bin/env python3
from pathlib import Path
import sys
base=Path(sys.argv[1] if len(sys.argv)>1 else '.')
candidates=[]
for cmake in base.rglob('CMakeLists.txt'):
    d=cmake.parent
    if (d/'main').is_dir() and ((d/'main'/'Kconfig.projbuild').exists() or (d/'main'/'CMakeLists.txt').exists()):
        score=0
        if (d/'main'/'mcp_server.cc').exists(): score+=5
        if (d/'main'/'application.cc').exists(): score+=3
        if 'xiaozhi' in d.name.lower(): score+=2
        candidates.append((score,d))
if not candidates:
    raise SystemExit('Could not locate xiaozhi source root under '+str(base))
candidates.sort(key=lambda x:(-x[0],len(str(x[1]))))
print(candidates[0][1].resolve())
