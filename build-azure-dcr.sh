#!/bin/bash

# Build script for syslog-ng with Azure DCR authentication
# This script helps set up the build environment on Windows

echo "Setting up syslog-ng build with Azure DCR authentication..."

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the syslog-ng root directory"
    exit 1
fi

# Windows-specific environment setup
echo "Setting up Windows build environment..."
export PATH="$PATH:/c/Program Files (x86)/GnuWin32/bin"

# Check for required tools
echo "Checking for required build tools..."
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake not found. Please install CMake."
    exit 1
fi

if ! command -v gcc &> /dev/null; then
    echo "Error: GCC not found. Please install MinGW-w64."
    exit 1
fi

if ! command -v mingw32-make &> /dev/null; then
    echo "Error: MinGW make not found. Please install MinGW-w64."
    exit 1
fi

# Check BISON version
BISON_VERSION=$(bison --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')
REQUIRED_BISON="3.7.6"
if ! command -v bison &> /dev/null; then
    echo "Warning: BISON not found. This may cause build issues."
elif [ "$(printf '%s\n' "$REQUIRED_BISON" "$BISON_VERSION" | sort -V | head -n1)" != "$REQUIRED_BISON" ]; then
    echo "Warning: BISON version $BISON_VERSION is older than required $REQUIRED_BISON"
    echo "Consider using pre-built grammar files or installing a newer BISON"
fi

# Try to use pre-generated grammar files if available
echo "Checking for pre-generated grammar files..."

# Create build directory
mkdir -p build
cd build

# Configure with required options for Azure DCR
echo "Configuring CMake..."

# Try with minimal configuration first to avoid BISON issues
cmake -G "MinGW Makefiles" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_CURL=ON \
    -DENABLE_JSON=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_MAKE_PROGRAM=mingw32-make \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER=g++ \
    .. 2>&1 | tee cmake_output.log

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo ""
    echo "❌ CMake configuration failed."
    echo ""
    echo "Common issues on Windows:"
    echo "1. BISON version too old (need 3.7.6+, found $BISON_VERSION)"
    echo "2. Missing dependencies: libcurl, json-c, glib2"
    echo "3. Grammar generation issues"
    echo ""
    echo "Possible solutions:"
    echo "1. Install newer BISON from MSYS2:"
    echo "   - Install MSYS2 from https://www.msys2.org/"
    echo "   - Run: pacman -S mingw-w64-x86_64-bison"
    echo ""
    echo "2. Use pre-built syslog-ng binaries and add Azure DCR as plugin"
    echo ""
    echo "3. Use Docker container with proper build environment"
    echo ""
    echo "For now, you can:"
    echo "a) Download pre-built syslog-ng from official releases"
    echo "b) Compile only the Azure DCR module as a plugin"
    echo "c) Use the source code as reference for integration"
    echo ""
    exit 1
fi

# Build the HTTP module with Azure authentication
echo "Building HTTP module..."
cmake --build . --target http -j 4

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Build successful!"
    echo ""
    echo "Azure DCR authentication has been built into the HTTP module."
    echo ""
    echo "To test the Azure authentication module:"
    echo "  cmake --build . --target test_azure_auth"
    echo "  ./modules/http/tests/test_azure_auth"
    echo ""
    echo "To build all modules:"
    echo "  cmake --build . -j 4"
    echo ""
else
    echo "❌ Build failed. Check the error messages above."
    exit 1
fi