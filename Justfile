SDK_VERSION := "1.0.0"
SDK_REV     := "r5"
SDK_TAR     := "friction-sdk-" + SDK_VERSION + SDK_REV + "-macOS.tar.xz"
SDK_URL     := "https://github.com/friction2d/friction-sdk/releases/download/v" + SDK_VERSION + "/" + SDK_TAR
SDK_SHA256  := "36a30cb68862d3cd0fe39f9c283f1a9fb9cf2ea01a9dfc65c85024b0c2171d2d"

default:
   @just --list

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
    just build-mac-arm
    # just build-mac-x86_64

build-mac-x86_64:
    arch -x86_64 ./src/scripts/build_mac.sh

build-mac-arm:
    ./src/scripts/build_mac.sh

# Build and then run arm64 build.
build-and-run: build-mac-arm run

# Run the arm64 build
run:
    build-release-arm64/dmg/Friction.app/Contents/MacOS/friction

# Run the debug-arm64 build (output stays in current terminal)
run-debug:
    build-debug-arm64/dmg/Friction.app/Contents/MacOS/friction

run-debug-render:
    QT_LOGGING_RULES="friction.renderoutput=true;friction.canvas=true;friction.videoencoder=true;friction.core=true" just run-debug > log.txt 2>&1;

run-debug-preview:
    QT_LOGGING_RULES="friction.renderhandler=true;friction.cachehandler=true;friction.canvas=true;friction.renderoutput=true;friction.audio=true" just run-debug > log.txt 2>&1;

run-debug-timeline:
    QT_LOGGING_RULES="SvgElementTrack=true;friction.svgflipbooktrack=true" just run-debug > log.txt 2>&1;

# Debug pivot track attachment decisions (read-path filtering + collectPivotDescs + applyPivotToTrack)
debug-pivot-track-attachment:
    QT_LOGGING_RULES="SvgElementTrack=true;friction.svgpivot=true" just run-debug > log.txt 2>&1;

# Produce the universal DMG from the two arch builds
package: build
    #!/usr/bin/env bash
    set -e
    VERSION=$(cat build-release-arm64/version.txt)
    VERSION=${VERSION} ./src/scripts/build_mac_universal.sh

# Full pipeline: deps → sdk → build → package
all: deps build package

# Build debug for native arm64 only (incremental; no DMG)
build-debug: sdk
    #!/usr/bin/env bash
    set -e -x
    CWD=$(pwd)
    CPU=arm64
    SDK="${CWD}/sdk/${CPU}"
    BUILD_DIR="${CWD}/build-debug-arm64"
    export PATH="${SDK}/bin:/usr/bin:/bin:/usr/sbin:/sbin"
    export PKG_CONFIG_PATH="${SDK}/lib/pkgconfig"
    git submodule update --init --recursive
    mkdir -p "${BUILD_DIR}" && cd "${BUILD_DIR}"
    if [ ! -f "CMakeCache.txt" ]; then
        cmake -G Ninja \
            -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
            -DMAC_DEPLOY=ON \
            -DBUILD_SKIA=OFF \
            -DSKIA_STATIC=ON \
            -DSKIA_LIB_PATH="${SDK}/lib" \
            -DCMAKE_BUILD_TYPE=Debug \
            -DQSCINTILLA_INCLUDE_DIRS="${SDK}/include" \
            -DQSCINTILLA_LIBRARIES_DIRS="${SDK}/lib" \
            "${CWD}"
    fi
    cmake --build .
    if [ ! -d "dmg/Friction.app" ]; then
        mv src/app/friction.app src/app/Friction.app
        macdeployqt src/app/Friction.app
        rm -f src/app/Friction.app/Contents/Frameworks/{libQt5MultimediaWidgets.5.dylib,libQt5Svg.5.dylib}
        rm -rf src/app/Friction.app/Contents/PlugIns/{bearer,iconengines,imageformats,mediaservice,printsupport,styles}
        mkdir dmg
        mv src/app/Friction.app dmg/
    else
        cp src/app/friction.app/Contents/MacOS/friction dmg/Friction.app/Contents/MacOS/friction
    fi

# Remove build output directories
clean:
    rm -rf build-release-arm64 build-release-x86_64 build-release-universal build-debug-arm64
