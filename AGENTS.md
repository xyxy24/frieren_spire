# Project1 — Agent Instructions

## Project Overview

This repository is the bootstrap for a Windows-first 2D action roguelike tower-climbing game. The player climbs one generated floor at a time, collects spells and relics, defeats three bosses, and wins after the third boss.

Treat these documents as the product source of truth:

- `docs/GAME_DESIGN.md` — player-facing rules, scope, terminology, and open design decisions.
- `docs/ARCHITECTURE.md` — system boundaries, state ownership, data flow, and planned technical structure.
- `docs/DEVELOPMENT_PLAN.md` — milestone order, acceptance gates, and immediate backlog.
- `docs/TEAM_WORK_SPLIT.md` — two-person ownership, handoff contracts, integration rules, and course-delivery schedule.
- `docs/HANDOFF_TO_B.md` — repository handoff, teammate environment setup, Codex onboarding, and daily Git flow.
- `docs/decisions/0001-sfml3-cmake.md` — accepted SFML 3/CMake technology decision and consequences.

When implementation and documentation disagree, determine whether the code or design is intended to change, then update the owning document in the same task. Do not silently let them drift.

## Current Repository State

- Language: C++20.
- Authoritative build system: CMake with `CMakePresets.json`.
- Current Windows toolchain: Visual Studio 2026, MSVC v145, x64 Debug/Release. The wrapper also supports Visual Studio 2022 for the teammate.
- Framework: statically linked SFML 3.1.0 fetched by CMake; Graphics/Window/System are enabled, Audio is intentionally deferred.
- Application target: `Project1`.
- Pure C++ library target: `arcane_core`.
- Test targets: `arcane_core_tests`, `combat_session_tests`, and `run_flow_tests`, registered with CTest.
- Implemented probe: SFML window/input adapter; player movement and facing; basic attack timing, AABB hit detection, HP, contact damage, a static training enemy and health bars; a visible combat → reward → return-to-map → spatial staircase → next-floor loop; an independently toggled learned-spell loadout overlay; deterministic floor/reward streams; merchant/event backend transactions; three-boss victory; and a real `CombatResult` handoff between combat and run domains.
- `Project1.slnx` and `Project1/Project1.vcxproj` are legacy empty-project files retained temporarily. Do not add new source or dependencies to them; CMake is the source of truth.

## Build & Verification

Run commands from the repository root in PowerShell. Initial configuration downloads the pinned SFML source and requires network access; later incremental builds use `build/`.

```powershell
.\scripts\Invoke-VsCMake.ps1 doctor
.\scripts\Invoke-VsCMake.ps1 configure
.\scripts\Invoke-VsCMake.ps1 build-debug
.\scripts\Invoke-VsCMake.ps1 test-debug
.\scripts\Invoke-VsCMake.ps1 build-release
.\scripts\Invoke-VsCMake.ps1 test-release
```

- At minimum, run the Debug x64 build and tests after C++ or CMake changes.
- Run Release x64 when changing performance-sensitive code, compiler settings, packaging, or third-party integration.
- Project-owned targets must compile with zero errors and zero warnings. Warnings emitted only while building pinned third-party source must be recorded separately and must not be hidden by lowering project warning levels.
- Do not claim tests passed unless the matching `test-*` command was run and CTest reported success.
- When adding deterministic, non-rendering behavior, add or update an `arcane_core` CTest target in the same change.

## Development Workflow

1. Read the relevant design and architecture sections before editing.
2. Identify the smallest milestone or acceptance criterion the change serves.
3. Inspect adjacent code and configuration; never invent commands, APIs, paths, or conventions.
4. Keep gameplay-domain logic independent from rendering, input devices, audio, and file formats.
5. Build and run the smallest relevant verification.
6. Report what changed, what was verified, and any design assumption introduced.

Prefer a playable vertical slice over broad scaffolding. Do not create every directory or abstraction from `docs/ARCHITECTURE.md` in advance; create a boundary when a current feature needs it.

## Game-Rule Invariants

Unless the user explicitly changes the design, preserve these rules:

- A run ends in victory after defeating the third boss.
- The player exposes HP, gold, learned spells, relics, and exactly three equipped spell slots.
- Each equipped spell has its own cooldown. No mana resource is currently specified.
- A normal combat victory grants gold and a choice of one spell from three candidates.
- A boss victory grants a choice of one stronger spell from three candidates.
- Learned spells and equipped spell slots are separate states; choosing a spell does not automatically replace a slot.
- Using the staircase completes the current floor, restores 50% of missing HP, then loads the next floor.
- The recovery formula is `currentHP += (maxHP - currentHP) * 0.5`, clamped to `maxHP`; integer rounding must be centralized and tested.
- Only the active floor is required in memory. Floor generation must be reproducible from run/floor seeds for debugging.

If any invariant changes, update `docs/GAME_DESIGN.md`, affected tests, and relevant architecture notes together.

## Architecture Rules

- Use explicit ownership and RAII. Owning raw pointers are not allowed.
- Avoid mutable global state and gameplay singletons. Run, floor, combat, reward, and RNG state must have clear owners.
- Use `enum class` or equivalent typed states for run, floor, encounter, and player-state transitions.
- Keep simulation values in consistent units and use delta time or fixed-step simulation where appropriate.
- Keep content data separate from behavior so spells, relics, enemies, encounters, and rewards can be balanced without recompiling gameplay code once a data format is selected.
- Random decisions must receive an explicit deterministic RNG stream. Do not call ambient/global randomness inside domain systems.
- Separate learned-spell inventory from the three-slot combat loadout.
- Centralize damage, healing, cooldown, reward, and relic-modifier calculations; UI must display results, not reimplement rules.
- Do not introduce ECS, object pooling, multithreading, or a service locator merely because they are common in games. Add them only after a measured need or an approved architectural decision.

## Code Style

- Use C++20 and follow the style of nearby files once source exists.
- Prefer small types with one clear responsibility and narrow public interfaces.
- Prefer value types and `std::unique_ptr` for exclusive ownership; use shared ownership only when lifetime requirements genuinely demand it.
- Mark immutable data and methods `const` where it communicates the contract.
- Use fixed-width integer types for serialized IDs, seeds, counters, and save-data fields when size matters.
- Use named domain types or scoped enums instead of magic integers and strings for spell IDs, relic IDs, floor types, and game states.
- Avoid per-frame heap allocation in proven hot paths, but profile before adding pools or custom allocators.
- Comments should explain intent, constraints, or non-obvious tradeoffs rather than restating code.

## Coursework Asset Boundaries

- This is a non-commercial course prototype. Reference-based placeholder art and classroom-only study material are acceptable when the team agrees they help finish the assignment.
- Keep a simple source list for borrowed or placeholder material so the presentation can identify references and any later public build can replace them deliberately. Commercial-release compliance is not a delivery gate for the course version.
- Do not commit private credentials, paid asset-store packages that cannot be shared between teammates, or machine-local SDK paths.

## Boundaries

- **Always do:** preserve deterministic seeds for reproducible runs; keep rules and UI calculations consistent; update owning docs when behavior changes; verify edited project files build.
- **Ask first:** replace SFML/CMake; add another third-party dependency; change save compatibility; introduce online services; change a core rule; add large binary assets; rename the project; restructure the repository broadly.
- **Never do:** edit `.vs/` files; commit generated build outputs; load every tower floor at run start; hide gameplay state in globals; fabricate a successful test/build result; add material that the other teammate cannot access or build with.

## Generated and Local Files

- Treat `.vs/`, `build/`, `x64/`, `Debug/`, `Release/`, `*.user`, build logs, caches, and generated intermediates as local/generated artifacts.
- Do not hand-edit generated Visual Studio databases or build intermediates.
- Preserve user-local files unless explicitly asked to remove them.

## Definition of Done

A task is complete only when:

- Its behavior matches the relevant acceptance criterion or the design document is deliberately updated.
- The applicable build succeeds.
- Deterministic domain behavior has tests when a test harness exists.
- No unrelated files or user changes were overwritten.
- The handoff lists verification performed and any remaining limitation or open decision.
