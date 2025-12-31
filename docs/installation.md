# Installation Guide

This guide will help you install all dependencies required to build and run Anki Video2Card on macOS, Linux, and Windows.

## Prerequisites

- **C++ Compiler**: A C++23 compatible compiler
  - macOS: Clang 17+ (Xcode Command Line Tools or Xcode)
  - Linux: GCC 13+ or Clang 17+
  - Windows: MSVC 2022+ (Visual Studio 2022)
- **CMake**: Version 3.25 or higher
- **Git**: For cloning repositories
- **Python 3**: For running helper scripts

## Platform-Specific Dependencies

### macOS

1. **Install Homebrew** (if not already installed):
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

2. **Install system dependencies**:
   ```bash
   brew install cmake git python3 mpv ffmpeg webp mecab mecab-ipadic
   ```

3. **Install Xcode Command Line Tools** (for compiler):
   ```bash
   xcode-select --install
   ```

### Linux (Ubuntu/Debian)

1. **Update package lists**:
   ```bash
   sudo apt-get update
   ```

2. **Install build tools**:
   ```bash
   sudo apt-get install build-essential cmake git python3 python3-pip
   ```

3. **Install system dependencies**:
   ```bash
   sudo apt-get install libmpv-dev libavformat-dev libavcodec-dev \
       libavutil-dev libswscale-dev libswresample-dev libwebp-dev \
       mecab libmecab-dev mecab-ipadic-utf8
   ```

### Linux (Fedora/RHEL)

1. **Install build tools**:
   ```bash
   sudo dnf install gcc-c++ cmake git python3 python3-pip
   ```

2. **Install system dependencies**:
   ```bash
   sudo dnf install mpv-libs-devel ffmpeg-devel libwebp-devel mecab-devel
   ```

### Windows

1. **Install Visual Studio 2022**:
   - Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/)
   - During installation, select "Desktop development with C++"
   - Ensure C++23 support is selected

2. **Install vcpkg** (for dependency management):
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   cd C:\vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   ```

3. **Install dependencies via vcpkg**:
   ```powershell
   .\vcpkg install mpv:x64-windows ffmpeg:x64-windows libwebp:x64-windows
   ```

4. **Install Python 3**:
   - Download from [python.org](https://www.python.org/downloads/)
   - During installation, check "Add Python to PATH"

5. **Install MeCab**:
   - Download MeCab from [taku910.github.io/mecab](https://taku910.github.io/mecab/)
   - Install and note the installation directory
   - Download and install mecab-ipadic dictionary

## CTranslate2

CTranslate2 is required for local offline translation and must be built from source on all platforms.

### macOS

```bash
# Clone the repository
mkdir -p $HOME/lib
cd $HOME/lib
git clone --recursive https://github.com/OpenNMT/CTranslate2.git

# Build and install
cd CTranslate2
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/opt/homebrew \
      -DOPENMP_RUNTIME=NONE \
      -DWITH_MKL=OFF \
      -DWITH_ACCELERATE=ON \
      ..
make -j$(sysctl -n hw.ncpu)
sudo make install
```

### Linux

```bash
# Clone the repository
mkdir -p $HOME/lib
cd $HOME/lib
git clone --recursive https://github.com/OpenNMT/CTranslate2.git

# Build and install
cd CTranslate2
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DOPENMP_RUNTIME=NONE \
      -DWITH_MKL=OFF \
      ..
make -j$(nproc)
sudo make install
```

### Windows

```powershell
# Clone the repository
cd C:\
git clone --recursive https://github.com/OpenNMT/CTranslate2.git

# Build and install
cd CTranslate2
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX="C:\Program Files\CTranslate2" `
      -DOPENMP_RUNTIME=NONE `
      -DWITH_MKL=OFF `
      ..
cmake --build . --config Release
cmake --install . --config Release
```

Note: You may need to run the install command in an Administrator PowerShell.

## Translation Model

Download the Helsinki-NLP/opus-mt-ja-en model for offline translation:

```bash
# Install Python dependencies
pip3 install huggingface-hub transformers torch ctranslate2

# Run the download script (from project root)
python3 scripts/download_translation_model.py
```

This will download approximately 300MB and convert the model to CTranslate2 format in `assets/translation_model/`.

## Pitch Accent Database (Optional)

Generate the pitch accent database from CSV files:

```bash
python3 scripts/convert_pitch_accent.py
```

This creates `assets/pitch_accent.db`. If skipped, the application will work without pitch accent support.

## Anki Setup

1. **Install Anki**:
   - Download from [apps.ankiweb.net](https://apps.ankiweb.net/)

2. **Install AnkiConnect add-on**:
   - Open Anki
   - Go to Tools → Add-ons → Get Add-ons
   - Enter code: `2055492159`
   - Restart Anki

## Verification

After installation, verify that key dependencies are available:

**macOS/Linux:**
```bash
cmake --version        # Should be 3.25+
mpv --version         # Should show mpv version
ffmpeg -version       # Should show ffmpeg version
mecab -v              # Should show mecab version
python3 --version     # Should be 3.8+
```

**Windows:**
```powershell
cmake --version
python --version
```

## Troubleshooting

### macOS

- **"command not found" errors**: Ensure Homebrew's bin directory is in your PATH
  ```bash
  echo 'export PATH="/opt/homebrew/bin:$PATH"' >> ~/.zshrc
  source ~/.zshrc
  ```

### Linux

- **Missing library errors**: Run `sudo ldconfig` after installing libraries
- **MeCab dictionary not found**: Install `mecab-ipadic-utf8` package

### Windows

- **CMake can't find dependencies**: Add vcpkg toolchain file to CMake:
  ```powershell
  cmake -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake ..
  ```

- **Missing DLL errors**: Ensure vcpkg installed libraries are in your PATH

## Next Steps

Once all dependencies are installed, proceed to [Building](building.md) to compile the application.