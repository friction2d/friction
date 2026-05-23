SDK_VERSION := "1.0.0"
SDK_REV     := "r5"
SDK_TAR     := "friction-sdk-" + SDK_VERSION + SDK_REV + "-macOS.tar.xz"
SDK_URL     := "https://github.com/friction2d/friction-sdk/releases/download/v" + SDK_VERSION + "/" + SDK_TAR
SDK_SHA256  := "36a30cb68862d3cd0fe39f9c283f1a9fb9cf2ea01a9dfc65c85024b0c2171d2d"

REL := env_var_or_default("REL", "OFF")

default: all

# Check and install required Homebrew tools
deps:
    #!/usr/bin/env bash
    set -e
    if ! command -v brew &>/dev/null; then
        echo "Error: Homebrew is not installed. Install it from https://brew.sh then re-run."
        exit 1
    fi
    brew install cmake ninja python pkg-config

# Download and SHA256-verify the SDK tarball into ./sdk/, skipping if already present
sdk:
    #!/usr/bin/env bash
    set -e
    if [ -d "sdk" ]; then
        echo "SDK already present, skipping download."
        exit 0
    fi
    echo "Downloading SDK..."
    curl -L -o "{{SDK_TAR}}" "{{SDK_URL}}"
    echo "Verifying SHA256..."
    echo "{{SDK_SHA256}}  {{SDK_TAR}}" | shasum -a 256 --check
    echo "Extracting SDK..."
    tar xf "{{SDK_TAR}}"
    rm "{{SDK_TAR}}"

# Build for arm64 (native) and x86_64, after updating submodules
build: sdk
    git submodule update --init --recursive
    just build-mac-x86_64
    just build-mac-arm

build-mac-x86_64:
    arch -x86_64 ./src/scripts/build_mac.sh

build-mac-arm:
    ./src/scripts/build_mac.sh

# Build and then run arm64 build.
build-and-run: build-mac-arm run

# Run the arm64 build
run:
    open build-release-arm64/dmg/Friction.app

# Produce the universal DMG from the two arch builds
package: build
    #!/usr/bin/env bash
    set -e
    REL="{{REL}}"
    VERSION=$(cat build-release-arm64/version.txt)
    if [ "$REL" != "ON" ]; then
        COMMIT=$(git rev-parse --short=8 HEAD)
        VERSION="${VERSION}-${COMMIT}"
    fi
    VERSION=${VERSION} ./src/scripts/build_mac_universal.sh

# Full pipeline: deps → sdk → build → package
all: deps build package

# Remove build output directories
clean:
    rm -rf build-release-arm64 build-release-x86_64 build-release-universal
