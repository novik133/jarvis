#!/bin/bash
set -e

# J.A.R.V.I.S. Universal Installation Script
# Works on any Linux distribution (Debian, Ubuntu, Fedora, openSUSE, Arch, etc.)

echo "═══════════════════════════════════════════════"
echo "  J.A.R.V.I.S. Universal Installer v0.2.0"
echo "  Auto-builds llama.cpp and all dependencies"
echo "═══════════════════════════════════════════════"
echo

# Detect distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
    DISTRO_VERSION=$VERSION_ID
else
    echo "Cannot detect Linux distribution"
    exit 1
fi

echo "Detected distribution: $DISTRO $DISTRO_VERSION"

# Install dependencies based on distribution
install_dependencies() {
    echo "[1/5] Installing dependencies for $DISTRO..."
    
    case $DISTRO in
        ubuntu|debian|linuxmint)
            echo "  Using apt-get..."
            sudo apt-get update
            sudo apt-get install -y \
                build-essential cmake git \
                qt6-base-dev qt6-declarative-dev qt6-multimedia-dev qt6-speech-dev \
                extra-cmake-modules \
                libkf6declarative-dev libkf6i18n-dev libkf6config-dev libkf6coreaddons-dev \
                libkf6windowsystem-dev \
                plasma-workspace-dev \
                libssl-dev zlib1g-dev
            ;;
        
        fedora|centos|rhel)
            echo "  Using dnf..."
            sudo dnf install -y \
                gcc-c++ cmake git \
                qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtmultimedia-devel qt6-qtspeech-devel \
                extra-cmake-modules \
                kf6-kdeclarative-devel kf6-ki18n-devel kf6-kconfig-devel kf6-kcoreaddons-devel \
                kf6-kwindowsystem-devel \
                plasma-workspace-devel \
                openssl-devel zlib-devel
            ;;
        
        opensuse-leap|opensuse-tumbleweed)
            echo "  Using zypper..."
            sudo zypper install -y \
                gcc-c++ cmake git \
                qt6-base-devel qt6-declarative-devel qt6-multimedia-devel qt6-speech-devel \
                extra-cmake-modules \
                kf6-kdeclarative-devel kf6-ki18n-devel kf6-kconfig-devel kf6-kcoreaddons-devel \
                kf6-kwindowsystem-devel \
                plasma-workspace-devel \
                libopenssl-devel zlib-devel
            ;;
        
        arch|manjaro|endeavouros)
            echo "  Using pacman..."
            sudo pacman -S --needed \
                base-devel cmake git \
                qt6-base qt6-declarative qt6-multimedia qt6-speech \
                extra-cmake-modules \
                kdeclarative ki18n kconfig kcoreaddons \
                plasma-workspace \
                openssl zlib
            ;;
        
        *)
            echo "  WARNING: Unsupported distribution: $DISTRO"
            echo "  Attempting generic installation..."
            echo "  Please ensure you have these installed:"
            echo "  - CMake 3.20+, GCC 11+, Git"
            echo "  - Qt6 development packages"
            echo "  - KDE Frameworks 6 development packages"
            echo "  - OpenSSL and zlib development libraries"
            read -p "  Continue anyway? (y/N): " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                exit 1
            fi
            ;;
    esac
}

# Build and install from source
build_from_source() {
    echo "[2/5] Configuring with CMake (fetching bundled deps)..."
    echo "       This will automatically build llama.cpp and whisper.cpp..."
    
    # Determine install prefix
    PREFIX="${CMAKE_INSTALL_PREFIX:-/usr}"
    
    cmake -B build \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DCMAKE_BUILD_TYPE=Release
    
    echo
    echo "[3/5] Building (llama.cpp + whisper.cpp + plugin)..."
    echo "       This may take 10-30 minutes depending on your system..."
    cmake --build build -j"$(nproc)"
    
    echo
    echo "[4/5] Installing (requires sudo)..."
    sudo cmake --install build
    
    echo
    echo "[5/5] Updating icon cache..."
    sudo gtk-update-icon-cache /usr/share/icons/hicolor/ 2>/dev/null || true
    sudo update-icon-caches /usr/share/icons/hicolor/ 2>/dev/null || true
}

# Create desktop entry
create_desktop_entry() {
    echo "Creating desktop entry for easy configuration..."
    
    mkdir -p ~/.local/share/applications
    cat > ~/.local/share/applications/jarvis-config.desktop << EOF
[Desktop Entry]
Name=J.A.R.V.I.S. Configuration
Comment=Configure J.A.R.V.I.S. AI Assistant
Exec=plasmashell -a org.kde.plasma.jarvis
Icon=jarvis-ai
Type=Application
Categories=System;Settings;
EOF
}

# Main installation
main() {
    # Check if we're in the right directory
    if [ ! -f "CMakeLists.txt" ]; then
        echo "Error: Please run this script from the J.A.R.V.I.S. source directory"
        exit 1
    fi
    
    # Install dependencies
    install_dependencies
    
    # Build from source
    build_from_source
    
    # Create desktop entry
    create_desktop_entry
    
    echo
    echo "═══════════════════════════════════════════════"
    echo "  Installation complete!"
    echo ""
    echo "  ✓ llama.cpp built and bundled"
    echo "  ✓ whisper.cpp built and bundled"
    echo "  ✓ All dependencies installed"
    echo ""
    echo "  To get started:"
    echo "  1. Add J.A.R.V.I.S. widget to desktop/panel"
    echo "  2. Open settings to download models"
    echo "  3. Download an LLM model (e.g., Qwen 2.5 0.5B)"
    echo "  4. Download a Whisper model for voice input"
    echo "  5. Download Piper TTS for voice output"
    echo ""
    echo "  Restart plasmashell to load the plasmoid:"
    echo "    plasmashell --replace &"
    echo "═══════════════════════════════════════════════"
}

# Run main function
main "$@"
