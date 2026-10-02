#!/usr/bin/env bash
# Sync sources and build/install on the workstation. Does not erase history.
set -euo pipefail
CLIPPER_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$CLIPPER_ROOT"
tar -czf - Makefile README.md LICENSE src filter device tools tests resources docs |
    bash tools/ws.sh 'set -e; mkdir -p /boot/home/clipper; tar xzf - --warning=no-timestamp -C /boot/home/clipper 2>/dev/null; cd /boot/home/clipper; make -j8 all check build-haiku/fixture; bash tools/install.sh; make package'
