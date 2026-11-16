#!/usr/bin/env bash
# Copies this mod into a Dusklight checkout as an in-tree mod so it gets bundled into the app.
# Usage: inject.sh <mod_dir> <dusklight_dir> [bundle_id]
set -euo pipefail

MOD_DIR="$1"
DUSK_DIR="$2"
BUNDLE_ID="${3:-}"
DEST="$DUSK_DIR/mods/tp_compass"

rm -rf "$DEST"
mkdir -p "$DEST"
# Only what the build needs.
cp "$MOD_DIR/CMakeLists.txt" "$MOD_DIR/mod.json" "$DEST/"
cp -R "$MOD_DIR/src" "$DEST/"
if [ -d "$MOD_DIR/res" ]; then
    cp -R "$MOD_DIR/res" "$DEST/"
fi

CMAKELISTS="$DUSK_DIR/CMakeLists.txt"
if ! grep -q "add_subdirectory(mods/tp_compass)" "$CMAKELISTS"; then
    # Register right after the last in-tree mod, inside the DUSK_ENABLE_CODE_MODS block.
    python3 - "$CMAKELISTS" <<'PY'
import sys
path = sys.argv[1]
src = open(path).read()
anchor = "    add_subdirectory(mods/audio_mod)\n"
if anchor not in src:
    sys.exit("inject.sh: could not find the in-tree mods block in CMakeLists.txt")
src = src.replace(anchor, anchor + "    add_subdirectory(mods/tp_compass)\n", 1)
open(path, "w").write(src)
PY
fi
grep -n "add_subdirectory(mods/tp_compass)" "$CMAKELISTS"

if [ -n "$BUNDLE_ID" ]; then
    echo "Overriding bundle identifier -> $BUNDLE_ID"
    sed -i.bak "s/set(DUSK_BUNDLE_IDENTIFIER dev.twilitrealm.dusk)/set(DUSK_BUNDLE_IDENTIFIER $BUNDLE_ID)/" "$CMAKELISTS"
    grep -n "DUSK_BUNDLE_IDENTIFIER" "$CMAKELISTS" | head -1
fi

echo "Injected TP Compass into $DEST"
