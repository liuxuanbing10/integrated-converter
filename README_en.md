# Integrated Format Converter

<div align="center">

**A Qt6-based multi-engine file format conversion desktop tool — unifies FFmpeg, Pandoc & ImageMagick under one GUI.**

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/cpp/17)
[![Qt](https://img.shields.io/badge/Qt-6-green)](https://www.qt.io/)
[![License](https://img.shields.io/badge/license-MIT-yellow)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)]()

</div>

---

## Introduction

**Integrated Format Converter** is a desktop application that unifies three powerful open-source conversion engines under a single intuitive Qt6 GUI, providing batch processing, parallel execution, priority scheduling, and rich error recovery.

| Engine | Domain | Capability |
|--------|--------|------------|
| **FFmpeg** | Audio / Video | Transcode between 20+ media formats, extract audio, probe media info |
| **Pandoc** | Documents | Convert between 13+ document formats (Markdown, LaTeX, Word, PDF, EPUB, etc.) |
| **ImageMagick** | Images | Convert between 200+ image formats, resize, compress, strip metadata |

---

## Features

### Format Conversion

- **Video transcoding** — MP4, AVI, FLV, MKV, WebM, MOV, WMV, MPEG; choose codec (H.264, H.265, VP9), resolution, bitrate, framerate
- **Audio transcoding** — MP3, WAV, AAC, FLAC, OGG, M4A; configure sample rate, channels, bitrate
- **Audio extraction** — Pull audio track from any video file
- **Image conversion** — Resize, compress, adjust quality, strip metadata, change format
- **Document conversion** — Markdown, HTML, LaTeX, DOCX, PDF, EPUB, PPTX, ODT, RST, Org, CSV, JSON, plain text
- **Large file support** — 5-level size categorization (Small → Huge); automatic segmented conversion for files > 100 MB using ffmpeg segment/mux
- **Stream copy mode** — Passthrough video/audio streams without re-encoding for compatible format pairs

### Task Management

- **Parallel execution** — Configurable concurrency (default 4) via `QThreadPool`; dynamically adjusts under memory pressure
- **Priority scheduling** — Per-task priority (Low / Normal / High); higher-priority tasks jump the queue
- **Batch processing** — Add multiple files, convert in one click; summary dialog with success/failure breakdown
- **Pause / Resume / Cancel** — Full lifecycle control per-task or globally
- **Progress details** — Per-task: percentage, processing speed, bitrate, ETA, processed bytes (from ffmpeg stderr parsing)
- **10 000+ task scalability** — Tested for memory stability with large task sets

### Error Handling & Recovery

- **Rich error taxonomy** — 15 typed error codes across 4 categories:
  - *Parameter errors* — InvalidParameter, FileNotFound, PermissionDenied, DiskSpaceInsufficient
  - *Converter errors* — ConverterNotFound, ConverterNotAvailable, UnsupportedFormat, ConversionFailed
  - *Task errors* — TaskCancelled, TaskTimeout, TaskDependencyFailed
  - *Process errors* — ProcessCrashed, OutOfMemory, ProcessFailedToStart
- **ErrorInfo struct** — Full context: code, message, details, suggestion, timestamp, retry count, recoverable flag
- **Auto-recovery** — Optional automatic retry of recoverable errors
- **Retry Manager** — Configurable max retries (default 3), exponential backoff delay (base 1 s, max 30 s, multiplier 2×), per-error-code retryability

### Skill System

- **Built-in skills** — Pre-registered operations accessible from the UI
- **External skill execution** — Invoke arbitrary external processes with parameter forms auto-generated from schema
- **Live progress & output** — Real-time stdout/stderr capture, timeout handling
- **Custom skill registration** — Add your own skills at runtime

### Memory & Resource Management

- **Memory Monitor** — Periodic heap check (configurable interval), 3-level alert (Normal / Warning / Critical)
- **Pressure-aware scheduling** — TaskManager reduces parallelism when memory is under pressure
- **Tracked allocation** — Records app-level allocation/deallocation for diagnostics

### Logging

- **Configurable level** — Debug / Info / Warning / Error
- **Dual output** — Console + file with independent toggles
- **Log rotation** — Max file size (default 10 MB), configurable backup count
- **Module filter** — Enable/disable logs per module (e.g., `Main`, `FFmpeg`, `TaskManager`)

### Configuration

- **JSON-based** — `~/.integrated_converter/config.json`
- **Auto-detect** — FFmpeg, Pandoc and ImageMagick paths resolved at startup via `PATH` lookup
- **Persistence** — Settings survive restarts; file is written on graceful shutdown

---

## Supported Formats

### FFmpeg (Video)

| Format | Codec Notes |
|--------|-------------|
| MP4 | H.264 / H.265 |
| AVI | — |
| FLV | — |
| MKV | Multi-track support |
| WebM | VP8 / VP9 |
| MOV | — |
| WMV | — |
| MPEG | MPEG-1 / MPEG-2 |

### FFmpeg (Audio)

| Format | Notes |
|--------|-------|
| MP3 | — |
| WAV | Uncompressed |
| AAC | Advanced Audio Coding |
| FLAC | Lossless |
| OGG | Vorbis |
| M4A | AAC in MP4 container |

### ImageMagick (Image)

ImageMagick supports **200+ image formats** including but not limited to:

| Format | Input | Output |
|--------|-------|--------|
| PNG | ✓ | ✓ |
| JPEG / JPG | ✓ | ✓ |
| GIF | ✓ | ✓ |
| BMP | ✓ | ✓ |
| TIFF | ✓ | ✓ |
| WebP | ✓ | ✓ |
| SVG | ✓ | ✓ |
| ICO | ✓ | ✓ |
| HEIC | ✓¹ | ✓¹ |
| AVIF | ✓¹ | ✓¹ |

> ¹ Requires ImageMagick built with HEIC/AVIF support.

Additional parameters: **resize**, **quality** (1–100), **compression** (e.g. JPEG2000, LZW), **density / DPI** (e.g. 300), **strip metadata**

### Pandoc (Document)

| Format | Extension | Input | Output |
|--------|-----------|-------|--------|
| Markdown | .md, .markdown | ✓ | ✓ |
| HTML | .html, .htm | ✓ | ✓ |
| LaTeX | .tex, .latex | ✓ | ✓ |
| Word (DOCX) | .docx | ✓ | ✓ |
| PDF | .pdf | ✓¹ | ✓¹ |
| reStructuredText | .rst | ✓ | ✓ |
| Org-mode | .org | ✓ | ✓ |
| EPUB | .epub | ✓ | ✓ |
| PowerPoint | .pptx | ✓ | ✓ |
| Plain Text | .txt | ✓ | ✓ |
| ODT | .odt | ✓ | ✓ |
| CSV | .csv | — | ✓ |
| JSON | .json | — | ✓ |

> ¹ PDF output requires a LaTeX engine (xelatex/pdflatex) or wkhtmltopdf.

---

## Requirements

| Component | Minimum |
|-----------|---------|
| OS | Windows 10 / 11 64-bit |
| RAM | 4 GB (8 GB+ recommended) |
| Disk | 500 MB free |
| FFmpeg | 4.x+ (8.x recommended) |
| Pandoc | 2.x+ (3.x recommended) |
| ImageMagick | 7.x+ (recommended) |
| Qt (build) | 6.x (Core, Gui, Widgets, Concurrent) |
| Compiler (build) | GCC (MinGW) 8+ or MSVC 2019+ |
| CMake (build) | 3.16+ |

---

## Quick Start

### 1. Install Dependencies

**FFmpeg**

```bash
# Windows
winget install ffmpeg
# or download from https://ffmpeg.org/download.html
```

Ensure `ffmpeg` and `ffprobe` are on your `PATH`.

**Pandoc**

```bash
# Windows
winget install pandoc
# or download from https://pandoc.org/installing.html
```

**ImageMagick**

```bash
# Windows
winget install imagemagick
# or download from https://imagemagick.org/script/download.php
```

> `magick` must be on your `PATH`.

### 2. Run

```bash
integrated_converter.exe
```

### 3. Convert

1. **Add files** — Click "Add Files" or drag & drop onto the file list
2. **Select output** — Pick a target format in the config panel
3. **Tune parameters** (optional) — Resolution, bitrate, codec, quality, PDF engine, etc.
4. **Convert** — Click "Start Conversion"
5. **Monitor** — Watch real-time progress, speed, ETA
6. **Review** — After batch completion, view the summary dialog (success/fail/retry)

---

## Build

```bash
# Clone
git clone https://github.com/liuxuanbing10/integrated-converter.git
cd integrated-converter

# Configure
cmake -B build -G Ninja

# Build
cmake --build build -j$(nproc)

# Deploy Qt runtime DLLs (Windows)
windeployqt build/integrated_converter.exe
```

> **MinGW note:** `WIN32_EXECUTABLE` is automatically disabled for MinGW builds to avoid `__imp___argc` link errors.

### Build Tests

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

The test suite covers:
- Unit tests for Logger, ConfigManager, TaskManager, ErrorHandler, FFmpegConverter, PandocConverter, ImageMagickConverter
- Integration workflow (task lifecycle, batch, parallel, cancellation, priority, pause/resume)
- Performance benchmarks (task creation, removal, concurrent operations, log throughput)

---

## Project Structure

```
integrated_converter/
├── src/
│   ├── main.cpp                  # Entry point
│   │
│   ├── core/                     # Core framework
│   │   ├── iconverter.h          # IConverter interface
│   │   ├── conversion_task.h/cpp # Task model
│   │   ├── task_runnable.h/cpp   # QRunnable adapter
│   │   ├── task_manager.h/cpp    # Central scheduler
│   │   ├── config_manager.h/cpp  # JSON config
│   │   ├── format_registry.h/cpp # Format registry
│   │   ├── logger.h/cpp          # Logging system
│   │   ├── error_types.h/cpp     # Error codes
│   │   ├── error_handler.h/cpp   # Error handler
│   │   ├── retry_manager.h/cpp   # Retry scheduler
│   │   ├── skill_manager.h/cpp   # Skill system
│   │   ├── memory_monitor.h/cpp  # Memory monitor
│   │   └── large_file_handler.h/cpp # Large file handler
│   │
│   ├── converters/               # Engine wrappers
│   │   ├── ffmpeg_converter.h/cpp    # FFmpeg subprocess
│   │   ├── pandoc_converter.h/cpp    # Pandoc subprocess
│   │   ├── imagemagick_converter.h/cpp # ImageMagick subprocess
│   │   └── segmented_converter.h/cpp # Segmented conversion
│   │
│   └── ui/                       # Qt Widgets frontend
│       ├── main_window.h/cpp          # Main window
│       ├── file_list_widget.h/cpp     # File list
│       ├── config_panel.h/cpp         # Config panel
│       ├── task_list_widget.h/cpp     # Task table
│       ├── progress_widget.h/cpp      # Progress bar
│       ├── batch_convert_dialog.h/cpp   # Batch dialog
│       ├── batch_conversion_summary.h/cpp # Result summary
│       ├── error_dialog.h/cpp         # Error dialog
│       └── skill_invoke_dialog.h/cpp  # Skill dialog
│
├── tests/                        # Test suite
│   ├── CMakeLists.txt
│   ├── test_main.cpp
│   ├── test_integration.cpp
│   ├── test_performance.cpp
│   ├── core/
│   │   ├── test_logger.cpp
│   │   ├── test_config_manager.cpp
│   │   ├── test_task_manager.cpp
│   │   └── test_error_handler.cpp
│   └── converters/
│       ├── test_ffmpeg_converter.cpp
│       ├── test_pandoc_converter.cpp
│       └── test_imagemagick_converter.cpp
│
├── CMakeLists.txt                # Top-level CMake
├── CMakePresets.json             # CMake presets
├── .clang-format                 # Code style
├── .gitignore
├── README.md
├── README_zh.md
└── README_en.md
```

---

## Configuration

File: `~/.integrated_converter/config.json`

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `maxParallelTasks` | int | 4 | Max concurrent conversions |
| `outputDirectory` | string | `$HOME` | Default output folder |
| `logLevel` | int | 1 | 0=Debug, 1=Info, 2=Warning, 3=Error |
| `logFileSize` | int | 10485760 | Max bytes per log file (before rotation) |
| `ffmpegPath` | string | `ffmpeg` | FFmpeg executable path |
| `pandocPath` | string | `pandoc` | Pandoc executable path |
| `imagemagickPath` | string | `magick` | ImageMagick executable path |

---

## Architecture

### Design Patterns

| Pattern | Usage |
|---------|-------|
| **Singleton** | Logger, ConfigManager, TaskManager, ErrorHandler, RetryManager, MemoryMonitor |
| **Strategy / Interface** | `IConverter` abstract base; `FFmpegConverter`, `PandocConverter`, `ImageMagickConverter` implementations |
| **Observer** | Qt signals/slots for task progress, error events, memory alerts |
| **QRunnable** | `TaskRunnable` wraps `ConversionTask` for `QThreadPool` execution |

### Threading Model

```
┌──────────────────────────────────────────────────┐
│  Main Thread (Qt Event Loop)                      │
│  - UI rendering & user interaction                │
│  - Signal/slot dispatch (QueuedConnection)        │
└──────────────────┬───────────────────────────────┘
                   │ addTask / cancel / pause / resume
                   ▼
┌──────────────────────────────────────────────────┐
│  TaskManager (main thread)                        │
│  - Queue management, priority insertion           │
│  - Memory-pressure-aware scheduling               │
└──────────────────┬───────────────────────────────┘
                   │ QThreadPool::start(runnable)
                   ▼
┌──────────────────────────────────────────────────┐
│  Worker Threads (QThreadPool, up to N)            │
│  - TaskRunnable::run() → IConverter::convert()   │
│  - QProcess execution (ffmpeg / pandoc / magick) │
│  - Emit progress signals (queued to main)         │
└──────────────────────────────────────────────────┘
```

---

## FAQ

**Q: Conversion fails — what now?**  
A: Check the error dialog for details. Common causes: missing FFmpeg/Pandoc/ImageMagick on `PATH`, insufficient disk space, unsupported format combination, or corrupted input file.

**Q: How to handle very large video files?**  
A: The app automatically detects files > 100 MB and can apply segmented conversion (split → convert → merge). Configure segment size/count in code or use stream copy for compatible format pairs.

**Q: How to improve conversion speed?**  
A: Increase `maxParallelTasks` in config (watch memory usage), enable stream copy when format change is container-only, or choose a faster codec (H.264 > H.265 > VP9).

**Q: Where are logs stored?**  
A: `~/.integrated_converter/converter.log` (rotated automatically).

---

## Changelog

### v1.0.1 (2025)
- Fix fullscreen: config panel text no longer obstructed
- Fix conversion freeze: mutex deadlock and ffmpeg async bug
- Fix MinGW compilation: disable WIN32_EXECUTABLE for MinGW
- Add ImageMagick image conversion support
- Add format registry for unified format management
- Update .gitignore for Qt deployment DLLs

### v1.0.0 (2024)
- Initial release
- FFmpeg video/audio transcoding with progress parsing
- Pandoc document conversion
- Batch and parallel task execution
- Error handling with typed error codes and auto-recovery
- Retry manager with exponential backoff
- Skill invocation system
- Memory monitoring and pressure-aware scheduling
- Large file handling with segmented conversion
- Comprehensive Qt Test suite (unit + integration + performance)

---

## License

[MIT](LICENSE)

## Acknowledgments

- [Qt](https://www.qt.io/) — Cross-platform C++ GUI framework
- [FFmpeg](https://ffmpeg.org/) — Multimedia processing library
- [Pandoc](https://pandoc.org/) — Universal document converter
- [ImageMagick](https://imagemagick.org/) — Image processing suite
