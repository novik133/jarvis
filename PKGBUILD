# Maintainer: Jarvis AI <jarvis@example.com>
pkgname=jarvis-plasmoid
pkgver=0.2.0
pkgrel=1
pkgdesc="J.A.R.V.I.S. AI Assistant KDE Plasmoid - Self-contained with bundled llama.cpp, whisper.cpp, and Piper TTS"
arch=('x86_64')
url="https://github.com/novik133/jarvis"
license=('GPL-3.0-or-later')
depends=(
    'kdeclarative'
    'ki18n'
    'kconfig'
    'kcoreaddons'
    'plasma-workspace'
    'qt6-base'
    'qt6-declarative'
    'qt6-multimedia'
    'qt6-speech'
    'qt6-5compat'
    'gcc-libs'
    'glibc'
)
makedepends=(
    'cmake'
    'extra-cmake-modules'
    'git'
    'qt6-tools'
    'ninja'
)
optdepends=(
    'pipewire-pulse: For audio input/output'
    'pulseaudio: Alternative audio backend'
    'xdotool: For typing text into windows'
)
provides=('jarvis-plasmoid')
conflicts=('jarvis-plasmoid-git')

# Building from local source - no source array needed

prepare() {
    # Create build directory
    mkdir -p build
}

build() {
    # We need to go up one level since makepkg runs in src/
    cd ..
    
    # Configure with optimized flags for Arch Linux
    cmake -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_C_FLAGS="-march=x86-64 -mtune=generic -O2 -pipe -fno-plt -fexceptions -Wp,-D_FORTIFY_SOURCE=2 -Wformat -Werror=format-security -fstack-clash-protection -fcf-protection" \
        -DCMAKE_CXX_FLAGS="-march=x86-64 -mtune=generic -O2 -pipe -fno-plt -fexceptions -Wp,-D_FORTIFY_SOURCE=2 -Wformat -Werror=format-security -fstack-clash-protection -fcf-protection" \
        -G Ninja
    
    # Build with all CPU cores
    ninja -C build
}

package() {
    # We need to go up one level since makepkg runs in src/
    cd ..
    
    # Install to package directory
    DESTDIR="$pkgdir" ninja -C build install
    
    # Install documentation
    install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md" 2>/dev/null || true
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE" 2>/dev/null || true
    
    # Create desktop file for easy access to settings
    mkdir -p "$pkgdir/usr/share/applications"
    cat > "$pkgdir/usr/share/applications/jarvis-settings.desktop" << 'EOF'
[Desktop Entry]
Name=J.A.R.V.I.S. Settings
Comment=Configure J.A.R.V.I.S. AI Assistant
Exec=kcmshell6 kcm_jarvis
Icon=jarvis-ai
Type=Application
Categories=Settings;System;
EOF
    
    # Install post-install message
    mkdir -p "$pkgdir/usr/share/doc/$pkgname"
    cat > "$pkgdir/usr/share/doc/$pkgname/POST_INSTALL" << 'EOM'
J.A.R.V.I.S. Plasmoid Installation Complete!

Quick Setup:
1. Add the J.A.R.V.I.S. widget to your desktop or panel
2. Open widget settings to configure components
3. Download an LLM model (recommended: Qwen 2.5 0.5B)
4. Start the bundled LLM server
5. Download a whisper model for voice commands
6. Download Piper TTS for high-quality speech

Models are stored in ~/.local/share/jarvis/
EOM
}

post_install() {
    echo ""
    echo "🤖 J.A.R.V.I.S. Plasmoid v$pkgver installed!"
    echo ""
    echo "📋 Quick Setup:"
    echo "   1. Add widget to desktop/panel"
    echo "   2. Open settings → Download LLM model"
    echo "   3. Start bundled LLM server"
    echo "   4. Download whisper model for voice"
    echo "   5. Download Piper TTS (optional)"
    echo ""
    echo "📖 Full documentation: /usr/share/doc/$pkgname/POST_INSTALL"
    echo ""
    
    # Update icon cache
    gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor 2>/dev/null || true
    update-icon-caches /usr/share/icons/hicolor 2>/dev/null || true
    
    # Restart plasmashell to load the plasmoid
    echo "💡 To load the plasmoid, run: plasmashell --replace &"
}

post_upgrade() {
    post_install
}

post_remove() {
    echo "🗑️  J.A.R.V.I.S. Plasmoid removed"
    echo "   Your models and data remain in: ~/.local/share/jarvis/"
    
    # Update icon cache
    gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor 2>/dev/null || true
    update-icon-caches /usr/share/icons/hicolor 2>/dev/null || true
}
