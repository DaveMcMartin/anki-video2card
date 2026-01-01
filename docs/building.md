# Building Guide

This guide explains how to build Anki Video2Card from source on macOS, Linux, and Windows.

## Prerequisites

Before building, ensure you have completed the [Installation Guide](installation.md) and have all dependencies installed.

## Quick Start

For experienced users, here's the quick version:

```bash
git clone https://github.com/DaveMcMartin/anki-video2card.git
cd anki-video2card
python3 scripts/download_translation_model.py
python3 scripts/convert_pitch_accent.py
mkdir build && cd build
cmake ..
cmake --build .
```

## Detailed Build Instructions

### 1. Clone the Repository

```bash
git clone https://github.com/DaveMcMartin/anki-video2card.git
cd anki-video2card
```

### 2. Prepare Assets

#### Download Translation Model

The translation model is required for offline Japanese-English translation:

```bash
# Install Python dependencies if not already installed
pip3 install huggingface-hub

# Download the model
python3 scripts/download_translation_model.py
```

This creates `assets/translation_model/` with the CTranslate2 model.

#### Generate Pitch Accent Database (Optional)

For pitch accent support:

```bash
python3 scripts/convert_pitch_accent.py
```

This creates `assets/pitch_accent.db`. If skipped, the application will function without pitch accent features.

### 3. Configure the Project

#### macOS/Linux

```bash
mkdir build
cd build
cmake ..
```

#### Windows (Visual Studio)

```powershell
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake ..
```

**Note**: Adjust the vcpkg path if you installed it elsewhere.

### 4. Build

#### macOS/Linux

```bash
# Use all available cores for faster compilation
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
```

#### Windows

```powershell
cmake --build . --config Release
```

### 5. Run the Application

#### macOS

```bash
./bin/Anki\ Video2Card.app/Contents/MacOS/Anki\ Video2Card
```

Or open the app bundle:
```bash
open bin/Anki\ Video2Card.app
```

#### Linux

```bash
./bin/Anki\ Video2Card
```

#### Windows

```powershell
.\bin\Release\Anki Video2Card.exe
```

Or simply:
```powershell
.\bin\Anki Video2Card.exe
```

## Build Configurations

### Debug Build

For development with debug symbols:

```bash
mkdir build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

### Release Build (Optimized)

For production use with optimizations:

```bash
mkdir build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## Advanced Build Options

### Custom Install Prefix

To install to a custom location:

```bash
cmake -DCMAKE_INSTALL_PREFIX=/custom/path ..
cmake --build .
cmake --install .
```

### Specify Compiler

To use a specific compiler:

```bash
# Use Clang
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ..

# Use GCC
cmake -DCMAKE_C_COMPILER=gcc-13 -DCMAKE_CXX_COMPILER=g++-13 ..
```

### Parallel Build

Control the number of parallel jobs:

```bash
cmake --build . -j8  # Use 8 cores
```

## Project Structure

After building, your directory structure will look like:

```
anki-video2card/
├── assets/
│   ├── translation_model/     # CTranslate2 model (if downloaded)
│   ├── pitch_accent.db        # Pitch accent database (if generated)
│   └── ...                    # Other assets
├── build/
│   ├── bin/                   # Executable location
│   │   └── Anki Video2Card.app (macOS)
│   │   └── Anki Video2Card     (Linux)
│   │   └── Anki Video2Card.exe (Windows)
│   └── ...                    # Build artifacts
├── src/                       # Source code
└── ...
```

## Clean Build

To start fresh:

```bash
rm -rf build
mkdir build
cd build
cmake ..
cmake --build .
```

Or on Windows:
```powershell
Remove-Item -Recurse -Force build
mkdir build
cd build
cmake ..
cmake --build .
```

## Code Formatting

The project uses clang-format for consistent code style. To format all source files:

```bash
bash scripts/format.sh
```

This script formats all `.cpp`, `.hpp`, `.h`, and `.c` files in the `src/` directory.

## Troubleshooting

### CMake Can't Find Dependencies

**macOS:**
```bash
# Ensure Homebrew packages are visible
export CMAKE_PREFIX_PATH="/opt/homebrew:$CMAKE_PREFIX_PATH"
cmake ..
```

**Linux:**
```bash
# Set pkg-config path if needed
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
cmake ..
```

**Windows:**
```powershell
# Ensure vcpkg toolchain is specified
cmake -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake ..
```

### CTranslate2 Not Found

If CMake can't find CTranslate2:

```bash
# macOS
export CMAKE_PREFIX_PATH="/opt/homebrew:$CMAKE_PREFIX_PATH"

# Linux
export CMAKE_PREFIX_PATH="/usr/local:$CMAKE_PREFIX_PATH"

# Then reconfigure
cmake ..
```

### MeCab Not Found

**macOS:**
```bash
# If installed via Homebrew
brew link mecab
```

**Linux:**
```bash
sudo apt-get install mecab libmecab-dev mecab-ipadic-utf8
```

### Missing SDL3 or ImGui

These are automatically fetched by CMake via FetchContent. If you encounter issues:

1. Ensure you have a stable internet connection
2. Clear CMake cache: `rm -rf build/_deps`
3. Reconfigure and rebuild

### Compilation Errors

**C++23 not supported:**
- Ensure you have a compatible compiler (see Prerequisites)
- Update your compiler to the latest version

**Linker errors:**
- Verify all dependencies are installed correctly
- Check that library paths are in your system's library search path
- On Linux, run `sudo ldconfig` after installing libraries

### Runtime Errors

**Missing shared libraries:**

**macOS:**
```bash
# Check what libraries are missing
otool -L bin/Anki\ Video2Card.app/Contents/MacOS/Anki\ Video2Card
```

**Linux:**
```bash
ldd bin/Anki\ Video2Card
```

**Windows:**
- Ensure all DLLs from vcpkg are in the same directory as the executable
- Or add vcpkg's bin directory to your PATH

## Development Workflow

1. Make changes to source files
2. Format code: `bash scripts/format.sh`
3. Rebuild: `cmake --build build`
4. Test the application
5. Repeat

For faster iteration, you can build only changed files:
```bash
cd build
make  # or 'ninja' if you're using ninja generator
```

## IDE Integration

### Visual Studio Code

Recommended extensions:
- C/C++ (Microsoft)
- CMake Tools
- clangd

### CLion

CLion has native CMake support. Simply open the project directory and CLion will detect the CMakeLists.txt.

### Visual Studio (Windows)

1. Open Visual Studio
2. File → Open → CMake
3. Select the project's CMakeLists.txt
4. Visual Studio will automatically configure and build

## Continuous Integration

The project includes GitHub Actions workflows for automated building and testing (when available). Check `.github/workflows/` for CI configuration.

## Next Steps

After successfully building the application, see the main [README](../README.md) for usage instructions.