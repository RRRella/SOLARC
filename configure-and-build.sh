#!/bin/bash
set -e  # Exit on error

# Defaults
CONFIG="debug"
RENDERER="Vulkan" # Linux default

# Parse arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    debug|release)
      CONFIG="$1"
      shift
      ;;
    -r|--renderer)
      RENDERER="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1"
      echo "Usage: ./configure_and_build.sh [debug|release] [-r DX12|Vulkan]"
      exit 1
      ;;
  esac
done

# Select Preset
if [[ "$CONFIG" == "debug" ]]; then
    PRESET="linux-gcc-debug"
else
    PRESET="linux-gcc-release"
fi

echo "=============================================="
echo "Configuration: $CONFIG"
echo "Preset:        $PRESET"
echo "Renderer:      $RENDERER"
echo "=============================================="

# Configure
cmake --preset "$PRESET" -DSOLARC_RENDERER="$RENDERER"

# Build
echo "Building..."
cmake --build --preset "$PRESET"

# Determine Output Path
# Logic: build/<preset>/out/<Config>/bin
# BASH ${CONFIG^} capitalizes first letter (debug -> Debug)
BINARY_DIR="$PWD/build/$PRESET/out/${CONFIG^}/bin"

echo "Build successful!"
echo "Executables are in: $BINARY_DIR"

# List executables
if [[ -d "$BINARY_DIR" ]]; then
    echo "  Found executables:"
    find "$BINARY_DIR" -maxdepth 1 -type f -executable -printf "    - %f\n"
else
    echo "  Warning: Binary directory not found at expected path."
fi