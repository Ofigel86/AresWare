# AresWare — CS:S v34

Внутренний (internal) чит для **Counter-Strike: Source v34** + референсные дампы движка, по которым сверяется логика (autowall, no-spread, resolver, network data).

Internal cheat for **Counter-Strike: Source v34**, with engine source dumps kept as ground-truth reference for game-logic verification (penetration, spread, prediction, networked data).

## Сборка / Build

- Visual Studio 2022+, откройте `Counter-Strike Source v34.sln`
- Конфигурация: **Release | Win32** (x86, MSVC v143)
- Артефакт: `oldcsshook.dll`

## Структура / Layout

```
AresWare/
├── Counter-Strike Source v34.sln        — solution
├── references/                          — engine dumps (не собираются / not built)
│   ├── source-2007-master/              — Source 2007 SDK (game shared code, FX, prediction)
│   ├── hl2sdk-ep1c-game-cstrike-1/      — EP1 cstrike SDK (FireBullet, materials, penetration)
│   └── Segregation-css_nosteam/         — чужой v34-чит для сверки формул spread/seed
└── Counter-Strike Source v34/           — сам чит / the cheat
    ├── oldcsshook.vcxproj(.filters)     — MSBuild project
    ├── Core/                            — точка входа, хуки-инфра, конфиг
    │                                      (Main, Hook, Config, VMTHook, SdkIncludes, NOTES)
    ├── Hooks/                           — реализации хуков движка
    │                                      (CreateMove, CL_Move, FrameStageNotify, PaintTraverse,
    │                                       Prediction, RunCommand, D3D9Hook, OverrideView, …)
    ├── Features/                        — функционал чита
    │                                      (Aimbot+Resolver, NoSpread, Triggerbot/Stuff,
    │                                       Whitelist, Macro, Recorder)
    ├── GUI/                             — ImGui-меню и рисование (GUI, Drawing)
    ├── Vendor/                          — third-party: detours, ade32, XOR, md5, VirtualizerSDK
    ├── SDK/                             — обёртки над движком (netvars, трейсы, tier1.lib)
    └── ImGui/                           — vendored Dear ImGui
```

Правила / rules:
- `#include` ищется через `AdditionalIncludeDirectories` (`$(ProjectDir)` + все папки выше) — старые `#include "X.h"` и `#include "SDK/X.h"` работают без правок.
- `references/` участвует только в анализе: в vcxproj не входит, код на него не ссылается.
