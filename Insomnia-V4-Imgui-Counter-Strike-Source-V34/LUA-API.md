# Awesware Lua API — документация

Версия API: **2.1** · Lua **5.4** · Counter-Strike: Source v34

Скрипты кладутся в `C:\Awesware\scripts\*.lua`. Управление: вкладка **SCRIPTS** в меню —
слева список скриптов и кнопки (Reload Scripts / Open Folder), справа — собственное меню
выбранного скрипта. `[!]` у имени = скрипт упал с ошибкой (наведи мышку — увидишь текст).

---

## Быстрый старт

```lua
-- myscript.lua
function on_paint()
    draw.text(8, 60, 255, 255, 255, 255, "hello from lua")
end
```
Сохрани файл → открой меню → SCRIPTS → **Reload Scripts** → включи меню списка — твой скрипт появится.
Кнопка **Open Folder** открывает папку скриптов. При первом запуске с пустой папкой создаётся `example.lua`.

---

## Колбеки (все необязательные)

| Колбек | Когда вызывается | Поток |
|---|---|---|
| `on_tick()` | каждый игровой тик | игровой |
| `on_paint()` | каждый кадр отрисовки | рендер |
| `on_event(ev)` | игровые события (см. game_event) | игровой |
| `on_frame_stage(st)` | стадии кадра (см. client_frame_stage) | рендер |
| `on_menu()` | когда открыта вкладка SCRIPTS и выбран твой скрипт | рендер |

Скрипты выполняются изолированно: у каждого свой `lua_State`. Ошибка в одном не роняет другие —
скрипт помечается `[!]` и останавливается до исправления + Reload.

---

## ui — элементы меню скрипта

Рисуются **только внутри `on_menu()`** (вкладка SCRIPTS, правая панель). Значения хранятся
по `label` (имени элемента) и живут до Reload скрипта. Чтобы прочитать значение из
`on_paint`/`on_tick` — вызывай там же такой же элемент **нельзя** (нарисует его на экране),
используй `ui.get(label, default)`.

| Функция | Возвращает | Описание |
|---|---|---|
| `ui.checkbox(label, default)` | bool | чекбокс |
| `ui.slider_int(label, min, max, default)` | int | слайдер целых |
| `ui.slider_float(label, min, max, default)` | float | слайдер дробных |
| `ui.combo(label, { "a", "b", "c" }, default)` | int | выпадающий список, индекс с 0 |
| `ui.button(label)` | bool | true в кадре нажатия |
| `ui.text(str)` | — | текст |
| `ui.header(str)` | — | красный заголовок с линией |
| `ui.separator()` | — | разделитель |
| `ui.same_line()` | — | следующий элемент на той же строке |
| `ui.get(label, default)` | any | прочитать значение по имени |
| `ui.set(label, value)` | — | принудительно записать значение |

```lua
function on_menu()
    ui.header("мой аим-хелпер")
    ui.checkbox("включён", true)
    ui.slider_int("fov", 1, 30, 5)
    ui.combo("режим", { "мягкий", "жёсткий" }, 0)
    if ui.button("пересчитать") then print("пересчитано!") end
end

function on_paint()
    if not ui.get("включён", true) then return end
    local fov = ui.get("fov", 5)
    draw.text(8, 60, 255, 80, 80, 255, "fov: " .. fov)
end
```

---

## cfg — настройки чита

`cfg.get(name)` / `cfg.set(name, value)` — чтение/запись настроек чит-меню.
`cfg.list()` — таблица всех имён. Полный список биндов:

| Группа | Имена |
|---|---|
| rage | `rage.active` (bool), `rage.autoshoot` (bool), `rage.silent` (bool), `rage.silent_perfect` (bool), `rage.hitscan` (bool), `rage.autowall` (bool), `rage.autostop` (bool), `rage.bodyawp` (bool), `rage.mindamage` (int), `rage.fov` (int), `rage.backtrack` (int), `rage.forcemindmg` (int) |
| resolver | `resolver.enabled` (bool), `resolver.smart` (bool), `resolver.mode` (int), `resolver.type` (int) |
| aa | `aa.enabled` (bool), `aa.pitch` (int), `aa.yaw` (int), `aa.yaw_mode` (int), `aa.real` (float), `aa.fake` (float), `aa.flick` (bool), `aa.flick_every` (int), `aa.flick_angle` (float), `aa.flick_random` (bool), `aa.flick_on_shot` (bool), `aa.at_targets` (bool), `aa.enemy_check` (bool) |
| fakelag | `fakelag.enabled` (bool), `fakelag.ticks` (int), `fakelag.mode` (int), `fakelag.in_attack` (bool), `fakelag.air_only` (bool) |
| dt | `dt.enabled` (bool), `dt.ticks` (int), `dt.auto` (bool), `dt.mode` (int), `dt.ground_only` (bool), `dt.delay_shot` (bool) |
| legit | `legit.enabled` (bool), `legit.autoshoot` (bool), `legit.silent` (bool), `legit.aim_type` (int), `legit.key` (int), `legit.fov` (int), `legit.smoothing` (int), `legit.reaction_ms` (int), `legit.rcs` (int), `legit.backtrack` (int), `legit.scoped_check` (bool), `legit.auto_scope` (bool), `legit.strafe` (bool), `legit.strafe_power` (int) |
| indicators | `indicators` (bool) |
| esp | `esp.box` (bool), `esp.name` (bool), `esp.health` (bool), `esp.weapon` (bool), `esp.bone` (bool), `esp.enemy_only` (bool) |
| nospread | `nospread` (bool), `nospread.mode` (int) |
| forceseed | `forceseed` (bool) |
| bhop | `bhop` (bool) |
| autostrafe | `autostrafe` (bool) |
| circlestrafe | `circlestrafe` (bool) |
| autoknife | `autoknife` (bool) |
| speedhack | `speedhack` (bool), `speedhack.amount` (int) |
| slowwalk | `slowwalk` (bool), `slowwalk.speed` (int) |
| thirdperson_key | `thirdperson_key` (int) |
| thirdperson_dist | `thirdperson_dist` (int) |

> Настройки **legit AA в API нет** — эта группа скриптами не управляется специально.

---

## ents — игроки

Объектное API: `ents.get(index)` → **player** или `nil`, `ents.local_player()` → локальный игрок,
`ents.get_all()` → таблица всех объектов игроков (фильтруй сам по `is_alive`/`get_team`).

```lua
for _, pl in ipairs(ents.get_all()) do
    if pl:is_alive() and pl:get_team() ~= ents.local_player():get_team() then
        local x, y, z = pl:get_origin()
        local sx, sy, vis = draw.world_to_screen(x, y, z + 70)
        if vis then draw.text(sx, sy, 255, 255, 255, 255, pl:get_name(), true) end
    end
end
```

### player

| Метод | Возвращает |
|---|---|
| `:index()` | int — индекс сущности (1..64) |
| `:is_dormant()` | bool — вне зоны потока данных сервера |
| `:is_alive()` | bool |
| `:has_helmet()` | bool — шлем |
| `:get_health()` | int |
| `:get_velocity()` | vector3 |
| `:get_origin()` | vector3 — мировая позиция |
| `:get_angles()` | vector3 — углы глаз (x=pitch, y=yaw, z=0) |
| `:get_team()` | int |
| `:get_movetype()` | int — {1 = stand, 2 = walk, 3 = air} |
| `:is_scoped()` | bool — в зуме (по m_iFOV) |
| `:get_poseparam(i)` / `:set_poseparam(i, v)` | float / — — позы 0..23 |
| `:get_name()` | string — ник с таблицы игроков |
| `:get_animstate()` | animstate |
| `:get_active_weapon()` | weapon |

### animstate

Поля (читаются как свойства, без скобок): `goal_feet_yaw`, `current_feet_yaw`,
`current_torso_yaw`, `last_turn_time`, `feet_yaw_init`, `eye_yaw`, `eye_pitch`.

```lua
local anim = pl:get_animstate()
print(anim.goal_feet_yaw, anim.eye_yaw)
```
> В v34 нет стабильного оффсета внутренней структуры анимстейта, поэтому это
> **живое вычисляемое представление**: `goal_feet_yaw` — реальный серверный feet yaw
> (m_angRotation, тот же источник, что использует анимтест-ресолвер), `eye_*` —
> настоящие сетевые углы, `current_feet_yaw`/торс/`last_turn_time` трекаются на нашей стороне.

### weapon

| Метод | Возвращает |
|---|---|
| `:get_next_attack()` | float — время (curtime) следующего выстрела |
| `:get_clip1()` | int — патроны в магазине |

---

## vector3 / vector2 / color

```lua
local v = vector3.new(1, 2, 3)
v.x = v.x + 1
local w = vector3.new(0, 1, 0)
local d = v:dist_to(w)      -- расстояние
local l = v:length_2d()     -- длина по XY
local s = v * 2             -- умножение на число, v + w, v - w
print(tostring(v))          -- "(2.00, 2.00, 3.00)"

local c = color.new(255, 60, 60, 255)
print(c.r, c.g, c.b, c.a)
c.a = 128

local p = vector2.new(10, 20)
p.y = p.y + 5
```

---

## draw — отрисовка (только в on_paint)

| Функция | Описание |
|---|---|
| `draw.text(x, y, r, g, b, a, text [, center])` | текст (center=true — по центру X) |
| `draw.filled_rect(x, y, w, h, r, g, b, a)` | залитый прямоугольник |
| `draw.outlined_rect(x, y, w, h, r, g, b, a)` | обводка |
| `draw.line(x1, y1, x2, y2, r, g, b, a)` | линия |
| `draw.circle(x, y, radius, r, g, b, a)` | круг |
| `draw.world_to_screen(x, y, z)` | → `visible, sx, sy` (или `false`) — 3D → 2D |

---

## input / utils

| Функция | Описание |
|---|---|
| `input.is_down(vk)` | зажата ли клавиша (VK-код: 0x01 = ЛКМ, 0x20 = пробел...) |
| `utils.tick()` | текущий игровой тик |
| `utils.time()` | системное время (мс) |
| `utils.curtime()` | игровое время |
| `utils.frametime()` | длительность кадра |
| `utils.interval_per_tick()` | длительность тика |

---

## cvars — игровые ConVar'ы

```lua
local sv = cvars.find("sv_cheats")
if sv then
    print(sv:get_name(), sv:get_int())
    -- sv:set_value_int(1)  -- на локальном сервере сработает
end
```
`get_name()` / `get_int()` / `get_float()` / `get_bool()` / `set_value_int(v)` /
`set_value_float(v)` / `set_value_string(s)`. Если переменной нет — `find` вернёт `nil`.

---

## game_event — on_event(ev)

```lua
function on_event(ev)
    if ev:get_name() == "player_hurt" then
        print("dmg:", ev:get_int("dmg_health"), "hp:", ev:get_int("health"))
    end
end
```
Методы: `get_name()`, `get_int(key [, def])`, `get_float(key [, def])`,
`get_bool(key [, def])`, `get_string(key [, def])`.
Полезные события v34: `player_hurt` (attacker, userid, health, armor, weapon, dmg_health),
`player_death` (attacker, userid), `round_start`, `player_connect`, `weapon_fire` (userid, weapon).

---

## client_frame_stage — on_frame_stage(st)

`client_frame_stage.FRAME_UNDEFINED (-1)`, `.FRAME_START (0)`, `.FRAME_NET_UPDATE_START (1)`,
`.FRAME_NET_UPDATE_POSTDATAUPDATE_START (2)`, `.FRAME_NET_UPDATE_POSTDATAUPDATE_END (3)`,
`.FRAME_NET_UPDATE_END (4)`, `.FRAME_RENDER_START (5)`, `.FRAME_RENDER_END (6)`.

```lua
function on_frame_stage(st)
    if st == client_frame_stage.FRAME_RENDER_START then end
end
```

---

## print

`print(...)` пишет в консоль чита (префикс `[lua]`), принимает любые значения и `tostring`.

---

## Песочница и лимиты

- Вырезаны: `io`, `os`, `package`, `debug`, `dofile`, `loadfile`, `require`
- Лимит инструкций на один вызов колбека: 5 000 000 — вечный цикл убивает скрипт с ошибкой
  «script timeout», а не игру
- Ошибка в любом колбеке: скрипт помечается `[!]` с текстом ошибки (тултип в списке + консоль),
  остальные колбеки того же скрипта останавливаются до Reload
- Доступ к другим потокам разрулен внутренним локом — из скрипта ничего блокировать не нужно

## Диагностика

| Симптом | Причина |
|---|---|
| Скрипта нет в списке | файл не `.lua`, либо папка не `C:\Awesware\scripts` |
| `[!]` у имени | наведи мышь — тултип с текстом ошибки; консоль чита дублирует |
| ui-элементы не появились | нет `function on_menu()`, либо скрипт с ошибкой, либо открыт не его экран |
| значения ui слетели | нажат Reload Scripts — состояние элементов живёт до перезагрузки |
| fps просел | тяжёлые циклы в `on_paint` — рисуй только видимое, кэшируй в `on_tick` |

## Полный пример

```lua
-- watermark + меню + лог смертей
function on_menu()
    ui.header("death log")
    ui.checkbox("enabled", true)
    ui.slider_int("y pos", 0, 400, 100)
end

function on_paint()
    local me = ents.local_player()
    if me and me:is_alive() then
        local anim = me:get_animstate()
        draw.filled_rect(6, 6, 120, 22, 20, 20, 20, 150)
        draw.text(12, 10, 255, 255, 255, 255,
            string.format("awesware | %.0f fps", 1.0 / math.max(utils.frametime(), 0.001)))
    end

    if not ui.get("enabled", true) then return end
    local y = ui.get("y pos", 100)
    draw.text(8, y, 179, 46, 46, 255, "death log active")
end

function on_event(ev)
    if ev:get_name() == "player_death" and ui.get("enabled", true) then
        print("kill! userid:", ev:get_int("userid"))
    end
end
```
