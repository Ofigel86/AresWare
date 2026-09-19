// BUILD MARKER r25 (2026-09-18): on_create_move dispatch + interp-off (Segregation 0x36200) + BestDamage full-scan + spectator list + dual hitbox selectors (old base combo removed).
// BUILD MARKER r23 (2026-09-18): exact-seed hitchance (256-seed census + ForceSeed-aware) + ForceSeed prefers hit seeds + Lua: on_shot, draw.get_screen_size, utils.latency/choke, ents.eye_angles/hitbox.
#ifndef __LUAAPI_H__
#define __LUAAPI_H__

#include <string>
#include <vector>

// embedded Lua 5.4 scripting: scripts live in C:\Awesware\scripts\*.lua
//
// callbacks (optional globals inside a script):
//   function on_tick()  end          -- every game tick (from CL_Move)
//   function on_paint() end          -- every rendered frame (from PaintTraverse)
//   function on_event(ev) end        -- game events (ev:get_name/get_int/get_float/get_bool/get_string)
//   function on_frame_stage(st) end  -- client_frame_stage.FRAME_* values
//   function on_menu() end           -- your script's settings UI in the SCRIPTS tab (ui.* elements)
//
// api:
// objects: ents.get(i)/ents.local_player() -> player, player:get_active_weapon() -> weapon,
//   player:get_animstate() -> animstate (goal_feet_yaw/current_feet_yaw/current_torso_yaw/
//   last_turn_time/feet_yaw_init/eye_yaw/eye_pitch), vector3/vector2/color classes (+ .new, __add...),
//   cvars.find("name") -> con_var (get_int/get_float/get_bool/get_name/set_value_*),
//   client_frame_stage.FRAME_* enum
// flat legacy api kept: ents.local/valid/alive/health/team/pos/flags/dormant/name
//   print(...) -> cheat console
//   cfg.get("name") / cfg.set("name", value)    -> cheat settings (see bind table)
//   draw.text(x, y, r, g, b, a, text [, center]) / draw.filled_rect / outlined_rect / line / circle
//   utils.tick() / utils.time()
namespace LuaAPI
{
	void Init( void );      // (re)load all scripts, called lazily
	void Shutdown( void );
	void Reload( void );

	void Tick( void );      // on_tick callbacks (game thread)
	void Paint( void );     // on_paint callbacks (render thread)
	void Event( void* eventPtr );   // on_event(event) - IGameEvent*
	void FrameStage( int stage );   // on_frame_stage(stage) - client_frame_stage values
	void Shot( int targetIndex );   // r23: on_shot(targetIndex) - fired when WE commit a live shot
	void CreateMove( void* cmdPtr ); // r25: on_create_move(cmd) - script cmd access (skipped in DT bursts)
	void Menu( int index );         // on_menu() of script #index - rendered inside the SCRIPTS tab
	bool ScriptHasMenu( int index );

	int  ScriptCount( void );
	const char* ScriptName( int i );
	const char* ScriptStatus( int i ); // "OK" or last error
}

#endif // __LUAAPI_H__
