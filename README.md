# Integrated Format Converter / 集成格式转换器

<div align="center">

**A Qt6-based multi-engine file format conversion desktop tool — unifies FFmpeg, Pandoc & ImageMagick under one GUI.**

**一个基于 Qt6 的多引擎文件格式转换桌面工具，集成 FFmpeg、Pandoc 和 ImageMagick，提供统一的图形界面。**

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/cpp/17)
[![Qt](https://img.shields.io/badge/Qt-6-green)](https://www.qt.io/)
[![License](https://img.shields.io/badge/license-MIT-yellow)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)]()

</div>

---

## 📖 Introduction / 简介

**Integrated Format Converter** is a desktop application that unifies three powerful open-source conversion engines under a single intuitive Qt6 GUI, providing batch processing, parallel execution, priority scheduling, and rich error recovery.

**集成格式转换器** 是一款桌面应用程序，将三种强大的开源转换引擎集成到统一的 Qt6 图形界面中，支持批量处理、并行执行、优先级调度以及完善的错误恢复机制。

| Engine / 引擎 | Domain / 领域 | Capability / 能力 |
|---|---|---|
| **FFmpeg** | Audio / Video / 音视频 | Transcode between 20+ media formats, extract audio, probe media info / 20+ 种媒体格式转换、音频提取、媒体信息探测 |
| **Pandoc** | Documents / 文档 | Convert between 13+ document formats (Markdown, LaTeX, Word, PDF, EPUB, etc.) / 13+ 种文档格式互转 |
| **ImageMagick** | Images / 图片 | Convert between 200+ image formats, resize, compress, strip metadata / 200+ 种图片格式转换、缩放、压缩、元数据清理 |

---

## ✨ Features / 功能特性

### 🔄 Conversion / 格式转换

- **Video transcoding / 视频转码** — MP4, AVI, FLV, MKV, WebM, MOV, WMV, MPEG; choose codec (H.264, H.265, VP9), resolution, bitrate, framerate / 可选编码器、分辨率、码率、帧率
- **Audio transcoding / 音频转码** — MP3, WAV, AAC, FLAC, OGG, M4A; configure sample rate, channels, bitrate / 可配置采样率、声道、码率
- **Audio extraction / 音频提取** — Pull audio track from any video file / 从视频文件中提取音轨
- **Image conversion / 图片转换** — Resize, compress, adjust quality, strip metadata, change format / 缩放、压缩、调整质量、清理元数据、格式转换
- **Document conversion / 文档转换** — Markdown, HTML, LaTeX, DOCX, PDF, EPUB, PPTX, ODT, RST, Org, CSV, JSON, plain text
- **Large file support / 大文件支持** — 5-level size categorization (Small → Huge); automatic segmented conversion for files > 100 MB using ffmpeg segment/mux / 五级文件大小分类，超 100MB 自动分段转换
- **Stream copy mode / 流复制模式** — Passthrough video/audio streams without re-encoding for compatible format pairs / 兼容格式对直接流复制，无需重新编码

### 📋 Task Management / 任务管理

- **Parallel execution / 并行执行** — Configurable concurrency (default 4) via `QThreadPool`; dynamically adjusts under memory pressure / 通过 QThreadPool 实现可配置并发（默认 4），内存压力下自动调整
- **Priority scheduling / 优先级调度** — Per-task priority (Low / Normal / High); higher-priority tasks jump the queue / 每任务优先级（低/中/高），高优先级任务插队执行
- **Batch processing / 批量处理** — Add multiple files, convert in one click; summary dialog with success/failure breakdown / 批量添加文件一键转换，完成后弹出成功/失败汇总
- **Pause / Resume / Cancel / 暂停/继续/取消** — Full lifecycle control per-task or globally / 单任务或全局全生命周期控制
- **Progress details / 进度详情** — Per-task: percentage, processing speed, bitrate, ETA, processed bytes (from ffmpeg stderr parsing) / 单任务百分比、处理速度、码率、预计剩余时间、已处理字节数
- **10 000+ task scalability / 万级任务可扩展性** — Tested for memory stability with large task sets / 经大任务集内存稳定性测试

### 🛡️ Error Handling & Recovery / 错误处理与恢复

- **Rich error taxonomy / 丰富错误分类** — 15 typed error codes across 4 categories / 15 种错误码，4 大分类:
  - *Parameter errors / 参数错误* — InvalidParameter, FileNotFound, PermissionDenied, DiskSpaceInsufficient
  - *Converter errors / 转换器错误* — ConverterNotFound, ConverterNotAvailable, UnsupportedFormat, ConversionFailed
  - *Task errors / 任务错误* — TaskCancelled, TaskTimeout, TaskDependencyFailed
  - *Process errors / 进程错误* — ProcessCrashed, OutOfMemory, ProcessFailedToStart
- **ErrorInfo struct / 错误信息结构体** — Full context: code, message, details, suggestion, timestamp, retry count, recoverable flag / 完整上下文：错误码、消息、详情、建议、时间戳、重试次数、可恢复标记
- **Auto-recovery / 自动恢复** — Optional automatic retry of recoverable errors / 可恢复错误可选自动重试
- **Retry Manager / 重试管理器** — Configurable max retries (default 3), exponential backoff delay (base 1 s, max 30 s, multiplier 2×), per-error-code retryability / 可配置最大重试次数（默认 3）、指数退避延迟（基数 1 秒、上限 30 秒、乘数 2 倍）
- **Dialog integration / 对话框集成** — Error dialog with retry action; error icon in status bar / 错误对话框支持重试，状态栏显示错误图标

### 🧩 Skill System / 技能系统

- **Built-in skills / 内置技能** — Pre-registered operations accessible from the UI / 预注册操作，UI 可直接调用
- **External skill execution / 外部技能执行** — Invoke arbitrary external processes with parameter forms auto-generated from schema / 调用任意外部进程，参数表单根据 schema 自动生成
- **Live progress & output / 实时进度与输出** — Real-time stdout/stderr capture, timeout handling / 实时 stdout/stderr 捕获，超时处理
- **Custom skill registration / 自定义技能注册** — Add your own skills at runtime / 运行时注册自定义技能

### 💾 Memory & Resource Management / 内存与资源管理

- **Memory Monitor / 内存监视器** — Periodic heap check (configurable interval), 3-level alert (Normal / Warning / Critical) / 定期堆检查（可配置间隔），三级警报（正常/警告/严重）
- **Pressure-aware scheduling / 压力感知调度** — TaskManager reduces parallelism when memory is under pressure / 内存不足时自动降低并行度
- **Tracked allocation / 分配追踪** — Records app-level allocation/deallocation for diagnostics / 记录应用级内存分配/释放，用于诊断

### 📝 Logging / 日志系统

- **Configurable level / 可配置级别** — Debug / Info / Warning / Error
- **Dual output / 双路输出** — Console + file with independent toggles / 控制台 + 文件，独立开关
- **Log rotation / 日志轮转** — Max file size (default 10 MB), configurable backup count / 最大文件大小（默认 10 MB），可配置备份数量
- **Module filter / 模块过滤** — Enable/disable logs per module (e.g., `Main`, `FFmpeg`, `TaskManager`) / 按模块开关日志（如 Main、FFmpeg、TaskManager）

### ⚙️ Configuration / 配置管理

- **JSON-based / 基于 JSON** — `~/.integrated_converter/config.json`
- **Auto-detect / 自动检测** — FFmpeg, Pandoc and ImageMagick paths resolved at startup via `PATH` lookup / 启动时通过 PATH 自动查找转换引擎路径
- **Persistence / 持久化** — Settings survive restarts; file is written on graceful shutdown / 设置跨重启持久保存，正常退出时写入文件

---

## 📊 Supported Formats / 支持格式

### 🎬 FFmpeg (Video / 视频)

| Format / 格式 | Codec Notes / 编码说明 |
|---|---|
| MP4 | H.264 / H.265 |
| AVI | — |
| FLV | — |
| MKV | Multi-track support / 多轨支持 |
| WebM | VP8 / VP9 |
| MOV | — |
| WMV | — |
| MPEG | MPEG-1 / MPEG-2 |

### 🎵 FFmpeg (Audio / 音频)

| Format / 格式 | Notes / 说明 |
|---|---|
| MP3 | — |
| WAV | Uncompressed / 未压缩 |
| AAC | Advanced Audio Coding / 高级音频编码 |
| FLAC | Lossless / 无损 |
| OGG | Vorbis |
| M4A | AAC in MP4 container / MP4 容器中的 AAC |

### 🖼️ ImageMagick (Image / 图片)

ImageMagick supports **200+ image formats** including but not limited to / 支持 **200+ 种图片格式**，包括但不限于:

| Format / 格式 | Input / 输入 | Output / 输出 |
|---|---|---|
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

> ¹ Requires ImageMagick built with HEIC/AVIF support / 需要 ImageMagick 编译时包含 HEIC/AVIF 支持。

Additional parameters / 额外参数: **resize / 缩放**, **quality / 质量** (1–100), **compression / 压缩** (e.g. JPEG2000, LZW), **density / DPI** (e.g. 300), **strip / 清理元数据**

### 📄 Pandoc (Document / 文档)

| Format / 格式 | Extension / 扩展名 | Input / 输入 | Output / 输出 |
|---|---|---|---|
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

> ¹ PDF output requires a LaTeX engine (xelatex/pdflatex) or wkhtmltopdf / PDF 输出需要 LaTeX 引擎或 wkhtmltopdf。

---

## 📸 Screenshots / 截图

> *Coming soon / 即将推出*

---

## 🔧 Requirements / 环境要求

| Component / 组件 | Minimum / 最低要求 |
|---|---|
| OS / 操作系统 | Windows 10 / 11 64-bit |
| RAM / 内存 | 4 GB (8 GB+ recommended / 建议) |
| Disk / 磁盘 | 500 MB free / 可用空间 |
| FFmpeg | 4.x+ (8.x recommended / 建议) |
| Pandoc | 2.x+ (3.x recommended / 建议) |
| ImageMagick | 7.x+ (recommended / 建议) |
| Qt (build / 编译) | 6.x (Core, Gui, Widgets, Concurrent) |
| Compiler / 编译器 (build / 编译) | GCC (MinGW) 8+ or MSVC 2019+ |
| CMake (build / 编译) | 3.16+ |

---

## 🚀 Quick Start / 快速开始

### 1. Install Dependencies / 安装依赖

**FFmpeg**

```bash
# Windows
winget install ffmpeg
# or download from / 或从 https://ffmpeg.org/download.html 下载
```

Ensure `ffmpeg` and `ffprobe` are on your `PATH` / 确保 `ffmpeg` 和 `ffprobe` 在 `PATH` 中。

**Pandoc**

```bash
# Windows
winget install pandoc
# or download from / 或从 https://pandoc.org/installing.html 下载
```

**ImageMagick**

```bash
# Windows
winget install imagemagick
# or download from / 或从 https://imagemagick.org/script/download.php 下载
```

> `magick` must be on your `PATH` / 请确保 `magick` 在 `PATH` 中。

### 2. Run / 运行

```bash
integrated_converter.exe
```

### 3. Convert / 开始转换

1. **Add files / 添加文件** — Click "Add Files" or drag & drop onto the file list / 点击"添加文件"或拖放到文件列表
2. **Select output / 选择输出** — Pick a target format in the config panel / 在配置面板选择目标格式
3. **Tune parameters / 调整参数** (optional / 可选) — Resolution, bitrate, codec, quality, PDF engine, etc. / 分辨率、码率、编码器、质量、PDF 引擎等
4. **Convert / 开始转换** — Click "Start Conversion" / 点击"开始转换"
5. **Monitor / 监视进度** — Watch real-time progress, speed, ETA / 观察实时进度、速度、预计剩余时间
6. **Review / 查看结果** — After batch completion, view the summary dialog (success/fail/retry) / 批量完成后查看汇总对话框（成功/失败/重试）

---

## 🏗️ Build / 编译构建

```bash
# Clone / 克隆仓库
git clone https://github.com/liuxuanbing10/integrated-converter.git
cd integrated-converter

# Configure / 配置
cmake -B build -G Ninja

# Build / 编译
cmake --build build -j$(nproc)

# Deploy Qt runtime DLLs (Windows / Windows 平台)
windeployqt build/integrated_converter.exe
```

> **MinGW note / MinGW 说明:** `WIN32_EXECUTABLE` is automatically disabled for MinGW builds to avoid `__imp___argc` link errors. / MinGW 编译时会自动禁用 `WIN32_EXECUTABLE` 以避免链接错误。

### Build Tests / 编译测试

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

The test suite covers / 测试套件覆盖:
- Unit tests for Logger, ConfigManager, TaskManager, ErrorHandler, FFmpegConverter, PandocConverter, ImageMagickConverter / Logger、ConfigManager、TaskManager、ErrorHandler、各转换器的单元测试
- Integration workflow (task lifecycle, batch, parallel, cancellation, priority, pause/resume) / 集成工作流（任务生命周期、批量处理、并行、取消、优先级、暂停/恢复）
- Performance benchmarks (task creation, removal, concurrent operations, log throughput) / 性能基准测试（任务创建、移除、并发操作、日志吞吐量）

---

## 📁 Project Structure / 项目结构

```
integrated_converter/
├── src/
│   ├── main.cpp                  # Entry point / 入口点
│   │
│   ├── core/                     # Core framework / 核心框架
│   │   ├── iconverter.h          # IConverter interface / 转换器接口
│   │   ├── conversion_task.h/cpp # Task model / 任务模型
│   │   ├── task_runnable.h/cpp   # QRunnable adapter / QRunnable 适配器
│   │   ├── task_manager.h/cpp    # Central scheduler / 中央调度器
│   │   ├── config_manager.h/cpp  # JSON config / JSON 配置管理
│   │   ├── format_registry.h/cpp # Format registry / 格式注册表
│   │   ├── logger.h/cpp          # Logging system / 日志系统
│   │   ├── error_types.h/cpp     # Error codes / 错误码定义
│   │   ├── error_handler.h/cpp   # Error handler / 错误处理
│   │   ├── retry_manager.h/cpp   # Retry scheduler / 重试调度
│   │   ├── skill_manager.h/cpp   # Skill system / 技能系统
│   │   ├── memory_monitor.h/cpp  # Memory monitor / 内存监控
│   │   └── large_file_handler.h/cpp # Large file handler / 大文件处理
│   │
│   ├── converters/               # Engine wrappers / 引擎包装器
│   │   ├── ffmpeg_converter.h/cpp    # FFmpeg subprocess / FFmpeg 子进程
│   │   ├── pandoc_converter.h/cpp    # Pandoc subprocess / Pandoc 子进程
│   │   ├── imagemagick_converter.h/cpp # ImageMagick subprocess / ImageMagick 子进程
│   │   └── segmented_converter.h/cpp # Segmented conversion / 分段转换
│   │
│   └── ui/                       # Qt Widgets frontend / Qt 界面
│       ├── main_window.h/cpp          # Main window / 主窗口
│       ├── file_list_widget.h/cpp     # File list / 文件列表
│       ├── config_panel.h/cpp         # Config panel / 配置面板
│       ├── task_list_widget.h/cpp     # Task table / 任务表格
│       ├── progress_widget.h/cpp      # Progress bar / 进度条
│       ├── batch_convert_dialog.h/cpp   # Batch dialog / 批量转换对话框
│       ├── batch_conversion_summary.h/cpp # Result summary / 结果汇总
│       ├── error_dialog.h/cpp         # Error dialog / 错误对话框
│       └── skill_invoke_dialog.h/cpp  # Skill dialog / 技能调用对话框
│
├── tests/                        # Test suite / 测试套件
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
├── CMakeLists.txt                # Top-level CMake / 顶层 CMake 构建
├── CMakePresets.json             # CMake presets / CMake 预设
├── .clang-format                 # Code style / 代码风格配置
├── .gitignore
└── README.md
```

---

## ⚙️ Configuration / 配置说明

File / 配置文件: `~/.integrated_converter/config.json`

| Key / 键 | Type / 类型 | Default / 默认值 | Description / 说明 |
|---|---|---|---|
| `maxParallelTasks` | int | 4 | Max concurrent conversions / 最大并行转换数 |
| `outputDirectory` | string | `$HOME` | Default output folder / 默认输出目录 |
| `logLevel` | int | 1 | 0=Debug, 1=Info, 2=Warning, 3=Error |
| `logFileSize` | int | 10485760 | Max bytes per log file (before rotation) / 日志文件最大字节数 |
| `ffmpegPath` | string | `ffmpeg` | FFmpeg executable path / FFmpeg 执行路径 |
| `pandocPath` | string | `pandoc` | Pandoc executable path / Pandoc 执行路径 |
| `imagemagickPath` | string | `magick` | ImageMagick executable path / ImageMagick 执行路径 |

---

## 🏛️ Architecture / 架构设计

### Design Patterns / 设计模式

| Pattern / 模式 | Usage / 用途 |
|---|---|
| **Singleton / 单例** | Logger, ConfigManager, TaskManager, ErrorHandler, RetryManager, MemoryMonitor |
| **Strategy / Interface / 策略接口** | `IConverter` abstract base; `FFmpegConverter`, `PandocConverter`, `ImageMagickConverter` implementations / 抽象基类及多个实现 |
| **Observer / 观察者** | Qt signals/slots for task progress, error events, memory alerts / Qt 信号槽用于进度、错误、内存告警 |
| **QRunnable** | `TaskRunnable` wraps `ConversionTask` for `QThreadPool` execution / 包装任务以供线程池执行 |

### Threading Model / 线程模型

```
┌──────────────────────────────────────────────────┐
│  Main Thread / 主线程 (Qt Event Loop / 事件循环)  │
│  - UI rendering & user interaction               │
│  - Signal/slot dispatch (QueuedConnection)        │
└──────────────────┬───────────────────────────────┘
                   │ addTask / cancel / pause / resume
                   ▼
┌──────────────────────────────────────────────────┐
│  TaskManager (main thread / 主线程)               │
│  - Queue management, priority insertion           │
│  - Memory-pressure-aware scheduling               │
└──────────────────┬───────────────────────────────┘
                   │ QThreadPool::start(runnable)
                   ▼
┌──────────────────────────────────────────────────┐
│  Worker Threads / 工作线程 (QThreadPool, up to N) │
│  - TaskRunnable::run() → IConverter::convert()    │
│  - QProcess execution (ffmpeg / pandoc / magick) │
│  - Emit progress signals (queued to main)         │
└──────────────────────────────────────────────────┘
```

### Key Signals / Slots / 关键信号与槽

- `TaskRunnable` → `TaskManager`: `started`, `progressChanged`, `finished`
- `TaskManager` → `MainWindow`: `taskAdded`, `taskStarted`, `taskCompleted`, `allTasksCompleted`
- `MemoryMonitor` → `TaskManager`: `memoryWarning`, `memoryCritical`, `memoryNormalized`
- `RetryManager` → `TaskManager`: `retryTriggered`

---

## ❓ FAQ / 常见问题

**Q: Conversion fails — what now? / 转换失败怎么办？**  
A: Check the error dialog for details. Common causes: missing FFmpeg/Pandoc/ImageMagick on `PATH`, insufficient disk space, unsupported format combination, or corrupted input file. / 查看错误对话框详情，常见原因：引擎未在 PATH 中、磁盘空间不足、格式组合不支持或输入文件损坏。

**Q: How to handle very large video files? / 如何处理超大视频文件？**  
A: The app automatically detects files > 100 MB and can apply segmented conversion (split → convert → merge). Configure segment size/count in code or use stream copy for compatible format pairs. / 应用自动检测超过 100 MB 的文件并支持分段转换（分割→转换→合并），可在代码中配置分段大小/数量。

**Q: How to improve conversion speed? / 如何提高转换速度？**  
A: Increase `maxParallelTasks` in config (watch memory usage), enable stream copy when format change is container-only, or choose a faster codec (H.264 > H.265 > VP9). / 增加配置文件中的 `maxParallelTasks`（注意内存使用），或启用流复制模式。

**Q: Where are logs stored? / 日志存储在哪里？**  
A: `~/.integrated_converter/converter.log` (rotated automatically / 自动轮转).

**Q: Can I define my own conversion presets? / 可以自定义转换预设吗？**  
A: Parameter maps are passed as `QVariantMap` to the converters. The UI exposes common presets; custom presets can be added by editing the `ConfigPanel` or by programmatic API. / 参数映射通过 `QVariantMap` 传递给转换器，UI 提供了常用预设，也可通过编辑 ConfigPanel 或编程接口添加。

---

## 📜 Changelog / 更新日志

### v1.0.1 (2025)
- Fix fullscreen: config panel text no longer obstructed / 修复全屏时配置面板文字遮挡问题
- Fix conversion freeze: mutex deadlock and ffmpeg async bug / 修复转换卡死（互斥锁死锁及 ffmpeg 异步 bug）
- Fix MinGW compilation: disable WIN32_EXECUTABLE for MinGW / 修复 MinGW 编译问题
- Add ImageMagick image conversion support / 新增 ImageMagick 图片转换支持
- Add format registry for unified format management / 新增格式注册表实现统一格式管理
- Update .gitignore for Qt deployment DLLs

### v1.0.0 (2024)
- Initial release / 初始发布
- FFmpeg video/audio transcoding with progress parsing / FFmpeg 音视频转码及进度解析
- Pandoc document conversion / Pandoc 文档转换
- Batch and parallel task execution / 批量及并行任务执行
- Error handling with typed error codes and auto-recovery / 类型化错误码及自动恢复的错误处理
- Retry manager with exponential backoff / 指数退避重试管理器
- Skill invocation system / 技能调用系统
- Memory monitoring and pressure-aware scheduling / 内存监控及压力感知调度
- Large file handling with segmented conversion / 大文件分段转换处理
- Comprehensive Qt Test suite (unit + integration + performance) / 全面的 Qt 测试套件（单元+集成+性能）

---

## 📄 License / 许可证

[MIT](LICENSE)

## 🙏 Acknowledgments / 致谢

- [Qt](https://www.qt.io/) — Cross-platform C++ GUI framework / 跨平台 C++ GUI 框架
- [FFmpeg](https://ffmpeg.org/) — Multimedia processing library / 多媒体处理库
- [Pandoc](https://pandoc.org/) — Universal document converter / 通用文档转换器
- [ImageMagick](https://imagemagick.org/) — Image processing suite / 图片处理套件
