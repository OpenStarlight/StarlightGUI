<h1 align="center">
  <p align="center">
    <img src="header.png" alt="Header">
  </p>
    Starlight GUI
  <p align="center">
    <a href="https://www.apache.org/licenses/LICENSE-2.0.html">
      <img src="https://img.shields.io/badge/License-Apache%202.0-blue?logo=cloudera&logoColor=white" alt="License">
    </a>
    <a href="https://opensource.org/license/apache-2-0">
      <img src="https://img.shields.io/badge/OSI-Certified-brightgreen?logo=opensourceinitiative" alt="OSI Certified">
    </a>
    <a href="https://github.com/microsoft/cppwinrt">
      <img src="https://img.shields.io/badge/Language-C++/WinRT-0078d7?logo=cplusplus" alt="Language">
    </a>
    <a href="https://space.bilibili.com/670866766">
      <img src="https://img.shields.io/badge/Bilibili-Stars_Azusa-white?logo=bilibili&logoColor=white&labelColor=ff69b4" alt="Bilibili">
    </a>
    <a href="https://afdian.com/a/StarsAzusa">
      <img src="https://img.shields.io/badge/Afdian-Stars_Azusa-white?logo=afdian&labelColor=green" alt="Bilibili">
    </a>
  </p>
</h1>

Starlight GUI 是一个基于 C++/WinRT 的 WinUI3 开源项目，为开发者的兴趣作品，致力于打造一个功能强大、界面美观、易于使用的开源 Windows 10 / 11 内核级工具箱。项目采用原生的 WinUI3 设计，完美贴合 Windows 11 系统的操作习惯和视觉风格。拥有很多实用功能和自定义设置，增强电脑的使用体验。  

**开发者**: Stars  
**许可证**: Apache 2.0 License | OSI Certified  
**Copyright © 2026 Starlight. Some Rights Reserved.**  
**网盘下载链接**: `https://pan.quark.cn/s/ee7a29ca2a76`  **提取码**: `vVmj`  （也可在 Github 页面下载）

![Alt](https://repobeats.axiom.co/api/embed/33edd92df6ac6348e3eb2c6c1ba38046ca12e037.svg "Repobeats analytics image")

## 主要功能

### 任务管理器
- 完全重构的任务管理器
- 集成大部分系统任务管理器核心功能
- 驱动级操控进程能力（终止、调节、权限等）
- 以高权限启动进程
- 读取、修改进程内存，注入 DLL

### 模块管理器
- 查看系统加载的所有模块
- 轻松加载、卸载驱动
- 加载无签名驱动
- 隐藏驱动能力

### 文件管理器
- 现代化界面文件管理器
- 多种用户级、内核级遍历文件方式
- 强制删除文件

### 窗口管理器
- 查看系统所有窗口（包括隐藏）
- 窗口强制显示、隐藏操作

### 监视器
- ARK 级系统内核资源查看
- 功能完善，数据一览无余

### 设置菜单
- 功能模块配置
- 界面个性化设置

## 开发环境设置

### 环境
- `Windows 10 / 11` 操作系统
- `Visual Studio 2026` （推荐）
- `C++ 20` 及以上
-  `C++/WinRT WinUI3` 开发环境

### 安装步骤

1. **克隆项目**
   ```bash
   git clone https://github.com/RinoRika/StarlightGUI.git
   cd StarlightGUI
   ```

2. **使用 Visual Studio 打开项目**
   - 打开 `Visual Studio`
   - 选择 `打开项目或解决方案`
   - 导航到项目目录，选择 `.sln` 解决方案文件

3. **还原 NuGet 包**
   - 在解决方案资源管理器中右键点击解决方案
   - 选择 `还原 NuGet 包`

4. **配置构建环境**
   - 确保选择正确的构建配置（Debug/Release）
   - 该项目依赖 `WinUI3` ，请注意架构配置

5. **构建和运行**
   - 使用菜单中的 `生成` → `生成解决方案`

6. **生成安装包**
   - 在项目文件夹的 `x64/Release(Debug)` 中将所有文件提取即可， `StarlightGUI.exe` 即为可执行文件
   - 可将上述文件自行创建压缩包或创建安装程序

(OPTIONAL) **注意事项**
1. 如果你在启动项目时遇到了类似 `参数错误` 的报错导致的断点，请直接忽略，这不会影响程序运行。
2. 如果你在切换至某些页面时出现了崩溃，请尝试关闭 Visual Studio 的 `XAML 热重载`。

## 免责声明

**重要提示**: 本项目仅供学习和研究目的使用。使用者需对使用本软件所产生的任何后果承担**全部责任**。

- 本软件没有病毒，由于为高权限操作，杀毒软件报毒是**正常现象**
- 本软件涉及系统底层操作，不当使用可能导致**系统不稳定**
- 通过本软件加载或创建的内容可能有**潜在安全风险**
- 本软件的功能可能会使系统或文件数据**意外损坏或丢失**
> 请尽量不要使用可能导致蓝屏的功能，驱动在操作过程中意外终止可能会导致系统永久性损坏，并引发软件无法启动的严重问题。请您对自己的行为负责。

> 软件的所有内容可以保证在完全符合配置要求的情况下完美运行，请确保您的系统配置符合要求。因该原因引发的所有问题，请勿上报给开发者。

安装并运行本软件，即代表您已**阅读并同意**以上以下内容：

- 您正在**了解相关风险**的情况下使用
- 您因使用本软件造成的任何损失，开发者**均不负责**
- 您遵守当地法律法规，合法使用本软件，不用于**恶意用途**
- 您理解本软件开发者仅为兴趣所做，本软件的最终解释权为**开发者所有**
- 您保证不对本软件及其开发者发表任何**侮辱性、污蔑性言论**
> 我们深知自己可能没有做到最好，并接受来自所有人的批评指正。请保留一份对开发者的尊重。

**驱动特别提醒/声明**:

- 本软件使用的驱动均为**自行开发/授权使用**
- 如果在使用途中遇到驱动报毒，请您**临时**为驱动**添加信任**，并确保您在停止使用该软件后**完全移除信任**
- 驱动的数字签名均来自**非正规渠道**，请勿信任该证书
- 禁止**未经授权**挪用本项目的驱动并用于其它目的
> 由于驱动已被签名，可能存在一定误导性，并可能被有心人士用于非法用途。请确保您没有在本项目（以及 SKT64 驱动授权的项目）以外发现您的电脑正在运行相关驱动，如果发现则请立刻进行安全检查！

> 项目使用的技术大多公开，且在本项目里完全处于合法用处。如果本项目的驱动被来自非本项目的程序使用，则其对应的开发者应当承担自己所做出一切行为的后果！本项目的开发者概不负责！

> 技术无罪，罪在将技术用于非法用途的人。项目开源是为了促进技术发展，愿您遵守这条声明。

## 特别感谢

### AI辅助开发
- **Deepseek**
- **ChatGPT**
- **Copilot**

### 项目支持
- **KALI_MC** - 驱动开发
- **Wormwaker** - 思路启发、宣传
- **MuLin** - 程序测试

### 开发环境
- **Visual Studio** - 强大的集成开发环境
- **Microsoft WinUI3** - 现代化 UI 框架
- **C++/WinRT** - 高效 Windows 运行时支持

## Star History

<a href="https://www.star-history.com/#RinoRika/StarlightGUI&type=date&legend=top-left">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=RinoRika/StarlightGUI&type=date&theme=dark&legend=top-left" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=RinoRika/StarlightGUI&type=date&legend=top-left" />
   <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=RinoRika/StarlightGUI&type=date&legend=top-left" />
 </picture>
</a>

**给我们点亮小星星，这对我们很重要！**  
