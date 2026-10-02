#!/usr/bin/env bash
# Native per-user install; add-ons have different names because input_server's
# add-on monitor compares filenames across device/filter directories.
set -euo pipefail
CLIPPER_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$CLIPPER_ROOT"
BUILD=${1:-build-haiku}
if [[ $BUILD == --uninstall ]]; then
    hey application/x-vnd.airOS-Clipper quit >/dev/null 2>&1 || true
    desklink --remove=Clipper >/dev/null 2>&1 || true
    rm -f "$HOME/config/settings/boot/launch/Clipper" \
        "$HOME/config/non-packaged/apps/Clipper" \
        "$HOME/config/non-packaged/data/deskbar/menu/Applications/Clipper" \
        "$HOME/config/non-packaged/add-ons/input_server/filters/Clipper_shortcuts" \
        "$HOME/config/non-packaged/add-ons/input_server/devices/Clipper_paste"
    echo "Clipper removed. Saved history remains in ~/config/settings/Clipper."
    exit 0
fi
make BUILD="$BUILD" all
hey application/x-vnd.airOS-Clipper quit >/dev/null 2>&1 || true
desklink --remove=Clipper >/dev/null 2>&1 || true
sleep 1
APPS="$HOME/config/non-packaged/apps"
FILTERS="$HOME/config/non-packaged/add-ons/input_server/filters"
DEVICES="$HOME/config/non-packaged/add-ons/input_server/devices"
MENU="$HOME/config/non-packaged/data/deskbar/menu/Applications"
mkdir -p "$APPS" "$FILTERS" "$DEVICES" "$MENU" "$HOME/config/settings/boot/launch"
cp "$BUILD/Clipper" "$APPS/Clipper.new"
mv -f "$APPS/Clipper.new" "$APPS/Clipper"
mimeset -f "$APPS/Clipper"
rm -f "$FILTERS/Clipper_shortcuts" "$DEVICES/Clipper_paste"
sleep 1
cp "$BUILD/Clipper_filter" "$FILTERS/Clipper_shortcuts"
cp "$BUILD/Clipper_device" "$DEVICES/Clipper_paste"
ln -sf "$APPS/Clipper" "$HOME/config/settings/boot/launch/Clipper"
ln -sf "$APPS/Clipper" "$MENU/Clipper"
nohup "$APPS/Clipper" > /tmp/clipper.log 2>&1 &
echo "Clipper installed with login startup."
