# vcos_kernel_nuttx代码仓库说明

## 1. 代码仓库简介
vcos_kernel_nuttx仓库是智能车控OS（VCOS）基于开源NuttX项目，定制化的内核代码仓库

智能车控OS（VCOS）具备实时性，小型化等特点，同时兼容POSIX标准和AUTOSAR标准

## 2. 代码目录
vcos_kernel_nuttx源代码在haloosspace/vcos/kernel/nuttx目录下，目录结构如下图所示：
```
nuttx
├── arch                    # 架构实现
├── audio                   # 音频框架
├── AUTHORS
├── binfmt                  # 二进制包 elf
├── boards                  # 开源板级代码
├── cmake                   # cmake 构建脚本
├── CMakeLists.txt
├── CONTRIBUTING.md
├── crypto                  # BSD 加密框架
├── Documentation           # 文档
├── drivers                 # 驱动框架
├── dummy                   # 伪挂载点，定制板解耦
├── fs                      # 虚拟文件系统 / 文件系统
├── graphics                # 内核图形库
├── include                 # 内核头文件
├── INVIOLABLES.md
├── Kconfig
├── libs                    # libc / libc++ 核心库实现
├── LICENSE
├── Makefile
├── mm                      # 内存管理
├── net                     # TCP/IP、 BT、CAN、MAC
├── NOTICE
├── openamp                 # 异构通信库
├── pass1                   # Protect 模式 阶段一脚本
├── README.md               # NuttX项目自述文件
├── README_VCOS.md          # VCOS项目自述文件
├── ReleaseNotes
├── sched                   # 调度器
├── syscall                 # 系统调用
├── tools                   # 工具和相关脚本
├── video                   # 视频库
└── wireless                # 无线库
```
## 3. 仓库使用入门
智能车控OS（VCOS）基于车载系统场景的[关键技术文档](https://gitee.com/haloos/vcos/tree/master/key_technical)

NuttX基础功能使用入门参照[NuttX项目自述文档](./README.md)
