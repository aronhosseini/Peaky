#!/bin/bash
# Peaky installer for macOS - developed by Aron H (https://www.instagram.com/aronh.official)
#
#   curl -fsSL https://raw.githubusercontent.com/YOUR_GITHUB_USER/Peaky/main/install-mac.sh | bash
#   ... | bash -s -- --system        (install for all users in /Library, asks for your password)
#
# Files fetched with curl never get the macOS quarantine flag, and this script also strips it
# (xattr -dr com.apple.quarantine) and ad-hoc signs the bundles, so DAWs load Peaky with no Gatekeeper prompt.
set -euo pipefail

REPO="${PEAKY_REPO:-YOUR_GITHUB_USER/Peaky}"
TAG="${PEAKY_VERSION:-latest}"        # e.g. PEAKY_VERSION=v1.0.0
ASSET="Peaky-macOS-plugins.zip"
SCOPE="user"
[ "${1:-}" = "--system" ] && SCOPE="system"

if [ "$TAG" = "latest" ]; then URL="https://github.com/$REPO/releases/latest/download/$ASSET"
else URL="https://github.com/$REPO/releases/download/$TAG/$ASSET"; fi

if [ "$SCOPE" = "system" ]; then BASE="/Library/Audio/Plug-Ins"; SUDO="sudo"
else BASE="$HOME/Library/Audio/Plug-Ins"; SUDO=""; fi

TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
echo "==> Downloading Peaky..."
if [ -n "${PEAKY_ZIP:-}" ]; then cp "$PEAKY_ZIP" "$TMP/$ASSET"; else curl -fL --progress-bar "$URL" -o "$TMP/$ASSET"; fi
ditto -x -k "$TMP/$ASSET" "$TMP/x"

echo "==> Removing quarantine flag and signing..."
xattr -cr "$TMP/x" 2>/dev/null || true
for b in "$TMP"/x/*.vst3 "$TMP"/x/*.component; do [ -e "$b" ] && codesign --force --deep --sign - "$b" 2>/dev/null || true; done

echo "==> Installing to $BASE"
$SUDO mkdir -p "$BASE/VST3" "$BASE/Components"
[ -d "$TMP/x/Peaky.vst3" ]      && { $SUDO rm -rf "$BASE/VST3/Peaky.vst3";           $SUDO cp -R "$TMP/x/Peaky.vst3" "$BASE/VST3/"; }
[ -d "$TMP/x/Peaky.component" ] && { $SUDO rm -rf "$BASE/Components/Peaky.component"; $SUDO cp -R "$TMP/x/Peaky.component" "$BASE/Components/"; }
$SUDO xattr -dr com.apple.quarantine "$BASE/VST3/Peaky.vst3" "$BASE/Components/Peaky.component" 2>/dev/null || true
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo "Done! Rescan plugins in your DAW. Peaky by Aron H - https://www.instagram.com/aronh.official"
