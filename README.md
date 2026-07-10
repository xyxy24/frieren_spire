# Arcane Spire / 秘法高塔（暂名）

《C++ 工程与项目设计》课程用 2D 动作 Roguelike 原型。项目使用 C++20、CMake 和 SFML 3。

## 当前可玩内容

- SFML 窗口与渲染循环。
- 设备输入映射到纯 C++ `PlayerIntent`。
- 占位玩家方块的左右移动、跳跃、重力、落地和屏幕边界。
- 基础近战攻击、攻击冷却、训练敌人、接触伤害与双方 HP 条。
- 战斗胜负结果：击败训练敌人后返回金币奖励，玩家 HP 归零后返回失败。
- 可见流程闭环：战斗、魔法奖励三选一、返回原地图、靠近楼梯交互、跨层回血和下一层。
- 可随时打开的魔法装备栏会显示全部已学魔法；选择奖励不会强制替换现有槽位。
- 每三层为一个占位 Boss 层，击败第三个 Boss 后进入胜利状态。
- 不依赖 SFML 的玩家控制器、战斗规则与流程测试。

## 控制

- `A/D` 或方向键：移动。
- `Space`：跳跃。
- `J`：基础近战攻击；同一次挥击只结算一次伤害。
- `Tab`：随时打开或关闭魔法装备栏；打开时暂停底层玩法。
- 奖励界面的 `U/I/O`：选择左/中/右魔法。
- 装备栏中的 `A/D` 或方向键：浏览全部已学魔法；`U/I/O`：把当前选中魔法装备到槽位 1/2/3。
- 战后返回地图，移动到右侧楼梯实体中并按 `E`：进入下一层并恢复 50% 已损失 HP。

窗口标题会显示当前楼层、HP、金币、Boss 进度和当前可用操作。魔法效果本身仍是后续 M1 工作。

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
