#!/bin/bash
set -e

echo "=========================================="
echo "  EFR32MG1 SED Firmware Build"
echo "=========================================="

# Configuration
PROJECT_NAME="efr32mg1-sed"
SLCP_FILE="${PROJECT_NAME}.slcp"

# Check if project file exists
if [ ! -f "${SLCP_FILE}" ]; then
    echo "ERROR: Project file ${SLCP_FILE} not found"
    exit 1
fi

# Check if SLC is available
if ! command -v slc &> /dev/null; then
    echo "=========================================="
    echo "  SLC CLI NOT AVAILABLE"
    echo "=========================================="
    echo ""
    echo "SLC CLI is required to generate build files but is not available."
    echo ""
    echo "REASON: SLC CLI requires authentication to download from Silicon Labs."
    echo ""
    echo "SOLUTIONS:"
    echo "  1. Download SLC CLI manually from:"
    echo "     https://www.silabs.com/developers/simplicity-studio"
    echo "     Then mount it into the Docker container"
    echo ""
    echo "  2. Use Simplicity Studio locally to generate project files"
    echo ""
    echo "  3. Use an official Silicon Labs Docker image (if available)"
    echo ""
    echo "For demonstration, creating stub build output..."
    mkdir -p build/debug build/release
    echo "Stub build - SLC CLI required for actual compilation" > build/debug/README.txt
    echo "Stub build - SLC CLI required for actual compilation" > build/release/README.txt
    exit 1
fi

# Generate project files using SLC
echo ""
echo "Generating project files with SLC..."
slc generate -p "${SLCP_FILE}" --with arm-gcc --output-type makefile --copy-sources

if [ $? -ne 0 ]; then
    echo "ERROR: SLC project generation failed"
    exit 1
fi

echo "Project files generated successfully"

# Build debug version
echo ""
echo "=========================================="
echo "  Building DEBUG version"
echo "=========================================="
make -j$(nproc) -f ${PROJECT_NAME}.Makefile debug

if [ $? -eq 0 ]; then
    echo "Debug build successful"

    # Copy artifacts to build directory
    mkdir -p build/debug
    cp build/debug/${PROJECT_NAME}.hex build/debug/ || true
    cp build/debug/${PROJECT_NAME}.bin build/debug/ || true
    cp build/debug/${PROJECT_NAME}.s37 build/debug/ || true

    # Show file sizes
    arm-none-eabi-size build/debug/${PROJECT_NAME}.axf
else
    echo "ERROR: Debug build failed"
    exit 1
fi

# Build release version
echo ""
echo "=========================================="
echo "  Building RELEASE version"
echo "=========================================="
make -j$(nproc) -f ${PROJECT_NAME}.Makefile release

if [ $? -eq 0 ]; then
    echo "Release build successful"

    # Copy artifacts to build directory
    mkdir -p build/release
    cp build/release/${PROJECT_NAME}.hex build/release/ || true
    cp build/release/${PROJECT_NAME}.bin build/release/ || true
    cp build/release/${PROJECT_NAME}.s37 build/release/ || true

    # Show file sizes
    arm-none-eabi-size build/release/${PROJECT_NAME}.axf
else
    echo "ERROR: Release build failed"
    exit 1
fi

echo ""
echo "=========================================="
echo "  Build Complete!"
echo "=========================================="
echo ""
echo "Artifacts:"
echo "  Debug:   build/debug/"
echo "  Release: build/release/"
echo ""

# List all generated files
find build -type f \( -name "*.hex" -o -name "*.bin" -o -name "*.s37" \) -exec ls -lh {} \;

exit 0
