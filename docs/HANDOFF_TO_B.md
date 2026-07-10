# 成员 B 项目交付与 Codex 入组指南

## 1. 推荐交付方式

使用一个两人都能访问的私有 Git 仓库作为唯一交付渠道。GitHub、Gitee 或学校 Git 服务均可；不要长期使用聊天软件发送压缩包，否则两人的代码、文档和约束文件会迅速分叉。

仓库应包含：

- C++ 源码、`CMakeLists.txt` 和 `CMakePresets.json`。
- 根目录 `AGENTS.md`。
- `docs/` 下的游戏设计、架构、计划、分工和技术决策。
- `scripts/Invoke-VsCMake.ps1`。
- `.gitignore`、README 和测试源码。

仓库不应包含：

- `.vs/`、`build/`、`x64/` 等本机构建目录。
- 下载到 `build/_deps/` 的 SFML 源码；B 首次配置时会自动下载固定版本。
- 用户级 Codex 配置、个人插件缓存或 `%USERPROFILE%/.codex`。
- 密钥、GitHub token、本地绝对路径和私人付费素材。

## 2. A 端首次推送

在项目根目录检查交付内容：

```powershell
git status --short
.\scripts\Invoke-VsCMake.ps1 doctor
.\scripts\Invoke-VsCMake.ps1 build-debug
.\scripts\Invoke-VsCMake.ps1 test-debug
```

确认后建立基线提交：

```powershell
git add .
git status --short
git commit -m "chore: bootstrap SFML course project"
git branch -M main
```

在选定的 Git 服务创建空的私有仓库后：

```powershell
git remote add origin <私有仓库地址>
git push -u origin main
```

把成员 B 加为协作者。不要把访问令牌直接发进聊天或写入项目文件。

## 3. B 端环境要求

B 需要安装：

- Git for Windows。
- Visual Studio 2022 或 Visual Studio 2026。
- Visual Studio 工作负载“使用 C++ 的桌面开发”。
- Visual Studio 的 C++ CMake 工具组件。
- PowerShell；首次配置时需要能访问 SFML 的 GitHub 仓库。

脚本会自动识别 VS2022/VS2026，并选择对应的 CMake preset。Visual Studio 2019 当前不受支持。

## 4. B 端克隆与构建

```powershell
git clone <私有仓库地址>
cd <仓库目录>
.\scripts\Invoke-VsCMake.ps1 doctor
.\scripts\Invoke-VsCMake.ps1 configure
.\scripts\Invoke-VsCMake.ps1 build-debug
.\scripts\Invoke-VsCMake.ps1 test-debug
```

如果系统阻止执行本地 PowerShell 脚本，只对当前终端临时放行：

```powershell
Set-ExecutionPolicy -Scope Process Bypass
```

随后运行 Debug 程序：

```powershell
# VS2026
.\build\windows-vs2026\Debug\Project1.exe

# VS2022
.\build\windows-vs2022\Debug\Project1.exe
```

验收标准：

- `doctor` 输出环境检查通过。
- CMake 首次配置成功下载并配置 SFML 3.1.0。
- Debug 构建成功。
- CTest 报告 `1/1` 通过。
- 窗口可启动，`A/D` 或方向键移动，`Space` 跳跃。

## 5. B 如何获得相同 Codex 约束

项目约束已经存放在仓库根目录的 `AGENTS.md`，因此会随 `git clone` 一起到达 B 的电脑。B 应在克隆后的仓库根目录打开 Codex 任务，使根 `AGENTS.md` 覆盖整个项目。

B 第一次使用 Codex 时可以先发送：

```text
先阅读根目录 AGENTS.md，以及 docs/TEAM_WORK_SPLIT.md、
docs/ARCHITECTURE.md 和 docs/DEVELOPMENT_PLAN.md。
概括成员 B 的职责、构建命令和禁止修改的边界，暂时不要改代码。
```

这一步用于确认 Codex 看到了正确仓库和约束，而不是用提示词替代 `AGENTS.md`。

### 不会自动同步的内容

- A 电脑上的个人 Codex 设置、记忆、已安装插件和全局技能不会随 Git 仓库同步。
- 当前项目构建不依赖任何个人 Codex 插件或全局技能。
- 如果 B 希望让 Codex 直接浏览/操作 GitHub，可在自己的 Codex 中单独安装 GitHub 插件；这不是编译项目的前置条件。

## 6. B 的首个分支

B 的首个流程分支已于 2026-07-10 提交并通过 A 的隔离构建检查。原始工作分支为 `B`，集成时保留原作者提交历史。

首次环境设置时可使用：

```powershell
git switch -c b/bootstrap-flow
```

首批任务验收状态：

1. `RunContext`、`FloorDescriptor`、确定性 RNG 和流程测试入口已完成。
2. 集成层已改为引用 `game/combat/CombatContracts.hpp` 中的真实 `CombatResult`。
3. 流程测试可直接构造脚本化结果，同时覆盖过期结果拒绝、剩余 HP、奖励、楼梯和三 Boss 胜利。
4. 下一步是在集成 PR 合并后拉取 `main`，从新的 `b/...` 分支继续楼层表现和流程 UI；不要继续在旧 `B` 分支追加无关功能。

## 7. 日常同步流程

每次开始工作：

```powershell
git switch main
git pull --ff-only
git switch <自己的分支>
git merge main
```

完成任务后：

```powershell
.\scripts\Invoke-VsCMake.ps1 build-debug
.\scripts\Invoke-VsCMake.ps1 test-debug
git status --short
git add <本任务文件>
git commit -m "feat(run): <简短说明>"
git push -u origin <自己的分支>
```

由另一位成员评审并合并。不要直接把未构建的工作推到 `main`。

## 8. 常见问题

### 找不到 Visual Studio 或 CMake

重新打开 Visual Studio Installer，确认安装了“使用 C++ 的桌面开发”和 CMake 工具。运行：

```powershell
.\scripts\Invoke-VsCMake.ps1 doctor
```

脚本会报告实际检测到的 Visual Studio 路径和版本。

### SFML 下载失败

首次 `configure` 需要访问 GitHub。先检查网络或 Git 代理，不要把 A 的整个 `build/` 目录提交到仓库，因为其中包含机器相关路径和生成文件。

### B 的 Codex 没有遵守分工

确认 Codex 打开的工作目录是仓库根目录，并让它先概括 `AGENTS.md` 与 `TEAM_WORK_SPLIT.md`。若只打开了仓库外的单个文件，项目级约束可能不在当前任务范围内。

### 两台电脑生成文件不同

只比较源码、CMake、脚本、文档和资源。`build/`、`.vs/` 和编译产物是本地生成内容，不应比较或提交。
