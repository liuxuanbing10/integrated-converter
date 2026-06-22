# 集成格式转换器

<div align="center">

**一个基于 Qt6 的多引擎文件格式转换桌面工具，集成 FFmpeg、Pandoc 和 ImageMagick，提供统一的图形界面。**

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)](https://en.cppreference.com/w/cpp/23)
[![Qt](https://img.shields.io/badge/Qt-6-green)](https://www.qt.io/)
[![License](https://img.shields.io/badge/license-MIT-yellow)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)]()

</div>

---

## 简介

**集成格式转换器**（Integrated Format Converter）是一款桌面应用程序，将三种强大的开源转换引擎集成到统一的 Qt6 图形界面中，支持批量处理、并行执行、优先级调度以及完善的错误恢复机制。

| 引擎 | 领域 | 能力 |
|------|------|------|
| **FFmpeg** | 音视频 | 20+ 种媒体格式转换、音频提取、媒体信息探测 |
| **Pandoc** | 文档 | 13+ 种文档格式互转（Markdown、LaTeX、Word、PDF、EPUB 等） |
| **ImageMagick** | 图片 | 200+ 种图片格式转换、缩放、压缩、元数据清理 |

---

## 功能特性

### 格式转换

- **视频转码** — MP4、AVI、FLV、MKV、WebM、MOV、WMV、MPEG；可选编码器（H.264、H.265、VP9）、分辨率、码率、帧率
- **音频转码** — MP3、WAV、AAC、FLAC、OGG、M4A；可配置采样率、声道、码率
- **音频提取** — 从视频文件中提取音轨
- **图片转换** — 缩放、压缩、调整质量、清理元数据、格式转换
- **文档转换** — Markdown、HTML、LaTeX、DOCX、PDF、EPUB、PPTX、ODT、RST、Org、CSV、JSON、纯文本
- **大文件支持** — 五级文件大小分类（小→超大），超过 100 MB 自动分段转换
- **流复制模式** — 兼容格式对直接流复制，无需重新编码

### 任务管理

- **并行执行** — 通过 QThreadPool 实现可配置并发（默认 4），内存压力下自动调整
- **优先级调度** — 每任务优先级（低/中/高），高优先级任务插队执行
- **批量处理** — 批量添加文件一键转换，完成后弹出成功/失败汇总
- **暂停/继续/取消** — 单任务或全局全生命周期控制
- **进度详情** — 单任务百分比、处理速度、码率、预计剩余时间、已处理字节数
- **万级任务可扩展性** — 经大任务集内存稳定性测试

### 错误处理与恢复

- **丰富错误分类** — 15 种错误码，4 大分类：
  - *参数错误* — InvalidParameter、FileNotFound、PermissionDenied、DiskSpaceInsufficient
  - *转换器错误* — ConverterNotFound、ConverterNotAvailable、UnsupportedFormat、ConversionFailed
  - *任务错误* — TaskCancelled、TaskTimeout、TaskDependencyFailed
  - *进程错误* — ProcessCrashed、OutOfMemory、ProcessFailedToStart
- **错误信息结构体** — 完整上下文：错误码、消息、详情、建议、时间戳、重试次数、可恢复标记
- **自动恢复** — 可恢复错误可选自动重试
- **重试管理器** — 可配置最大重试次数（默认 3）、指数退避延迟（基数 1 秒、上限 30 秒、乘数 2 倍）

### 技能系统

- **内置技能** — 预注册操作，UI 可直接调用
- **外部技能执行** — 调用任意外部进程，参数表单根据 schema 自动生成
- **实时进度与输出** — 实时 stdout/stderr 捕获，超时处理
- **自定义技能注册** — 运行时注册自定义技能

### 内存与资源管理

- **内存监视器** — 定期堆检查（可配置间隔），三级警报（正常/警告/严重）
- **压力感知调度** — 内存不足时自动降低并行度
- **分配追踪** — 记录应用级内存分配/释放，用于诊断

### 日志系统

- **可配置级别** — Debug / Info / Warning / Error
- **双路输出** — 控制台 + 文件，独立开关
- **日志轮转** — 最大文件大小（默认 10 MB），可配置备份数量
- **模块过滤** — 按模块开关日志（如 Main、FFmpeg、TaskManager）

### 配置管理

- **基于 JSON** — `~/.integrated_converter/config.json`
- **自动检测** — 启动时通过 PATH 自动查找转换引擎路径
- **持久化** — 设置跨重启持久保存，正常退出时写入文件

---

## 支持格式

### FFmpeg（视频）

| 格式 | 编码说明 |
|------|----------|
| MP4 | H.264 / H.265 |
| AVI | — |
| FLV | — |
| MKV | 多轨支持 |
| WebM | VP8 / VP9 |
| MOV | — |
| WMV | — |
| MPEG | MPEG-1 / MPEG-2 |

### FFmpeg（音频）

| 格式 | 说明 |
|------|------|
| MP3 | — |
| WAV | 未压缩 |
| AAC | 高级音频编码 |
| FLAC | 无损 |
| OGG | Vorbis |
| M4A | MP4 容器中的 AAC |

### ImageMagick（图片）

ImageMagick 支持 **200+ 种图片格式**，包括但不限于：

| 格式 | 输入 | 输出 |
|------|------|------|
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

> ¹ 需要 ImageMagick 编译时包含 HEIC/AVIF 支持。

额外参数：**缩放**、**质量**（1–100）、**压缩**（如 JPEG2000、LZW）、**DPI**（如 300）、**清理元数据**

### Pandoc（文档）

| 格式 | 扩展名 | 输入 | 输出 |
|------|--------|------|------|
| Markdown | .md, .markdown | ✓ | ✓ |
| HTML | .html, .htm | ✓ | ✓ |
| LaTeX | .tex, .latex | ✓ | ✓ |
| Word (DOCX) | .docx | ✓ | ✓ |
| PDF | .pdf | ✓¹ | ✓¹ |
| reStructuredText | .rst | ✓ | ✓ |
| Org-mode | .org | ✓ | ✓ |
| EPUB | .epub | ✓ | ✓ |
| PowerPoint | .pptx | ✓ | ✓ |
| 纯文本 | .txt | ✓ | ✓ |
| ODT | .odt | ✓ | ✓ |
| CSV | .csv | — | ✓ |
| JSON | .json | — | ✓ |

> ¹ PDF 输出需要 LaTeX 引擎（xelatex/pdflatex）或 wkhtmltopdf。

---

## 环境要求

| 组件 | 最低要求 |
|------|----------|
| 操作系统 | Windows 10 / 11 64-bit |
| 内存 | 4 GB（建议 8 GB+） |
| 磁盘 | 500 MB 可用空间 |
| FFmpeg | 4.x+（建议 8.x） |
| Pandoc | 2.x+（建议 3.x） |
| ImageMagick | 7.x+（建议） |
| Qt（编译） | 6.x（Core, Gui, Widgets, Concurrent） |
| 编译器（编译） | GCC (MinGW) 8+ 或 MSVC 2019+ |
| CMake（编译） | 3.16+ |

---

## 快速开始

### 1. 安装依赖

**FFmpeg**

```bash
# Windows
winget install ffmpeg
# 或从 https://ffmpeg.org/download.html 下载
```

请确保 `ffmpeg` 和 `ffprobe` 在 `PATH` 中。

**Pandoc**

```bash
# Windows
winget install pandoc
# 或从 https://pandoc.org/installing.html 下载
```

**ImageMagick**

```bash
# Windows
winget install imagemagick
# 或从 https://imagemagick.org/script/download.php 下载
```

> 请确保 `magick` 在 `PATH` 中。

### 2. 运行

```bash
integrated_converter.exe
```

### 3. 开始转换

1. **添加文件** — 点击"添加文件"或拖放到文件列表
2. **选择输出** — 在配置面板选择目标格式
3. **调整参数**（可选）— 分辨率、码率、编码器、质量、PDF 引擎等
4. **开始转换** — 点击"开始转换"
5. **监视进度** — 观察实时进度、速度、预计剩余时间
6. **查看结果** — 批量完成后查看汇总对话框（成功/失败/重试）

---

## 编译构建

```bash
# 克隆仓库
git clone https://github.com/liuxuanbing10/integrated-converter.git
cd integrated-converter

# 配置
cmake -B build -G Ninja

# 编译
cmake --build build -j$(nproc)

# 部署 Qt 运行时 DLL（Windows）
windeployqt build/integrated_converter.exe
```

> **MinGW 说明：** MinGW 编译时会自动禁用 `WIN32_EXECUTABLE` 以避免链接错误。

### 编译测试

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

测试套件覆盖：
- Logger、ConfigManager、TaskManager、ErrorHandler、各转换器的单元测试
- 集成工作流（任务生命周期、批量处理、并行、取消、优先级、暂停/恢复）
- 性能基准测试（任务创建、移除、并发操作、日志吞吐量）

---

## 项目结构

```
integrated_converter/
├── src/
│   ├── main.cpp                  # 入口点
│   │
│   ├── core/                     # 核心框架
│   │   ├── iconverter.h          # 转换器接口
│   │   ├── conversion_task.h/cpp # 任务模型
│   │   ├── task_runnable.h/cpp   # QRunnable 适配器
│   │   ├── task_manager.h/cpp    # 中央调度器
│   │   ├── config_manager.h/cpp  # JSON 配置管理
│   │   ├── format_registry.h/cpp # 格式注册表
│   │   ├── logger.h/cpp          # 日志系统
│   │   ├── error_types.h/cpp     # 错误码定义
│   │   ├── error_handler.h/cpp   # 错误处理
│   │   ├── retry_manager.h/cpp   # 重试调度
│   │   ├── skill_manager.h/cpp   # 技能系统
│   │   ├── memory_monitor.h/cpp  # 内存监控
│   │   └── large_file_handler.h/cpp # 大文件处理
│   │
│   ├── converters/               # 引擎包装器
│   │   ├── ffmpeg_converter.h/cpp    # FFmpeg 子进程
│   │   ├── pandoc_converter.h/cpp    # Pandoc 子进程
│   │   ├── imagemagick_converter.h/cpp # ImageMagick 子进程
│   │   └── segmented_converter.h/cpp # 分段转换
│   │
│   └── ui/                       # Qt 界面
│       ├── main_window.h/cpp          # 主窗口
│       ├── file_list_widget.h/cpp     # 文件列表
│       ├── conversion_params_dialog.h/cpp  # 转换参数对话框
│       ├── task_list_widget.h/cpp     # 任务表格
│       ├── progress_widget.h/cpp      # 进度条
│       ├── batch_convert_dialog.h/cpp   # 批量转换对话框
│       ├── batch_conversion_summary.h/cpp # 结果汇总
│       ├── error_dialog.h/cpp         # 错误对话框
│       └── skill_invoke_dialog.h/cpp  # 技能调用对话框
│
├── tests/                        # 测试套件
│   ├── CMakeLists.txt
│   ├── test_main.cpp
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
├── CMakeLists.txt                # 顶层 CMake 构建
├── CMakePresets.json             # CMake 预设
├── .clang-format                 # 代码风格配置
├── .gitignore
├── README.md
├── README_zh.md
└── README_en.md
```

---

## 配置说明

配置文件：`~/.integrated_converter/config.json`

| 键 | 类型 | 默认值 | 说明 |
|-----|------|---------|------|
| `maxParallelTasks` | int | 4 | 最大并行转换数 |
| `outputDirectory` | string | `$HOME` | 默认输出目录 |
| `logLevel` | int | 1 | 0=Debug, 1=Info, 2=Warning, 3=Error |
| `logFileSize` | int | 10485760 | 日志文件最大字节数 |
| `ffmpegPath` | string | `ffmpeg` | FFmpeg 执行路径 |
| `pandocPath` | string | `pandoc` | Pandoc 执行路径 |
| `imagemagickPath` | string | `magick` | ImageMagick 执行路径 |

---

## 架构设计

### 设计模式

| 模式 | 用途 |
|------|------|
| **单例** | Logger、ConfigManager、TaskManager、ErrorHandler、RetryManager、MemoryMonitor |
| **策略/接口** | `IConverter` 抽象基类，FFmpegConverter、PandocConverter、ImageMagickConverter 实现 |
| **观察者** | Qt 信号槽用于进度、错误、内存告警 |
| **QRunnable** | TaskRunnable 包装 ConversionTask 以供 QThreadPool 执行 |

### 线程模型

```
┌──────────────────────────────────────────────────┐
│  主线程（Qt 事件循环）                             │
│  - UI 渲染与用户交互                               │
│  - 信号/槽分发（QueuedConnection）                  │
└──────────────────┬───────────────────────────────┘
                   │ addTask / cancel / pause / resume
                   ▼
┌──────────────────────────────────────────────────┐
│  TaskManager（主线程）                             │
│  - 队列管理、优先级插入                             │
│  - 内存压力感知调度                                 │
└──────────────────┬───────────────────────────────┘
                   │ QThreadPool::start(runnable)
                   ▼
┌──────────────────────────────────────────────────┐
│  工作线程（QThreadPool，最多 N 个）                  │
│  - TaskRunnable::run() → IConverter::convert()    │
│  - QProcess 执行（ffmpeg / pandoc / magick）       │
│  - 发射进度信号（排队到主线程）                      │
└──────────────────────────────────────────────────┘
```

---

## 常见问题

**问：转换失败怎么办？**  
答：查看错误对话框详情。常见原因：引擎未在 PATH 中、磁盘空间不足、格式组合不支持或输入文件损坏。

**问：如何处理超大视频文件？**  
答：应用自动检测超过 100 MB 的文件并支持分段转换（分割→转换→合并），可在代码中配置分段大小/数量或对兼容格式对使用流复制模式。

**问：如何提高转换速度？**  
答：增加配置文件中的 `maxParallelTasks`（注意内存使用），或对仅容器格式变化时启用流复制模式，或选择更快的编码器（H.264 > H.265 > VP9）。

**问：日志存储在哪里？**  
答：`~/.integrated_converter/converter.log`（自动轮转）。

---

## 更新日志

### v1.4.0 (2026-06-22)
- 精简代码库：移除 ~1100 行冗余代码
- 移除模块：SkillManager、SkillInvokeDialog、MemoryMonitor、RetryManager、ErrorHandler、ILogger（全部未使用）
- 简化 TaskManager：移除暂停/恢复/内存压力相关方法、信号、槽和成员
- 简化 LargeFileHandler：移除 7 个仅测试用的方法
- 简化 ErrorTypes：移除未使用的工厂函数和枚举值
- 简化 FormatRegistry：QSet → QStringList::contains()
- 简化 ConfigManager：移除未使用的 configChanged 信号
- 简化 ConversionTask：移除速度/比特率/ETA 追踪
- Logger：Singleton 模式重构为 ILogger 接口
- 修复测试：TestSegmentedConverter、TestLargeFileHandler 断言阈值修正（全部 11 套件通过）
- UI 美化：12 个 Material Design SVG 图标、Fusion 样式

### v1.3.1 (2026-06-05)
- 修复"转换参数设置"对话框内下拉框的阴影/半透明外观,统一为与主窗口"输出格式"相同的纯白底(2px 蓝色边框、5px 圆角)
- 同步更新版本号:CMake project VERSION、QApplication 版本、窗口标题、"关于"对话框

### v1.0.1 (2025)
- 修复全屏时配置面板文字遮挡问题
- 修复转换卡死（互斥锁死锁及 ffmpeg 异步 bug）
- 修复 MinGW 编译问题
- 新增 ImageMagick 图片转换支持
- 新增格式注册表实现统一格式管理
- 更新 .gitignore 以排除 Qt 部署 DLL

### v1.0.0 (2024)
- 初始发布
- FFmpeg 音视频转码及进度解析
- Pandoc 文档转换
- 批量及并行任务执行
- 类型化错误码及自动恢复的错误处理
- 指数退避重试管理器
- 技能调用系统
- 内存监控及压力感知调度
- 大文件分段转换处理
- 全面的 Qt 测试套件（单元+集成+性能）

---

## 许可证

[MIT](LICENSE)

## 致谢

- [Qt](https://www.qt.io/) — 跨平台 C++ GUI 框架
- [FFmpeg](https://ffmpeg.org/) — 多媒体处理库
- [Pandoc](https://pandoc.org/) — 通用文档转换器
- [ImageMagick](https://imagemagick.org/) — 图片处理套件
