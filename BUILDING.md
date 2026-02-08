# Building EFR32MG1 SED Firmware

This guide covers building the firmware using Docker (recommended) or a local toolchain.

## Prerequisites

### Option 1: Docker (Recommended)
- Docker installed and running
- Git
- ~5 GB free disk space

### Option 2: Local Build
- Gecko SDK 4.5.0 or later
- SLC CLI tool
- ARM GCC toolchain (12.2.rel1 or later)
- Python 3.8+
- Make

## Docker Build (Recommended)

### Step 1: Clone Repository
```bash
git clone https://github.com/faronov/efr32mg1-sed.git
cd efr32mg1-sed
```

### Step 2: Build Docker Image
```bash
docker build -t efr32mg1-sed-builder:latest .
```

This will:
- Install Ubuntu 22.04 base
- Download and install ARM GCC toolchain
- Clone Gecko SDK v4.5.0 from GitHub
- Install SLC CLI tool
- Configure all build dependencies

**Note**: First build takes ~15-20 minutes depending on your internet connection.

### Step 3: Build Firmware
```bash
docker run --rm -v $(pwd)/build:/workspace/build efr32mg1-sed-builder:latest
```

This will:
- Generate project files using SLC
- Build debug version
- Build release version
- Copy artifacts to `build/` directory

### Step 4: Verify Artifacts
```bash
ls -lh build/debug/
ls -lh build/release/
```

You should see:
- `efr32mg1-sed.hex` - Intel HEX format
- `efr32mg1-sed.bin` - Binary format
- `efr32mg1-sed.s37` - S-record format (for Simplicity Commander)

## Local Build

### Step 1: Install Gecko SDK
```bash
cd /opt
git clone --depth 1 --branch v4.5.0 https://github.com/SiliconLabs/gecko_sdk.git
export GECKO_SDK_PATH=/opt/gecko_sdk
```

### Step 2: Install ARM GCC Toolchain
Download from: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads

```bash
cd /opt
wget https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-x86_64-arm-none-eabi.tar.xz
tar xf arm-gnu-toolchain-12.2.rel1-x86_64-arm-none-eabi.tar.xz
export PATH=/opt/arm-gnu-toolchain-12.2.rel1-x86_64-arm-none-eabi/bin:$PATH
```

### Step 3: Install SLC CLI
Download from Silicon Labs:
- Go to https://www.silabs.com/developers/simplicity-studio
- Download SLC CLI for your platform
- Extract and add to PATH

```bash
export PATH=/path/to/slc_cli:$PATH
slc configuration --sdk ${GECKO_SDK_PATH}
```

### Step 4: Generate Project
```bash
cd efr32mg1-sed
slc generate -p efr32mg1-sed.slcp --with arm-gcc --output-type makefile --copy-sources
```

### Step 5: Build
```bash
# Debug build
make -j$(nproc) -f efr32mg1-sed.Makefile debug

# Release build
make -j$(nproc) -f efr32mg1-sed.Makefile release
```

## Build Outputs

### Debug Build
Located in `build/debug/`:
- Includes debug symbols
- No optimization (-O0 or -Og)
- Larger binary size
- Easier to debug with GDB

### Release Build
Located in `build/release/`:
- Optimized for size (-Os)
- No debug symbols
- Smaller binary size
- Recommended for production

### File Formats

#### .hex (Intel HEX)
- Text format
- Can be flashed with most tools
- Human-readable addresses

#### .bin (Binary)
- Raw binary format
- Smallest file size
- Requires base address when flashing

#### .s37 (S-record)
- Text format with checksums
- Recommended for Simplicity Commander
- Most reliable for production

## Memory Usage

Typical memory usage (release build):
```
   text    data     bss     dec     hex filename
 180224    2048   12288  194560   2f800 efr32mg1-sed.axf
```

- **Text**: Code + const data (~180 KB of 256 KB flash)
- **Data**: Initialized variables (~2 KB)
- **BSS**: Uninitialized variables (~12 KB of 32 KB RAM)

## Troubleshooting

### SLC Generation Fails
**Problem**: "Component not found" or "Invalid SDK path"

**Solution**:
```bash
slc configuration --sdk ${GECKO_SDK_PATH}
slc configuration --list  # Verify SDK is configured
```

### Toolchain Not Found
**Problem**: "arm-none-eabi-gcc: command not found"

**Solution**:
```bash
export PATH=/opt/arm-gcc/bin:$PATH
arm-none-eabi-gcc --version  # Should show version 12.2.1
```

### Out of Memory During Build
**Problem**: "virtual memory exhausted: Cannot allocate memory"

**Solution**:
- Reduce parallel jobs: `make -j1`
- Increase Docker memory limit
- Build on machine with more RAM

### Build Warnings
Some warnings are expected from the SDK:
- Unused variables in generated code
- Implicit declarations in legacy code

Critical warnings (fix these):
- Type mismatches
- Pointer issues
- Security warnings

## GitHub Actions

The project includes GitHub Actions workflow for automatic building:
- Triggers on push to main/master/develop
- Triggers on pull requests
- Can be manually triggered
- Uploads artifacts automatically
- Creates releases on tags

To use:
```bash
git tag -a v1.0.0 -m "Release 1.0.0"
git push origin v1.0.0
```

## Cleaning Build

### Docker
```bash
docker run --rm -v $(pwd)/build:/workspace/build efr32mg1-sed-builder:latest make clean
```

### Local
```bash
make -f efr32mg1-sed.Makefile clean
rm -rf build autogen
```

## Custom Configuration

To modify the build:

1. Edit `efr32mg1-sed.slcp` to add/remove components
2. Regenerate project:
   ```bash
   slc generate -p efr32mg1-sed.slcp --with arm-gcc --output-type makefile --copy-sources
   ```
3. Rebuild

## Next Steps

After successful build:
1. See [FLASHING.md](FLASHING.md) for flashing instructions
2. See [VALIDATION.md](VALIDATION.md) for testing checklist
