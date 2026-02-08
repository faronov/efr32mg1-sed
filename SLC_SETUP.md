# SLC CLI Setup Guide

## Issue

The Silicon Labs Configurator (SLC) CLI tool is required to generate build files from the `.slcp` project file. However, SLC CLI requires authentication to download from Silicon Labs' website, which prevents automated Docker builds from succeeding.

## Why This Happens

Silicon Labs requires users to create an account and log in to download their development tools, including SLC CLI. The download URL (`https://www.silabs.com/documents/login/software/slc_cli_linux.zip`) redirects to a login page, causing `wget` in the Dockerfile to fail.

## Solutions

### Solution 1: Manual SLC Download (Recommended for Testing)

1. **Download SLC CLI**:
   - Go to https://www.silabs.com/developers/simplicity-studio
   - Create account / Log in
   - Download "Simplicity Commander" or "SLC CLI" for your platform
   - Extract the archive

2. **Mount SLC into Docker**:
   ```bash
   docker build -t efr32mg1-sed-builder .
   docker run --rm \
     -v $(pwd)/build:/workspace/build \
     -v /path/to/slc_cli:/opt/slc_cli:ro \
     efr32mg1-sed-builder
   ```

### Solution 2: Use Simplicity Studio Locally

1. **Install Simplicity Studio 5**:
   - Download from Silicon Labs website
   - Install on your local machine (Windows/Linux/macOS)

2. **Open Project**:
   - File → Import → More → Silicon Labs → Project from file
   - Select `efr32mg1-sed.slcp`

3. **Build**:
   - Right-click project → Build Project
   - Or use command line:
     ```bash
     slc generate -p efr32mg1-sed.slcp --with arm-gcc --output-type makefile
     make -f efr32mg1-sed.Makefile release
     ```

### Solution 3: Pre-Generated Build Files (For Demonstration)

If you only need to inspect the code structure without building:

1. The source files in `src/` are complete and ready
2. The `.slcp` file defines all project configuration
3. Build files would be generated into `autogen/` by SLC

### Solution 4: Official Silicon Labs Docker Image

Check if Silicon Labs provides an official Docker image with SLC pre-installed:
- https://hub.docker.com/u/silabs
- Community Docker images may exist

## For CI/CD (GitHub Actions)

### Current Status
❌ GitHub Actions build fails because SLC cannot be downloaded

### Workarounds

#### Option A: Self-Hosted Runner
Run GitHub Actions on a self-hosted runner that has SLC CLI installed:
```yaml
runs-on: self-hosted
```

#### Option B: Cached SLC
Store SLC CLI in a private artifact repository and download during build:
```yaml
- name: Download SLC CLI
  run: |
    aws s3 cp s3://your-bucket/slc_cli_linux.zip .
    unzip slc_cli_linux.zip
```

#### Option C: Manual Build Upload
Build locally and upload artifacts manually:
```bash
# Build locally
docker build -t efr32mg1-sed-builder .
docker run --rm -v $(pwd)/build:/workspace/build efr32mg1-sed-builder

# Create release and upload
gh release create v1.0.0 build/release/*.{hex,bin,s37}
```

#### Option D: Skip Build, Provide Pre-Built Artifacts
Commit pre-built artifacts to the repository (not recommended for production):
```
build/
  release/
    efr32mg1-sed.hex
    efr32mg1-sed.bin
    efr32mg1-sed.s37
```

## Recommended Workflow

### For Development:
1. Install Simplicity Studio 5 locally
2. Use SLC CLI from Simplicity Studio installation
3. Build and test locally

### For Production:
1. Set up self-hosted GitHub Actions runner with SLC
2. Or use local build + manual artifact upload
3. Or wait for official Silicon Labs Docker image

## Alternative: Pre-Generated Project

For this specific project, we could provide pre-generated Makefiles and configuration files. However, this defeats the purpose of the `.slcp` system and makes updates harder.

## Future Improvements

We're monitoring for:
- Official Silicon Labs Docker images with SLC pre-installed
- Alternative open-source tools for `.slcp` parsing
- Silicon Labs providing direct download links for automation

## Testing Without Build

You can still:
- ✅ Review source code quality
- ✅ Understand architecture and design
- ✅ Verify API usage and patterns
- ✅ Check documentation completeness
- ❌ Cannot compile without SLC

## Questions?

File an issue on GitHub: https://github.com/faronov/efr32mg1-sed/issues
