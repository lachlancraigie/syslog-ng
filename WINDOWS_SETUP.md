# Windows Development Environment Setup for syslog-ng Azure DCR

This guide helps you set up the development environment on Windows to build syslog-ng with Azure DCR authentication.

## Option 1: Using MSYS2 (Recommended)

MSYS2 provides a Unix-like environment on Windows with package management.

### Install MSYS2
1. Download MSYS2 from https://www.msys2.org/
2. Install to `C:\msys64` (default location)
3. Update the package database:
   ```bash
   pacman -Syu
   ```

### Install Dependencies
```bash
# Update package database
pacman -Syu

# Install build tools
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-ninja

# Install syslog-ng dependencies
pacman -S mingw-w64-x86_64-glib2
pacman -S mingw-w64-x86_64-curl
pacman -S mingw-w64-x86_64-json-c
pacman -S mingw-w64-x86_64-openssl
pacman -S mingw-w64-x86_64-pkg-config

# Install development tools
pacman -S git
pacman -S autotools
```

### Build Commands (MSYS2)
```bash
# Navigate to syslog-ng directory
cd /f/OneDrive/syslog/syslog-ng

# Configure
cmake -B build -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_CURL=ON \
  -DENABLE_JSON=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build HTTP module with Azure authentication
cmake --build build --target http -j 4

# Test Azure authentication
cmake --build build --target test_azure_auth
./build/modules/http/tests/test_azure_auth.exe
```

## Option 2: Using vcpkg (Visual Studio)

If you prefer Visual Studio, use vcpkg for dependency management.

### Install vcpkg
```cmd
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

### Install Dependencies
```cmd
vcpkg install curl:x64-windows
vcpkg install json-c:x64-windows
vcpkg install glib:x64-windows
vcpkg install openssl:x64-windows
```

### Build Commands (Visual Studio)
```cmd
cmake -B build -G "Visual Studio 16 2019" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DENABLE_CURL=ON ^
  -DENABLE_JSON=ON ^
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build --config Debug --target http
```

## Option 3: Using Chocolatey

Install dependencies using Chocolatey package manager.

### Install Chocolatey
Run PowerShell as Administrator:
```powershell
Set-ExecutionPolicy Bypass -Scope Process -Force
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
```

### Install Dependencies
```cmd
choco install cmake
choco install mingw
choco install git
```

## Verification

After installing dependencies, verify they're available:

```bash
# Check versions
cmake --version
gcc --version
pkg-config --version

# Check libraries
pkg-config --cflags --libs glib-2.0
pkg-config --cflags --libs libcurl
pkg-config --cflags --libs json-c
```

## Building the Azure DCR Feature

Once dependencies are installed:

```bash
# Navigate to project
cd /f/OneDrive/syslog/syslog-ng

# Configure with Azure DCR support
cmake -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_CURL=ON \
  -DENABLE_JSON=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build HTTP module
cmake --build build --target http -j 4

# Run tests
cmake --build build --target test_azure_auth
./build/modules/http/tests/test_azure_auth
```

## Troubleshooting

### Common Issues

1. **CMake not found**: Add CMake to PATH or use full path
2. **Missing pkg-config**: Install pkg-config for dependency detection
3. **Library not found**: Check PKG_CONFIG_PATH environment variable
4. **Compiler errors**: Ensure mingw-w64 is in PATH

### Environment Variables (MSYS2)
Add to your PATH:
- `C:\msys64\mingw64\bin`
- `C:\msys64\usr\bin`

### VS Code Integration
Update the c_cpp_properties.json paths to match your installation:
- MSYS2: `C:/msys64/mingw64/include/*`
- vcpkg: `C:/vcpkg/installed/x64-windows/include`

## Next Steps

1. Choose your preferred development environment
2. Install the dependencies
3. Configure the build with CMake
4. Build and test the Azure DCR authentication

The Azure DCR authentication feature is ready to build once the environment is set up!