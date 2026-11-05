#!/bin/bash
# Peaky - macOS packaging.  Developed by Aron H
# Produces in installer/mac/out/:
#   Peaky-<ver>-macOS.pkg        installer (VST3 + AU)
#   Peaky-macOS-plugins.zip      raw bundles, used by install-mac.sh (curl) and the Homebrew cask
# Plugins are ad-hoc signed (required for arm64). If DEV_ID_APPLICATION / DEV_ID_INSTALLER are set,
# real Developer ID signing is used instead.
# Usage: installer/mac/build_pkg.sh [version] [build_dir]
set -euo pipefail

VERSION="${1:-1.0.0}"
BUILD_DIR="${2:-build}"
OUT_DIR="installer/mac/out"
WORK="installer/mac/pkgroot"
SCRIPTS="installer/mac/scripts"
chmod +x "$SCRIPTS"/postinstall   # git/zip may drop the executable bit

VST3="$(find "$BUILD_DIR" -type d -name 'Peaky.vst3' | head -n1)"
AU="$(find "$BUILD_DIR" -type d -name 'Peaky.component' | head -n1 || true)"
[ -n "$VST3" ] || { echo "Peaky.vst3 not found in $BUILD_DIR"; exit 1; }

rm -rf "$WORK" "$OUT_DIR"
mkdir -p "$WORK/vst3/Library/Audio/Plug-Ins/VST3" "$WORK/zip" "$OUT_DIR"
cp -R "$VST3" "$WORK/vst3/Library/Audio/Plug-Ins/VST3/"
cp -R "$VST3" "$WORK/zip/"

sign_bundle () {
  if [ -n "${DEV_ID_APPLICATION:-}" ]; then
    codesign --force --deep --options runtime --timestamp --sign "$DEV_ID_APPLICATION" "$1"
  else
    codesign --force --deep --sign - "$1"          # ad-hoc
  fi
}

PKGSIGN=()
[ -n "${DEV_ID_INSTALLER:-}" ] && PKGSIGN=(--sign "$DEV_ID_INSTALLER")

sign_bundle "$WORK/vst3/Library/Audio/Plug-Ins/VST3/Peaky.vst3"
sign_bundle "$WORK/zip/Peaky.vst3"

pkgbuild --root "$WORK/vst3" --identifier com.aronh.peaky.vst3 --version "$VERSION" \
         --scripts "$SCRIPTS" --install-location / ${PKGSIGN[@]+"${PKGSIGN[@]}"} "$OUT_DIR/Peaky-VST3.pkg"

CHOICES='<line choice="vst3"/>'
PKGREFS='<pkg-ref id="com.aronh.peaky.vst3">Peaky-VST3.pkg</pkg-ref>'
CHOICEDEFS='<choice id="vst3" title="Peaky VST3" description="Installs to /Library/Audio/Plug-Ins/VST3"><pkg-ref id="com.aronh.peaky.vst3"/></choice>'

if [ -n "$AU" ]; then
  mkdir -p "$WORK/au/Library/Audio/Plug-Ins/Components"
  cp -R "$AU" "$WORK/au/Library/Audio/Plug-Ins/Components/"
  cp -R "$AU" "$WORK/zip/"
  sign_bundle "$WORK/au/Library/Audio/Plug-Ins/Components/Peaky.component"
  sign_bundle "$WORK/zip/Peaky.component"
  pkgbuild --root "$WORK/au" --identifier com.aronh.peaky.au --version "$VERSION" \
           --scripts "$SCRIPTS" --install-location / ${PKGSIGN[@]+"${PKGSIGN[@]}"} "$OUT_DIR/Peaky-AU.pkg"
  CHOICES="$CHOICES<line choice=\"au\"/>"
  PKGREFS="$PKGREFS<pkg-ref id=\"com.aronh.peaky.au\">Peaky-AU.pkg</pkg-ref>"
  CHOICEDEFS="$CHOICEDEFS<choice id=\"au\" title=\"Peaky Audio Unit\" description=\"Installs to /Library/Audio/Plug-Ins/Components\"><pkg-ref id=\"com.aronh.peaky.au\"/></choice>"
fi

cat > "$OUT_DIR/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>Peaky by Aron H</title>
  <options customize="allow" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <choices-outline>$CHOICES</choices-outline>
  $PKGREFS
  $CHOICEDEFS
</installer-gui-script>
XML

productbuild --distribution "$OUT_DIR/distribution.xml" --package-path "$OUT_DIR" \
             ${PKGSIGN[@]+"${PKGSIGN[@]}"} "$OUT_DIR/Peaky-$VERSION-macOS.pkg"

# raw bundles for curl / Homebrew installs (ditto keeps signatures and symlinks intact)
ditto -c -k --sequesterRsrc "$WORK/zip" "$OUT_DIR/Peaky-macOS-plugins.zip"
rm -f "$OUT_DIR/Peaky-VST3.pkg" "$OUT_DIR/Peaky-AU.pkg" "$OUT_DIR/distribution.xml"

echo "Created:"; ls -1 "$OUT_DIR"
echo "sha256 (plugins zip): $(shasum -a 256 "$OUT_DIR/Peaky-macOS-plugins.zip" | cut -d' ' -f1)"
