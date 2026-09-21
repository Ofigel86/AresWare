# CS:S Project

Internal-чит (DLL) для **Counter-Strike: Source v34**. Инжектится в процесс игры,
перехватывает интерфейсы движка Source и Direct3D9. Меню на Dear ImGui (`Insert`).

## Сборка (только Windows)

- Visual Studio 2017 (PlatformToolset **v141**), конфигурация **Win32 / x86**
- Windows SDK `10.0.16299.0`
- DirectX SDK June 2010 (переменная `DXSDK_DIR`, линкуется `d3d9.lib`)

Конфигурации: `Debug` → `v34.dll`, `Home` → `v34.dll`, `Release` → `release.dll`.

## Лицензия (HWID-привязка)

По умолчанию проверка **выключена** — DLL работает на любом ПК.

Чтобы собрать приватную версию с привязкой к железу:

1. В `Counter-Strike Source v34/License.hpp` раскомментируйте `#define PRIVATE_BUILD`
   (либо добавьте `PRIVATE_BUILD` в Preprocessor Definitions проекта);
2. Узнайте свой ключ: инжектните Debug-сборку — ключ напечатается через `DPRINT`;
3. Добавьте ключ в `AllowedKeys` в `Counter-Strike Source v34/License.cpp`.

Без совпадения ключа DLL тихо выгружается, не трогая игру.

## Структура

- `Counter-Strike Source v34/` — исходники чита (`Main.cpp` — точка входа,
  `Source.cpp` — интерфейсы/хуки, `Hooked.cpp` — перехваченные функции,
  `Config.*` — конфиги `C:\rraggerr\v34\`, фичи: `Aimbot`, `Triggerbot`, ESP/Chams,
  Anti-Aim, Misc).
  Вкладка аима разделена на **RAGEBOT** и **LEGITBOT**, активный стиль выбирается
  комбо `Style` (работает что-то одно; в легите рейдж-фичи — прострел, хитскан,
  сайлент, носпред, резолвер, бэктрек — принудительно выключены)
- `Other Source/` — справочные SDK и сторонние исходники (в сборке не участвуют)
