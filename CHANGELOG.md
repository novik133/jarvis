# Changelog

All notable changes to J.A.R.V.I.S. Plasmoid will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.0] - 2026-03-26

### Added
- **Bundled llama-server** — Pre-built llama.cpp server (b8533) included, no separate installation required
- **Bundled whisper.cpp** — Built from source (v1.7.3) for wake word detection and transcription
- **LLM Manager** — New `JarvisLlmManager` class to control bundled llama-server (start/stop/restart)
- **Settings UI for Server Control** — Buttons in General settings to start/stop bundled LLM server
- **Model Download Integration** — Download LLM models (GGUF) directly from settings UI with progress tracking
- **Whisper Model Management** — Download and activate whisper models from settings
- **Piper TTS Download** — Download Piper TTS voices directly from settings UI
- **Self-Contained Architecture** — All AI components bundled, works out-of-the-box after installation
- **Arch Linux Package** — PKGBUILD for building native Arch packages with bundled dependencies

### Changed
- **Dependency Management** — Switched from system-installed llama.cpp/whisper.cpp to bundled via CMake FetchContent
- **Build System** — CMake now downloads and builds whisper.cpp from source, downloads pre-built llama-server
- **Settings Persistence** — Model paths and Piper binary locations now managed through settings
- **TTS Initialization** — Updated to use settings-managed Piper binary path with fallback to system paths
- **Whisper Model Loading** — Prioritizes settings-managed model paths over legacy hardcoded paths

### Technical
- CMake FetchContent for whisper.cpp (v1.7.3) with static linking
- CMake file(DOWNLOAD) for llama-server pre-built binary (b8533)
- New `llm/jarvisllmmanager.cpp` for bundled server lifecycle management
- QML bindings for server status, model download progress
- Compiler flags: `-Wno-error` for compatibility with GCC 15
- Position-independent code for whisper.cpp static libraries

## [0.1.1] - 2026-03-06

### Changed
- **LLM Streaming**: Switched from blocking request to SSE token streaming — response text appears in real-time as the LLM generates it
- **Sentence-based TTS Pipeline**: LLM response is split into sentences on-the-fly; first complete sentence is spoken immediately while the LLM continues generating
- **Piper TTS Refactor**: Sentence-queue architecture — each sentence is synthesized and played sequentially, no more single monolithic shell command per utterance
- **Sentence Splitting**: New `splitIntoSentences()` utility splits on `.!?;:` punctuation boundaries for natural TTS chunking
- **Stop Behavior**: `stop()` now clears the entire sentence queue instantly, improving responsiveness

### Added
- `streamingResponse` Q_PROPERTY — QML can bind to real-time token output during LLM generation
- `speakSentence()` method in TTS — allows external callers (streaming pipeline) to enqueue individual sentences
- `trySpeakCompleteSentences()` — monitors streamed text and dispatches complete sentences to TTS as they form
- `finalizeStreamingResponse()` — handles end-of-stream: speaks remaining text, executes ACTION blocks

### Optimized
- Compilation flags: `-march=native -O3` for native architecture optimization
- Whisper: already uses `WHISPER_SAMPLING_GREEDY` strategy, `audio_ctx=512`, 2 threads, amplitude threshold
- Removed unused `QMediaPlayer`/`QAudioOutput` from TTS (cleanup from 0.1.0)
- Cached Piper binary path — no filesystem scan per utterance

### Fixed
- TTS mute now properly stops all queued sentences, not just current playback

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

[0.2.0]: https://github.com/novik133/jarvis/releases/tag/v0.2.0
[0.1.1]: https://github.com/novik133/jarvis/releases/tag/v0.1.1
[0.1.0]: https://github.com/novik133/jarvis/releases/tag/v0.1.0
