# Changelog

All notable changes to J.A.R.V.I.S. Plasmoid will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-03-05

### Added
- Initial public release
- Local LLM integration via llama.cpp (OpenAI-compatible API)
- Wake word detection ("Jarvis") using whisper.cpp (tiny model, CPU-only)
- Voice command transcription with automatic language detection
- Piper TTS with downloadable voice models (espeak-ng fallback)
- Real-time system monitoring: CPU, RAM, temperature, uptime
- 14 built-in voice commands (open apps, lock screen, volume, screenshot)
- Custom voice command mappings with CRUD management
- Advanced system interaction via LLM — structured ACTION blocks:
  - `run_command` — execute shell commands
  - `open_terminal` — open terminal with a command
  - `write_file` — create/write files with content
  - `open_app` — launch GUI applications
  - `open_url` — open URLs in browser
  - `type_text` — type text into focused window via xdotool
- Reminder and timer system with quick presets
- Downloadable LLM models (GGUF) and TTS voices from HuggingFace
- Iron Man HUD-style UI with arc reactor animation, waveform visualizer
- KDE Plasma 6 configuration dialog with General, Voice Commands, and J.A.R.V.I.S. tabs
- 100% local and private — no cloud, no subscriptions, no data leaves your machine

### Technical
- C++23 backend with modular architecture (settings, TTS, audio, system, commands)
- QML frontend with Kirigami components
- CMake build system with ECM integration
- Qt 6 (Quick, Qml, Network, TextToSpeech, Multimedia, Concurrent)
- KF6::I18n for internationalization support

[0.1.0]: https://github.com/novik133/jarvis/releases/tag/v0.1.0
