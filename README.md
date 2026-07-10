# Arcane Spire / 秘法高塔（暂名）

《C++ 工程与项目设计》课程用 2D 动作 Roguelike 原型。项目使用 C++20、CMake 和 SFML 3。

## 当前可玩内容

- SFML 窗口与渲染循环。
- 设备输入映射到纯 C++ `PlayerIntent`。
- 占位玩家方块的左右移动、跳跃、重力、落地和屏幕边界。
- 不依赖 SFML 的玩家控制器测试。

## 控制

- `A/D` 或方向键：移动。
- `Space`：跳跃。
- `J`：攻击意图（尚未实现攻击表现）。
- `U/I/O`：三个魔法槽意图（尚未实现魔法）。
- `E`：交互意图（尚未实现交互）。

## 配置与构建

在仓库根目录运行：

```powershell
.\scripts\Invoke-VsCMake.ps1 doctor
.\scripts\Invoke-VsCMake.ps1 configure
.\scripts\Invoke-VsCMake.ps1 build-debug
.\scripts\Invoke-VsCMake.ps1 test-debug
```

Release 验证：

```powershell
.\scripts\Invoke-VsCMake.ps1 build-release
.\scripts\Invoke-VsCMake.ps1 test-release
```

Debug 可执行文件根据 Visual Studio 版本生成在：

```text
build/windows-vs2026/Debug/Project1.exe
build/windows-vs2022/Debug/Project1.exe
```

首次配置会从 SFML 官方 GitHub 仓库下载固定的 `3.1.0` 源码。

## 文档

- `AGENTS.md`：开发约束和验证要求。
- `docs/GAME_DESIGN.md`：游戏设计。
- `docs/ARCHITECTURE.md`：系统架构。
- `docs/DEVELOPMENT_PLAN.md`：开发计划。
- `docs/TEAM_WORK_SPLIT.md`：双 C++ 开发者分工。
- `docs/HANDOFF_TO_B.md`：成员 B 克隆、构建、Codex 约束与日常 Git 流程。
- `docs/decisions/0001-sfml3-cmake.md`：技术选型记录。
