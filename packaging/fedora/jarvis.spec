%global app_name    plasma6-jarvis
%global plugin_id   org.kde.plasma.jarvis

Name:           %{app_name}
Version:        0.1.1
Release:        1%{?dist}
Summary:        Local AI assistant for KDE Plasma 6 — J.A.R.V.I.S.
License:        GPL-3.0-or-later
URL:            https://github.com/novik133/jarvis
Source0:        %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.25
BuildRequires:  extra-cmake-modules >= 6.0
BuildRequires:  gcc-c++ >= 13
BuildRequires:  cmake(Qt6Quick) >= 6.0
BuildRequires:  cmake(Qt6Qml) >= 6.0
BuildRequires:  cmake(Qt6Network) >= 6.0
BuildRequires:  cmake(Qt6TextToSpeech) >= 6.0
BuildRequires:  cmake(Qt6Multimedia) >= 6.0
BuildRequires:  cmake(Qt6Concurrent) >= 6.0
BuildRequires:  kf6-plasma-devel
BuildRequires:  kf6-ki18n-devel
BuildRequires:  kf6-kconfig-devel
Requires:       kf6-plasma%{?_isa}
Requires:       qt6-qtbase%{?_isa}
Requires:       qt6-qtdeclarative%{?_isa}
Requires:       pipewire
Recommends:     piper
Recommends:     xdotool
Suggests:       llama-cpp

%description
A fully local AI assistant plasmoid for KDE Plasma 6, inspired by
Tony Stark's iconic J.A.R.V.I.S. (Just A Rather Very Intelligent System).

Features include local LLM chat via llama.cpp, wake word detection using
whisper.cpp, Piper text-to-speech with downloadable voices, real-time
system monitoring, voice commands, and advanced system interaction
(run commands, open apps, write files, type text).

Everything runs locally — no cloud, no API keys, no data leaves your machine.

%prep
%autosetup -n jarvis-%{version}

%build
%cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="%{optflags} -O3" \
    -DCMAKE_CXX_FLAGS="%{optflags} -O3"
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%doc README.md CHANGELOG.md
%dir %{_datadir}/plasma/plasmoids/%{plugin_id}
%{_datadir}/plasma/plasmoids/%{plugin_id}/*
%dir %{_libdir}/qt6/qml/org/kde/plasma/jarvis
%{_libdir}/qt6/qml/org/kde/plasma/jarvis/*
%{_datadir}/icons/hicolor/*/apps/jarvis-ai.*

%changelog
* Thu Mar 06 2026 Kamil 'Novik' Nowicki <kamil@kamilnowicki.com> - 0.1.1-1
- LLM token streaming via SSE — real-time response display
- Sentence-based TTS pipeline
- Piper TTS sentence-queue architecture
- Native compilation optimizations
- New streamingResponse Q_PROPERTY for QML

* Wed Mar 05 2026 Kamil 'Novik' Nowicki <kamil@kamilnowicki.com> - 0.1.0-1
- Initial release
