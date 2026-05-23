# 集成格式转换工具

一个基于Qt6的多功能文件格式转换工具，集成FFmpeg和Pandoc实现视频、音频和文档格式转换。

## 功能特性

### 核心功能
- **文档格式转换**：支持Markdown、HTML、LaTeX、Word、PDF、reStructuredText、Org-mode、EPUB、PowerPoint等格式的相互转换
- **音视频格式转换**：支持MP4、AVI、FLV、MKV、WebM、MOV、WMV、MPEG等视频格式，以及MP3、WAV、AAC、FLAC、OGG、M4A等音频格式
- **批量转换**：支持多文件批量转换，提高工作效率
- **并行多任务**：支持最多4个并行转换任务，充分利用多核CPU资源
- **参数配置**：提供丰富的转换参数配置，包括视频分辨率、比特率、编码器、文档样式等
- **进度显示**：实时显示转换进度、预估剩余时间、处理速度等详细信息
- **错误处理**：完善的错误处理机制，提供友好的错误提示和恢复建议
- **日志记录**：详细的日志记录，支持日志文件轮转，便于问题排查
- **Skill集成**：支持从界面调用各种Skill功能

### 用户界面
- 直观的图形用户界面
- 支持拖放文件操作
- 任务列表实时更新
- 批量转换结果汇总
- 错误对话框和重试功能

## 支持的格式

### FFmpeg（视频/音频）

#### 视频格式
| 格式 | 说明 | 备注 |
|------|------|------|
| MP4 | 最常用的视频格式 | H.264/H.265编码 |
| AVI | 传统Windows视频格式 | |
| FLV | Flash视频格式 | |
| MKV | Matroska多媒体容器 | 支持多音轨 |
| WebM | Web视频格式 | VP8/VP9编码 |
| MOV | QuickTime格式 | |
| WMV | Windows Media Video | |
| MPEG | MPEG-1/2视频 | |

#### 音频格式
| 格式 | 说明 | 备注 |
|------|------|------|
| MP3 | 最常用的音频格式 | |
| WAV | 无压缩音频 | |
| AAC | 高级音频编码 | |
| FLAC | 无损音频编码 | |
| OGG | Vorbis音频 | |
| M4A | AAC音频文件 | |

### Pandoc（文档）

| 输入格式 | 输出格式 | 扩展名 |
|---------|---------|--------|
| Markdown | ✓ | .md, .markdown |
| HTML | ✓ | .html, .htm |
| LaTeX | ✓ | .tex, .latex |
| Word | ✓ | .docx |
| PDF | ✓ | .pdf |
| reStructuredText | ✓ | .rst |
| Org-mode | ✓ | .org |
| EPUB | ✓ | .epub |
| PowerPoint | ✓ | .pptx |
| Plain Text | ✓ | .txt |
| ODT | ✓ | .odt |
| CSV | ✓ | .csv |
| JSON | ✓ | .json |

## 系统要求

- **操作系统**：Windows 11 64位（或Windows 10 64位）
- **内存**：至少4GB RAM（推荐8GB+）
- **磁盘空间**：至少500MB可用空间
- **依赖软件**：
  - FFmpeg 8.x+（用于音视频转换）
  - Pandoc 3.x+（用于文档转换）

## 快速开始

### 1. 安装依赖

#### FFmpeg
下载并安装FFmpeg：
```bash
# 使用winget安装
winget install ffmpeg

# 或从官网下载：https://ffmpeg.org/download.html
```

确保将FFmpeg添加到系统PATH环境变量。

#### Pandoc
下载并安装Pandoc：
```bash
# 使用winget安装
winget install pandoc

# 或从官网下载：https://pandoc.org/installing.html
```

### 2. 运行程序

直接运行 `integrated_converter.exe` 即可启动程序。

### 3. 基本使用

#### 文档转换
1. 点击"添加文件"或拖放文件到文件列表
2. 在参数配置面板选择输出格式（如Word）
3. 可选：配置转换参数（如PDF引擎、目录生成等）
4. 点击"开始转换"按钮
5. 等待转换完成，查看结果

#### 音视频转换
1. 点击"添加文件"或拖放视频/音频文件
2. 在参数配置面板选择输出格式（如MP4）
3. 可选：配置视频/音频参数（分辨率、比特率等）
4. 点击"开始转换"按钮
5. 等待转换完成，查看结果

#### 批量转换
1. 添加多个文件到文件列表
2. 选择统一的输出格式和参数
3. 点击"批量转换"按钮
4. 查看批量转换进度和结果汇总

### 4. 参数配置

#### 视频参数
- **分辨率**：可选择预设（1080p、720p、480p等）或自定义
- **比特率**：视频比特率，影响视频质量
- **帧率**：视频帧率（24、30、60fps等）
- **编码器**：H.264、H.265、VP9等

#### 音频参数
- **采样率**：音频采样率（44100、48000Hz等）
- **声道**：单声道、立体声
- **比特率**：音频比特率（128k、192k、320k等）

#### 文档参数
- **PDF引擎**：xelatex、pdflatex、wkhtmltopdf等
- **目录**：是否生成目录及目录深度
- **模板**：自定义文档模板

## 构建项目

### 环境要求
- CMake 3.16+
- Qt6（Core, Gui, Widgets, Concurrent模块）
- C++17编译器（MinGW或MSVC）
- Ninja或Make构建工具

### 构建步骤

```bash
# 克隆项目
cd integrated_converter

# 创建构建目录
mkdir build
cd build

# 配置项目
cmake .. -G "MinGW Makefiles"

# 或使用Ninja
cmake .. -G Ninja

# 编译
cmake --build . -j4
```

### Qt部署

Windows平台使用Qt部署工具：
```bash
# 复制所有Qt依赖到构建目录
windeployqt integrated_converter.exe
```

## 配置说明

配置文件位于 `~/.integrated_converter/config.json`，支持以下配置项：

| 配置项 | 说明 | 默认值 |
|--------|------|--------|
| maxParallelTasks | 最大并行任务数 | 4 |
| outputDirectory | 默认输出目录 | 用户主目录 |
| logLevel | 日志级别(0-3) | 1 (Info) |
| logFileSize | 单个日志文件最大大小 | 10MB |
| ffmpegPath | FFmpeg可执行文件路径 | ffmpeg |
| pandocPath | Pandoc可执行文件路径 | pandoc |

## 项目结构

```
integrated_converter/
├── src/
│   ├── core/                    # 核心功能模块
│   │   ├── iconverter.h         # 转换器接口
│   │   ├── conversion_task.h/cpp # 转换任务类
│   │   ├── task_manager.h/cpp   # 任务管理器
│   │   ├── config_manager.h/cpp # 配置管理
│   │   ├── logger.h/cpp         # 日志系统
│   │   ├── error_handler.h/cpp  # 错误处理
│   │   ├── retry_manager.h/cpp  # 重试管理
│   │   ├── skill_manager.h/cpp  # Skill管理器
│   │   ├── memory_monitor.h/cpp # 内存监控
│   │   └── large_file_handler.h/cpp # 大文件处理
│   ├── converters/              # 转换器实现
│   │   ├── ffmpeg_converter.h/cpp # FFmpeg转换器
│   │   ├── pandoc_converter.h/cpp # Pandoc转换器
│   │   └── segmented_converter.h/cpp # 分段转换器
│   ├── ui/                      # 用户界面
│   │   ├── main_window.h/cpp    # 主窗口
│   │   ├── task_list_widget.h/cpp # 任务列表
│   │   ├── config_panel.h/cpp   # 配置面板
│   │   ├── file_list_widget.h/cpp # 文件列表
│   │   ├── progress_widget.h/cpp # 进度显示
│   │   ├── batch_convert_dialog.h/cpp # 批量转换对话框
│   │   ├── batch_conversion_summary.h/cpp # 转换结果汇总
│   │   ├── error_dialog.h/cpp   # 错误对话框
│   │   └── skill_invoke_dialog.h/cpp # Skill调用对话框
│   └── main.cpp                 # 程序入口
├── tests/                       # 测试文件
├── CMakeLists.txt               # 构建配置
└── README.md                    # 说明文档
```

## 技术架构

### 设计模式
- **单例模式**：Logger、ConfigManager、TaskManager等核心类
- **接口模式**：IConverter定义统一的转换器接口
- **观察者模式**：任务状态变化通过信号槽机制通知
- **工厂模式**：转换器工厂创建适当的转换器

### 线程模型
- **主线程**：负责UI交互和事件处理
- **工作线程池**：使用QThreadPool并行执行转换任务
- **信号槽通信**：Qt::QueuedConnection确保跨线程安全

### 内存管理
- QObject父子关系管理UI组件生命周期
- 智能指针管理转换器对象
- 内存监控防止大文件转换导致内存溢出

## 常见问题

### Q: 转换失败怎么办？
A: 检查错误提示对话框，查看详细错误信息。常见原因：
- 文件不存在或路径错误
- 没有写入权限
- 磁盘空间不足
- FFmpeg/Pandoc未正确安装

### Q: 如何处理大文件？
A: 程序内置大文件优化：
- 自动检测大文件并优化参数
- 支持分段转换超大文件
- 内存监控防止内存溢出

### Q: 如何提高转换速度？
A: 建议：
- 增加并行任务数（在设置中调整）
- 对于不需要重新编码的情况，使用流式复制模式
- 选择更快的编码器（如H.264替代H.265）

### Q: 日志文件在哪里？
A: 默认位置：
- Windows: `C:\Users\<用户名>\AppData\Local\integrated_converter\logs\`

## 更新日志

### v1.0.0 (2024)
- 初始版本发布
- 支持FFmpeg音视频转换
- 支持Pandoc文档转换
- 实现批量转换功能
- 实现并行多任务
- 实现错误处理和日志记录
- 实现Skill调用集成

## 许可证

MIT License

## 联系方式

如有问题或建议，请通过以下方式联系：
- 提交Issue
- 发送邮件

## 致谢

- [Qt](https://www.qt.io/) - 跨平台C++图形用户界面应用程序框架
- [FFmpeg](https://ffmpeg.org/) - 音视频处理工具
- [Pandoc](https://pandoc.org/) - 文档格式转换工具
