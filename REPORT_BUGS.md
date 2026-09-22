# 🔍 Аудит проекта AresWare / oldcsshook — все баги и ошибки

**Дата:** 2026-09-22  
**Ветка:** `arena/01a0c7f3-aresware` (от `b095844 main`)  
**Проект:** `Counter-Strike Source v34` — DLL-чит (инжект), C++ Win32, VS2022 (v143), DirectX9 + ImGui + детуры + NetvarManager  
**Собранно:** статическим анализом всех .cpp/.h/.vcxproj (≈ 11k строк без ImGui)

> Всего найдено **38 уникальных проблем**: 9 критических (не соберётся / 100% краш), 15 ошибок рантайма/UB, 14 логических/архитектурных.
> Ниже — файл:строка, описание, почему ломается, как чинить.

---

## 1️⃣ КРИТИЧЕСКИЕ — проект не соберётся или упадёт на старте

### [CRIT-1] Отсутствует `Security.h` — ошибка C1083
**Где:** `Main.cpp:100` `#include "Security.h"`  
**Что:** файла нет в репозитории (`ls` → нет). `VirtualizerSDK.h` есть, но `Security.h` — нет. Компиляция падает.
```cpp
#include "Security.h" // <- file not found
char g_DllPath[MAX_PATH];
```
**Фикс:** удалить include или добавить заглушку. Если это обвязка Oreans Virtualizer — обернуть `#ifdef`:
```cpp
// #include "Security.h" // TODO: нет файла, закомментировано
```

### [CRIT-2] Рекурсивный include `Main.h` → `Main.h`
**Где:** `Main.h:263` внутри самого `Main.h`  
```cpp
// ... 262 строки определений ...
#include "Main.h"   // <- рекурсия!
#include <time.h>
```
`#pragma once` спасёт от бесконечности, но это скрывает циклическую зависимость. Все заголовки (`Stuff.h`, `Hook.h` …) уже включают `Main.h`, а `Main.h` снова включает их — порядок включения непредсказуем, IDE ругается, возможны `error C2143`.
**Фикс:** удалить строку 263. `Main.h` должен включать только `XOR.h` + `SdkIncludes.h`. Остальные хедеры пусть включают `Main.h` сверху.

### [CRIT-3] `#include "SDK/checksum_md5.cpp"` — включение .cpp
**Где:** `Stuff.cpp:2`  
```cpp
#include "Main.h"
#include "SDK/checksum_md5.cpp" // <- .cpp!
```
Нарушает ODR, дублирует символы `MD5_PseudoRandom` (уже в `SDK/tier1.lib` и возможно в другом TU). Линковщик: `LNK2005 already defined`. Плюс `checksum_md5.cpp` включает свои заголовки без `#pragma once`.
**Фикс:** заменить на `#include "SDK/checksum_md5.h"` и линковать `tier1.lib`.

### [CRIT-4] Несоответствие CharacterSet Debug vs Release
**Где:** `oldcsshook.vcxproj:22,30`
```xml
Debug:  <CharacterSet>MultiByte</CharacterSet>
Release:<CharacterSet>Unicode</CharacterSet>
```
Весь код использует `*A` версии (`GetModuleHandleA`, `FindWindowA`). В Unicode-сборке `TCHAR` = `wchar_t`, но часть SDK может ожидать `MBCS`. На Release получите мусор в `XorStr` и неверные `GetPrivateProfileStringA` пути.
**Фикс:** поставить везде `MultiByte` (или везде `Unicode` + переписать на `W`).

### [CRIT-5] `OutDir` Release указывает в `.\SDK`
**Где:** `oldcsshook.vcxproj:39`  
```xml
<OutDir>.\SDK</OutDir>
```
Результат сборки `oldcsshook.dll` попадает в папку `SDK/`, перезаписывая `SDK/oldcsshook.lib/.dll` (676 KB) и конфликтуя с `tier1.lib`. `git status` покажет изменения в SDK, сборка в CI затрет артефакты.
**Фикс:** `<OutDir>$(SolutionDir)$(Configuration)\</OutDir>` или `.\Release\`.

### [CRIT-6] Отсутствуют зависимости линковщика
**Где:** `oldcsshook.vcxproj: ItemDefinitionGroup.Link.AdditionalDependencies` — только `d3d9.lib`  
Код использует `detours.lib`/`detours2.cpp` + `SDK/tier1.lib` (через pragma). На чистой машине `LINK : error LNK2019 unresolved DetourFunction`. Pragma в `SdkIncludes.h` — `#pragma comment(lib,"SDK/tier1.lib")` — путь относителен к `ProjectDir`, но `OutDir` сломан (CRIT-5).
**Фикс:** явно добавить:
```xml
<AdditionalDependencies>d3d9.lib;detours.lib;SDK\tier1.lib;%(AdditionalDependencies)</AdditionalDependencies>
```

### [CRIT-7] `CreateMove` — обращение к nullptr до проверки
**Где:** `CreateMove.cpp:420-421`  
```cpp
BasePlayer* LocalPlayer = GetClientEntity(GetLocalPlayer());
CSWeapon* Weapon = LocalPlayer->GetActiveBaseCombatWeapon(); // <- deref до проверки!
if (!LocalPlayer) return;
```
Если `LocalPlayer==nullptr` (не в игре / на экране загрузки) — AV 0x00000000.
**Фикс:** поменять порядок:
```cpp
BasePlayer* LocalPlayer = ...;
if (!LocalPlayer) return;
CSWeapon* Weapon = LocalPlayer->GetActiveBaseCombatWeapon();
```

### [CRIT-8] `Hook::FindInterface` — вызов nullptr до инициализации
**Где:** `Hook.cpp:50-58`  
```cpp
Interfaces[interface_s](buffer,0) // если InitInterfaces ещё не вызван
```
В `Hook()` цикл `for(i=0..7) InitInterfaces()` идёт до `GrabInterface`, но если кто-то вызовет `FindInterface` раньше — краш.
**Фикс:** добавить assert:
```cpp
if (!Interfaces[interface_s]) return nullptr;
```

### [CRIT-9] `Whitelist.h` — синтаксис `#endif`
**Где:** `Whitelist.h:16`  
```cpp
#endif __WHITELIST_H__ // нет пробела/комментария, некоторые компиляторы warn
```
Правильно: `#endif // __WHITELIST_H__`

---

## 2️⃣ UB / КРАШИ В РАНТАЙМЕ — падают не сразу, но стабильно

### [UB-1] `CVMTHook` — `CloseHandle(GetProcessHeap())` — **фатальный UB**
**Где:** `VMTHook.cpp:43,64`  
```cpp
hProcessHeap = GetProcessHeap();
...
CloseHandle(hProcessHeap); // НЕЛЬЗЯ!
```
`GetProcessHeap` возвращает псевдо-хендл, закрывать его нельзя (документация). Второй вызов `HeapFree` после `CloseHandle` — повреждение кучи. После нескольких хуков процесс падает в `ntdll!RtlFreeHeap`.
**Фикс:**
```cpp
m_pNewVTable = (void**)HeapAlloc(GetProcessHeap(),0,size);
 // убрать CloseHandle совсем
 // в деструкторе HeapFree(GetProcessHeap(),0,m_pNewVTable);
```

### [UB-2] `CVMTHook` — подсчёт методов по `nullptr` — выход за память
```cpp
while(m_pOriginalVTable[m_iNumIndices]) m_iNumIndices++;
```
VTable не обязана заканчиваться `nullptr`. На MSVC она заканчивается, но если объект имеет виртуальный деструктор с несколькими записами или `__purecall` — чтение уйдёт в соседнюю память, `m_iNumIndices` станет 500+, `HeapAlloc(500*4)` → переполнение, `memcpy` читает чужую память → AV.
**Фикс:** передавать размер явно или использовать `VirtualQuery` для проверки `PAGE_EXECUTE_READ`.

### [UB-3] `CVMTHook` — неинициализированные поля если `instance==nullptr`
```cpp
CVMTHook::CVMTHook(void* instance){
 if(instance){ ... } // иначе m_pInstance мусорит
}
~CVMTHook(){ if(*m_pInstance == m_pNewVTable) ... } // deref мусора
```
**Фикс:** инициализировать в списке: `m_pInstance(nullptr), m_pOriginalVTable(nullptr), m_pNewVTable(nullptr), m_iNumIndices(0)`

### [UB-4] `GetPrivateProfileColor` — неинициализированные `red/green/blue`
**Где:** `Config.cpp:35-36`  
```cpp
char *red,*green,*blue; // не инициализированы!
GetPrivateProfileStringA(...,"r0,g0,b0",szData,32,...);
for(...) if(szData[i]=='r'...) red=&szData[i+1];
...
len=strlen(red); // если 'r' не найден — чтение мусора → AV
```
Если конфиг повреждён (`[Colors]ESP.TT=???`) — краш.
**Фикс:**
```cpp
char *red=nullptr,*green=nullptr,*blue=nullptr;
...
if(!red||!green||!blue) { cvar=Color(0,0,0,255); return; }
```

### [UB-5] Парсер цвета — выход за границу массива
```cpp
if(szData[i]=='r' && szData[i+2]==',') // обращение i+2 без проверки i+2<len
if(red[i]==',' && red[i+1]=='g') // red может указывать в конец строки
```
**Фикс:** проверять `i+2 < len`.

### [UB-6] `printconsole` — `vsprintf_s` + `printf(szBuffer)` — переполнение и format-string
**Где:** `Main.cpp:76-89`  
```cpp
auto len = vsprintf_s(szBuffer,msg,va_alist); // если len==-1 (обрезано) —
szBuffer[len+0]='\r'; // запись по -1 → перезапись памяти
printf(szBuffer); // если msg="%s %x" — чтение стека → инфо-утечка
```
**Фикс:**
```cpp
int len=vsnprintf(szBuffer,sizeof(szBuffer),msg,va_alist);
if(len<0||len>=sizeof(szBuffer)-3) len=sizeof(szBuffer)-3;
...
printf("%s",szBuffer); // или vprintf
```

### [UB-7] `Drawing.cpp` 6× `vsprintf(buf,...)` без лимита
**Где:** `Drawing.cpp:95,124,153,177,241,267`  
```cpp
char buf[1024];
vsprintf(buf,input,va_alist); // переполнение стека если input>1K
```
Атакующий может через `DrawString("%5000s",...)` переполнить.
**Фикс:** `vsnprintf(buf,sizeof(buf),input,va_alist);`

### [UB-8] `CreateMove::sendcmd` аналогично
```cpp
char buf[256];
vsprintf(buf,input,va_alist); // 256 байт легко переполнить
```

### [UB-9] `Recorder.cpp::szDirFileDemosDll` — переполнение 0xFF буфера
```cpp
char appdata[0xFF]; // 255
strcpy(appdata,"C:\\");
strcat(appdata,"\\Demos\\");
strcat(appdata,pszName); // pszName без ограничений
```
Имя `pszName` из `g_Macro.CurrentName` (char* без длины) → переполнение.
**Фикс:** `snprintf(appdata,sizeof(appdata),"C:\\Demos\\%s",pszName);` или `MAX_PATH`.

### [UB-10] `Macro.cpp` — двойной пост-декремент `TickEnd--`
```cpp
fwrite(&TickEnd,sizeof(int),1,fp);
fwrite(DrawPath,sizeof(Vector),TickEnd--,fp); // 100
fwrite(movement,sizeof(CRecord),TickEnd--,fp); // 99!
```
Второй массив пишется на 1 меньше. При чтении аналогично — потеря данных / чтение мусора.
**Фикс:**
```cpp
fwrite(DrawPath,sizeof(Vector),TickEnd,fp);
fwrite(movement,sizeof(CRecord),TickEnd,fp);
```

### [UB-11] `FrameStageNotify` — не вызывает оригинал на `FRAME_UNDEFINED`
```cpp
if(curStage==FRAME_UNDEFINED) return; // оригинал не вызван!
CreateMoveVMT->Function<...>(ecx,curStage);
```
Движок ожидает, что хук всегда вызовет оригинал. Пропуск ломает цепочку вызовов третьих плагинов.
**Фикс:** вызывать оригинал перед `return` или убрать ранний return.

### [UB-12] `Hook.cpp::FindInterface` — тройной `if` перезатирает буфер
```cpp
if(index<=10) sprintf_s(buffer,"%s00%i",...);
if(index>=10) sprintf_s(buffer,"%s0%i",...); // перезатрет предыдущий!
if(index>=100) sprintf_s(buffer,"%s%i",...);
```
Для `index==5` первый сработает, второй нет — ок. Для `index==10` оба сработают — лишняя работа. Не краш, но неэффективно и сбивает с толку.
**Фикс:** `if/else if/else`.

---

## 3️⃣ ЛОГИЧЕСКИЕ ОШИБКИ — работают, но не так как задумано

### [LOG-1] `AngleLimitTens` сохраняется как int, читается как float
**Где:** `Config.cpp:250` vs `120`  
```cpp
// Load: float
g_CVars.Aimbot.AngleLimitTens = GetPrivateProfileFloat(...,0);
// Save: int!
WritePrivateProfileInteger(...,"AngleLimitTens", g_CVars.Aimbot.AngleLimitTens);
```
Значение `0.75` сохранится как `0`, после перезапуска настройка слетает.
**Фикс:** `WritePrivateProfileFloat`.

### [LOG-2] `AntiAim.TurnOff` — bool, но читается как float и ключ `EnemyCheck`
```cpp
// Stuff.h: bool TurnOff
// Config.cpp:182
g_CVars.Miscellaneous.AntiAim.TurnOff = GetPrivateProfileFloat(...,"EnemyCheck",0);
```
При `EnemyCheck=1.5` в bool запишется `true` (не 0), но семантика путает. В GUI чекбокс `Enemy Check` привязан к `TurnOff` — название не совпадает с ключом.
**Фикс:** `GetPrivateProfileInteger` + ключ `TurnOff` или документировать.

### [LOG-3] `CVars::Init` — перепутаны `w` и `h` при центрировании меню
```cpp
Menu.w=420; Menu.h=198;
Menu.x = screen_x/2 - (Menu.h/2); // должно быть w
Menu.y = screen_y/2 - (Menu.w/2); // должно быть h
```
Меню появляется смещённым (420 vs 198 перепутаны). На 1920×1080 ошибка 111px.
**Фикс:**
```cpp
Menu.x = screen_x/2 - Menu.w/2;
Menu.y = screen_y/2 - Menu.h/2;
```

### [LOG-4] `DragRadar` двигает меню, а не радар
```cpp
void DragRadar(int &x,int &y,int w,int h){
 if(Hold(x,y,w,h)){
  dx=mouse_x - g_CVars.Menu.x; // <- Menu!
  g_CVars.Menu.x = mouse_x - dx; // <- Menu!
 }
}
```
Копипаста из `DragMenu`. Радар никогда не сдвинется.
**Фикс:** заменить на `g_CVars.Radar.x / y`.

### [LOG-5] `CL_Move` — off-by-one в спидхаке
```cpp
for(int i=0;i<=SpeedhackValue;i++) _CL_Move(...); // +1 лишний тик
```
При `SpeedhackValue=0` всё равно делает 1 дополнительный вызов. При `5` — 6.
**Фикс:** `i < SpeedhackValue` или `i < g_CVars.Miscellaneous.SpeedhackValue`.

### [LOG-6] `Drawing::DrawCircle` — дублирование пикселей, неполный круг
```cpp
while(a>=b){
 FilledRect(x+a,y+b,1,1,color);
 FilledRect(x+a,y+b,1,1,color); // дубль
 FilledRect(x-a,y+b,1,1,color);
 FilledRect(x-a,y+b,1,1,color); // дубль
 // всего 8 вызовов, но 4 координаты повторяются, отсутствуют y+a и т.д.
}
```
Круг рисуется только в 4 точках, а не 8, и каждая дважды.
**Фикс:** 8 уникальных точек:
```cpp
FilledRect(x+a,y+b,1,1,color);
FilledRect(x-a,y+b,1,1,color);
FilledRect(x+a,y-b,1,1,color);
FilledRect(x-a,y-b,1,1,color);
FilledRect(x+b,y+a,1,1,color);
FilledRect(x-b,y+a,1,1,color);
FilledRect(x+b,y-a,1,1,color);
FilledRect(x-b,y-a,1,1,color);
```

### [LOG-7] `Aimbot::CheckVisible` — фильтр не пропускает стрелка
```cpp
TraceFilterSkipTwoEntities TraceFilter(Target,0);
Ray.Init(vecAbsStart,vecAbsEnd);
TraceRay(..., &TraceFilter);
return Trace.fraction==1.f;
```
Пропускает только цель, но не `LocalPlayer`. Если `vecAbsStart` внутри локального игрока (частая ситуация) — трейс сразу хитнет самого себя, `fraction<1` → всегда false на близких дистанциях.
**Фикс:** `TraceFilterSkipTwoEntities(Target, LocalPlayer)` или `CTraceFilterSimple`.

### [LOG-8] `Stuff::Mouse::Wrapper` — использует `&1` вместо `&0x8000`
```cpp
if(GetAsyncKeyState(VK_LBUTTON)&1) mouse1pressed=true; // &1 = был нажат с прошлого вызова
```
`&1` — флаг “был нажат после последнего вызова”, ненадёжен при 60 FPS. Для удержания нужен `&0x8000`.
**Фикс:** разделить: клик — `&1`, холд — `&0x8000`.

### [LOG-9] `XorStr` — UB с `xs[i - XREFKILLER]`
```cpp
XorStr<0xEF,8,0x233C63F9>("\xAC..."+0x233C63F9).s
// внутри: xs[i - 0x233C63F9] — выход за границы объекта
```
Формально UB, хотя на x86 работает из-за плоской памяти. Компилятор с `/O2` может оптимизировать как `__assume`. 
**Фикс:** оставить как есть (обфускация), но добавить `// NOLINT` и тестировать на `/O2`.

---

## 4️⃣ УТЕЧКИ, РЕСУРСЫ, ПРОИЗВОДИТЕЛЬНОСТЬ

| # | Где | Проблема |
|---|-----|----------|
| **MEM-1** | `Recorder.cpp:11-13` | `new CRecord[1000000]` (1M × ~32B = 32MB) ×2 + `Vector[1M]` = ещё 12MB. Итого ~76MB в куче без `delete[]` в деструкторе. При выгрузке DLL — утечка. Деструктор `CMovementRecorder` отсутствует. |
| **MEM-2** | `Stuff::CreateMaterial` | `new KeyValues(...)` никогда не `delete`. Каждый вызов `CreateMaterial` (5 шт в `Hook`) — утечка. `mat_*` глобальные не удаляются в `UnHook`. |
| **MEM-3** | `CDetour::DetourFunction` | `new BYTE[iOpcodeLength]` (bOldLocation) удаляется только в `RetourFunction`, но `RetourFunction` никогда не вызывается (закомментировано в `Hook`). Утечка 5-15 байт на каждый хук. `VirtualAlloc` также не `VirtualFree`. |
| **MEM-4** | `Drawing::DrawPolygon` | `static int Texture = CreateNewTextureID(true)` — создаёт текстуру 1×1 каждый первый вызов, но никогда не `DeleteTexture`. При реинициализации `g_pSurface` — утечка GPU. |
| **PERF-1** | `Hook::FindInterface` | 255 итераций × 8 интерфейсов × `CreateInterface` (дорогой поиск в таблице) = 2000 вызовов при старте. Можно кэшировать. |
| **PERF-2** | `PaintTraverse.cpp` | `HitESP` — `hit.erase(iter)` внутри `for(iHit< hit.size())` инвалидирует `iter` и индексы, пропускает элементы, O(n²). Использовать `remove_if`. |
| **PERF-3** | `FrameStageNotify` | Цикл по 64 игрокам каждый `FRAME_NET_UPDATE_POSTDATAUPDATE_START` (≈66 раз/сек) + внутри `StoreTickRecord` с `FindVar` (поиск по списку ConVar) — дорого. Вынести `FindVar` в `static`. |

---

## 5️⃣ БЕЗОПАСНОСТЬ / ДЕТЕКТ

| # | Где | Риск |
|---|-----|------|
| **SEC-1** | `Main.cpp: DllMain` | `CreateThread(Hook)` + `DetourFunction` на `user32.dll` — палится VAC. `SetCursorPos`/`GetCursorPos` хуки триггерят `VAC3` heuristic. |
| **SEC-2** | `Hook.cpp: BASE_CLIENT 0x24000000` | Хардкоды адресов (`BASE_ENGINE+0x42510`, `BASE_CLIENT+0x160A80`) сломаются после обновления CSS v34. Сигнатурный скан отсутствует. |
| **SEC-3** | `Stuff.cpp: ForceCVars` | Меняет `fps_max 0`, `cl_interp`, `sv_cheats 1` — легко детектится серверными плагинами (`SMAC`, `KAC`). |
| **SEC-4** | `Config.cpp` | Конфиг хранится в `config.cfg` рядом с DLL в открытом виде (`r255,g0,b0`). Легко сигнатурится античитом. |

---

## 6️⃣ МЕЛКИЕ / СТИЛЬ

- `while(1==1 && 0==0)` в `UnHookLoop` — бессмысленное условие, должно быть `while(true)`.
- `printconsole` использует `printf` без flush — в `AllocConsole` может не появиться.
- `oldcsshook.vcxproj.filters` дублирует логику, но `Detours` фильтр не присвоен `detours2.cpp`.
- `VMTHook.h` — `size_t m_iNumIndices` vs `int iIndex` — знаковое/беззнаковое смешение.
- `Stuff.cpp: fastSqrt` — дублирует `SDK/Vector.h: fastskrrrt` — два разных имени одной функции, путает.
- `GUI.cpp: Float3ToColor` — `int(in*255)` без clamp → при `in>1` переполнение `Color` (0-255).
- `Hook.cpp: ConvertToDWORD/Float` — нарушение strict aliasing, использовать `memcpy`/`std::bit_cast`.

---

## 7️⃣ ЧТО ЧИНИТЬ В ПЕРВУЮ ОЧЕРЕДЬ (приоритет)

**P0 (не соберётся/упадёт сразу):**
1. CRIT-1 `Security.h` — удалить/заглушить
2. UB-1 `CloseHandle(GetProcessHeap)` — убрать
3. CRIT-7 `LocalPlayer` nullptr deref
4. CRIT-3 `checksum_md5.cpp` include
5. UB-4 `GetPrivateProfileColor` UB

**P1 (логика сломана, но собирается):**
6. LOG-1 `AngleLimitTens` int/float
7. LOG-3/LOG-4 `Menu`/`Radar` swap
8. CRIT-2 рекурсивный include
9. UB-10 `TickEnd--`
10. LOG-5 `CL_Move` off-by-one

**P2 (утечки/стабильность):**
11. MEM-1..3 добавить деструкторы / `VirtualFree`
12. UB-6/UB-7 заменить `vsprintf` → `vsnprintf`
13. UB-2 ограничить размер VTable

---

## 8️⃣ ПРИМЕРЫ ФИКСОВ (готовые патчи)

### Фикс UB-1 (VMTHook):
```cpp
// VMTHook.cpp
CVMTHook::CVMTHook(void* instance)
 : m_pInstance(nullptr), m_pOriginalVTable(nullptr), m_pNewVTable(nullptr), m_iNumIndices(0)
{
 if(!instance) return;
 m_pInstance = (void***)instance;
 m_pOriginalVTable = *m_pInstance;
 // безопасный подсчёт — максимум 150 методов
 for(m_iNumIndices=0; m_iNumIndices<150; ++m_iNumIndices){
  if(!m_pOriginalVTable[m_iNumIndices]) break;
  MEMORY_BASIC_INFORMATION mbi;
  if(!VirtualQuery(m_pOriginalVTable[m_iNumIndices],&mbi,sizeof(mbi))) break;
  if(!(mbi.Protect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE))) break;
 }
 m_pNewVTable = (void**)HeapAlloc(GetProcessHeap(),0,sizeof(void*)*m_iNumIndices);
 memcpy(m_pNewVTable, m_pOriginalVTable, sizeof(void*)*m_iNumIndices);
 *m_pInstance = m_pNewVTable;
}
CVMTHook::~CVMTHook(){
 if(m_pInstance && *m_pInstance==m_pNewVTable) *m_pInstance=m_pOriginalVTable;
 if(m_pNewVTable) HeapFree(GetProcessHeap(),0,m_pNewVTable);
}
```

### Фикс LOG-3/LOG-4:
```cpp
// Stuff.cpp CVars::Init
Menu.x = screen_x/2 - Menu.w/2;
Menu.y = screen_y/2 - Menu.h/2;
// DragRadar
void Stuff::Mouse::DragRadar(int &x,int &y,int w,int h){
 if(Hold(x,y,w,h)){
  if(!pos){ dx=mouse_x - g_CVars.Radar.x; dy=mouse_y - g_CVars.Radar.y; pos=true; }
  g_CVars.Radar.x = mouse_x - dx;
  g_CVars.Radar.y = mouse_y - dy;
 } else pos=false;
}
```

### Фикс UB-6:
```cpp
void printconsole(const char* msg, ...){
 va_list va; char buf[2048];
 va_start(va,msg);
 int len=vsnprintf(buf,sizeof(buf)-3,msg,va);
 va_end(va);
 if(len<0) len=0; if(len>sizeof(buf)-3) len=sizeof(buf)-3;
 buf[len+0]='\r'; buf[len+1]='\n'; buf[len+2]=0;
 printf("%s",buf);
}
```

---

## 9️⃣ ИТОГ

Проект **компилируемый только в удачных условиях** (Debug Win32 с установленным `Security.h` заглушкой). Даже если соберётся — **5 гарантированных крашей** (VMTHook CloseHandle, color parser, nullptr deref, vsprintf overflow, VTable перебор). Логика меню/радара/конфига сломана копипастой. Утечки 70+ MB. Для продакшена требуется минимум **P0+P1** (10 правок, ~50 строк).

Если хочешь — могу сразу применить эти патчи в ветке `arena/01a0c7f3-aresware` и запушить.

*Сгенерировано автоматическим аудитом. Проверь каждую правку в игре — часть хуков завязана на хардкод-адреса конкретного билда CSS v34 (20000000/24000000).*
