#!/usr/bin/env bash
set -euo pipefail
CLIPPER_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$CLIPPER_ROOT"
BUILD=${BUILD:-build-haiku}
NAME=$(awk '$1 == "name" { print $2; exit }' resources/Clipper.PackageInfo)
VERSION=$(awk '$1 == "version" { print $2; exit }' resources/Clipper.PackageInfo)
ARCH=$(awk '$1 == "architecture" { print $2; exit }' resources/Clipper.PackageInfo)
make BUILD="$BUILD" all
STAGE=$(mktemp -d /tmp/clipper-package-XXXXXX)
trap 'rm -rf -- "$STAGE"' EXIT
mkdir -p "$STAGE/apps" "$STAGE/add-ons/input_server/filters" "$STAGE/add-ons/input_server/devices" \
    "$STAGE/data/deskbar/menu/Applications" "$STAGE/data/licenses" "$STAGE/documentation/packages/clipper" artifacts
cp "$BUILD/Clipper" "$STAGE/apps/Clipper"
strip --strip-debug "$STAGE/apps/Clipper"
xres -o "$STAGE/apps/Clipper" "$BUILD/Clipper.rsrc"
cp "$BUILD/Clipper_filter" "$STAGE/add-ons/input_server/filters/Clipper_shortcuts"
cp "$BUILD/Clipper_device" "$STAGE/add-ons/input_server/devices/Clipper_paste"
strip --strip-debug "$STAGE/add-ons/input_server/filters/Clipper_shortcuts" "$STAGE/add-ons/input_server/devices/Clipper_paste"
cp resources/Clipper.PackageInfo "$STAGE/.PackageInfo"
cp LICENSE "$STAGE/data/licenses/MIT"
cp README.md LICENSE "$STAGE/documentation/packages/clipper/"
cp -R docs "$STAGE/documentation/packages/clipper/"
ln -s ../../../../apps/Clipper "$STAGE/data/deskbar/menu/Applications/Clipper"
(cd "$STAGE" && mimeset -f --all --mimedb data/mime_db --mimedb /boot/system/data/mime_db apps/Clipper)
package create -C "$STAGE" "$CLIPPER_ROOT/artifacts/$NAME-$VERSION-$ARCH.hpkg"
