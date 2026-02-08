#!/bin/bash
set -euo pipefail

echo "=========================================="
echo "  EFR32MG1 SED Firmware Build"
echo "=========================================="

# Configuration
PROJECT_NAME="efr32mg1-sed"
SLCP_FILE="${SLCP_FILE:-${PROJECT_NAME}.slcp}"
BUILD_DIR="build"

# Set HOME if not set
export HOME="${HOME:-/root}"

# Set GSDK_DIR if not set
if [ -z "${GSDK_DIR:-}" ]; then
    # Try to find Gecko SDK
    if [ -d "/gecko_sdk_4.5.0" ]; then
        export GSDK_DIR="/gecko_sdk_4.5.0"
    elif [ -d "/opt/gecko_sdk" ]; then
        export GSDK_DIR="/opt/gecko_sdk"
    else
        echo "ERROR: GSDK_DIR not set and Gecko SDK not found"
        exit 1
    fi
fi

echo "Using Gecko SDK: $GSDK_DIR"

# Find and add GCC to PATH
echo "Looking for GCC ARM toolchain..."

GCC_PKG=$(find "$HOME/.silabs/slt/installs/conan/p" -maxdepth 1 -type d -name "gcc-*" 2>/dev/null | grep -v "gcc-a51b5" | head -1 || true)

if [ -z "$GCC_PKG" ]; then
    echo "WARNING: GCC not found in slt conan packages"
    echo "Checking for system GCC..."
    if command -v arm-none-eabi-gcc &> /dev/null; then
        echo "✓ Using system GCC: $(which arm-none-eabi-gcc)"
    else
        echo "ERROR: No ARM GCC toolchain found!"
        exit 1
    fi
else
    GCC_BIN="$GCC_PKG/p/bin"

    if [ ! -d "$GCC_BIN" ]; then
        echo "ERROR: GCC bin directory not found at: $GCC_BIN"
        exit 1
    fi

    echo "Found GCC at: $GCC_BIN"
    export PATH="$GCC_BIN:$PATH"

    # Verify GCC
    if "$GCC_BIN/arm-none-eabi-gcc" --version > /dev/null 2>&1; then
        echo "✓ GCC is executable"
        "$GCC_BIN/arm-none-eabi-gcc" --version | head -1
    else
        echo "ERROR: Cannot execute arm-none-eabi-gcc!"
        exit 1
    fi
fi

# Find SLC
SLC_CMD=$(which slc || echo "$HOME/.local/bin/slc" || true)
if [ -z "$SLC_CMD" ] || [ ! -x "$SLC_CMD" ]; then
    echo "ERROR: SLC command not found"
    exit 1
fi

echo "✓ SLC: $SLC_CMD"
echo "✓ Building project: $SLCP_FILE"

# Check if project file exists
if [ ! -f "$SLCP_FILE" ]; then
    echo "ERROR: Project file $SLCP_FILE not found"
    exit 1
fi

# Clean previous build
echo ""
echo "Cleaning previous build..."
rm -rf autogen/ "$BUILD_DIR/" *.Makefile

# Generate project files using SLC
echo ""
echo "Generating project files with SLC..."
"$SLC_CMD" generate \
    -p "$SLCP_FILE" \
    --with arm-gcc \
    --output-type makefile \
    --copy-sources

if [ $? -ne 0 ]; then
    echo "ERROR: SLC project generation failed"
    exit 1
fi

echo "✓ Project files generated successfully"

# Build release version
echo ""
echo "=========================================="
echo "  Building RELEASE version"
echo "=========================================="

make -j$(nproc) -f "${PROJECT_NAME}.Makefile" release

if [ $? -eq 0 ]; then
    echo "✓ Release build successful"

    # Show file sizes
    echo ""
    echo "Memory usage:"
    arm-none-eabi-size "$BUILD_DIR/release/${PROJECT_NAME}.axf"

    # List artifacts
    echo ""
    echo "Build artifacts:"
    find "$BUILD_DIR/release" -type f \( -name "*.hex" -o -name "*.bin" -o -name "*.s37" -o -name "*.out" \) -exec ls -lh {} \;
else
    echo "ERROR: Release build failed"
    exit 1
fi

echo ""
echo "=========================================="
echo "  Build Complete!"
echo "=========================================="

exit 0
