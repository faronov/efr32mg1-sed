FROM ubuntu:22.04

# Avoid interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive

# Set versions
ENV GECKO_SDK_VERSION=v4.5.0
ENV ARM_TOOLCHAIN_VERSION=12.2.rel1
ENV SLC_VERSION=2024.6.0

# Install dependencies
RUN apt-get update && apt-get install -y \
    wget \
    curl \
    git \
    python3 \
    python3-pip \
    python3-venv \
    default-jre \
    openjdk-11-jdk \
    unzip \
    xz-utils \
    make \
    cmake \
    ninja-build \
    bzip2 \
    libncurses5 \
    libpython2.7 \
    && rm -rf /var/lib/apt/lists/*

# Install ARM GCC toolchain
WORKDIR /opt
RUN wget -q https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_TOOLCHAIN_VERSION}/binrel/arm-gnu-toolchain-${ARM_TOOLCHAIN_VERSION}-x86_64-arm-none-eabi.tar.xz && \
    tar xf arm-gnu-toolchain-${ARM_TOOLCHAIN_VERSION}-x86_64-arm-none-eabi.tar.xz && \
    rm arm-gnu-toolchain-${ARM_TOOLCHAIN_VERSION}-x86_64-arm-none-eabi.tar.xz && \
    ln -s arm-gnu-toolchain-${ARM_TOOLCHAIN_VERSION}-x86_64-arm-none-eabi arm-gcc

ENV PATH="/opt/arm-gcc/bin:${PATH}"

# Clone Gecko SDK from GitHub
WORKDIR /opt
RUN git clone --depth 1 --branch ${GECKO_SDK_VERSION} https://github.com/SiliconLabs/gecko_sdk.git

ENV GECKO_SDK_PATH=/opt/gecko_sdk

# Download and install SLC CLI
WORKDIR /opt
RUN wget -q https://www.silabs.com/documents/login/software/slc_cli_linux.zip && \
    unzip -q slc_cli_linux.zip -d slc_cli && \
    rm slc_cli_linux.zip && \
    chmod +x slc_cli/slc

ENV PATH="/opt/slc_cli:${PATH}"

# Configure SLC with Gecko SDK path
RUN slc configuration --sdk ${GECKO_SDK_PATH}

# Install Python dependencies for build scripts
RUN pip3 install --no-cache-dir \
    pyserial \
    pycryptodome \
    intelhex

# Set up workspace
WORKDIR /workspace

# Copy project files
COPY . .

# Create build output directory
RUN mkdir -p build/debug build/release

# Build script
COPY docker-build.sh /usr/local/bin/build.sh
RUN chmod +x /usr/local/bin/build.sh

# Default command
CMD ["/usr/local/bin/build.sh"]
