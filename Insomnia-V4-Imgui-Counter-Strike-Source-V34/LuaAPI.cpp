// BUILD MARKER r35 (2026-09-18): audit fixes - all player-indexed arrays [64] -> [65] (OOB at entindex 64), lua print clamped.
// BUILD MARKER r33 (2026-09-18): menu schematic fix (FAKEDUCK block was under the DOUBLE TAP header) + lua 2.4 (cmd get/set_sendpacket, input.cursor).
// BUILD MARKER r25 (2026-09-18): on_create_move dispatch + interp-off (Segregation 0x36200) + BestDamage full-scan + spectator list + dual hitbox selectors (old base combo removed).
// BUILD MARKER r23 (2026-09-18): exact-seed hitchance (256-seed census + ForceSeed-aware) + ForceSeed prefers hit seeds + Lua: on_shot, draw.get_screen_size, utils.latency/choke, ents.eye_angles/hitbox.
#include "Main.h"
// BUILD MARKER r7 (ROLLBACK-tree, 2026-09-18): imgui include + CSWeapon* casts + circle segments + 24 named lua callbacks (zero lambdas).
// VERIFY: Ctrl+F "BUILD MARKER r7" must find this line. If not - you have a STALE file.
#include "LuaAPI.h"
#include "Lua/lua.hpp"
#include "ImGui/imgui.h" // ui.* callbacks draw ImGui widgets in the SCRIPTS tab

#include <vector>
#include <string>
#include <math.h>




// ---- script storage ----
struct LuaScript
{
	lua_State* L;
	std::string name;
	std::string status; // "OK" or last error (errored scripts stop running)
	bool hasTick;
	bool hasPaint;
	bool hasEvent;
	bool hasFrameStage;
	bool hasMenu;
	bool hasShot;
	bool hasCreateMove;
};
static std::vector<LuaScript> s_scripts;
static bool s_inited = false;
// one gate for every lua dispatch: game-thread (tick/event) and render-thread
// (paint/frame/menu) must never enter the same lua_State at once
static CRITICAL_SECTION s_luaCS;
static bool s_luaCSInit = false;
static void LuaLock( void ) { if( !s_luaCSInit ) { InitializeCriticalSection( &s_luaCS ); s_luaCSInit = true; } EnterCriticalSection( &s_luaCS ); }
static void LuaUnlock( void ) { LeaveCriticalSection( &s_luaCS ); }

// ---- sandbox: 5M instructions per callback, then the script is killed ----
static void CountHook( lua_State* L, lua_Debug* ar )
{
	if( ar->event == LUA_HOOKCOUNT )
		luaL_error( L, "script timeout (instruction limit exceeded)" );
}

static void Sandbox( lua_State* L )
{
	static const char* blocked[] = { "io", "os", "package", "debug", "dofile", "loadfile", "require" };
	for( int i = 0; i < ( int )( sizeof( blocked ) / sizeof( blocked[ 0 ] ) ); i++ )
	{
		lua_pushnil( L );
		lua_setglobal( L, blocked[ i ] );
	}
}

//=========================== value types ===========================

struct Vec3UD { float x, y, z; };
struct Vec2UD { float x, y; };
struct ColorUD { int r, g, b, a; };
struct PlayerUD { BasePlayer* ent; int idx; };
struct WeaponUD { BasePlayer* owner; }; // re-resolved on every call (never dangles)
struct AnimUD { BasePlayer* ent; int idx; };
struct EventUD { IGameEvent* ev; };
struct CvarUD { ConVar* cv; };

static Vec3UD* Vec3_check( lua_State* L, int i )
{
	return ( Vec3UD* )luaL_checkudata( L, i, "aw.vec3" );
}
static Vec2UD* Vec2_check( lua_State* L, int i )
{
	return ( Vec2UD* )luaL_checkudata( L, i, "aw.vec2" );
}
static ColorUD* Color_check( lua_State* L, int i )
{
	return ( ColorUD* )luaL_checkudata( L, i, "aw.color" );
}

// ---- vector3 ----
static int l_vec3_new( lua_State* L )
{
	Vec3UD* v = ( Vec3UD* )lua_newuserdata( L, sizeof( Vec3UD ) );
	v->x = ( float )luaL_optnumber( L, 1, 0 );
	v->y = ( float )luaL_optnumber( L, 2, 0 );
	v->z = ( float )luaL_optnumber( L, 3, 0 );
	luaL_getmetatable( L, "aw.vec3" );
	lua_setmetatable( L, -2 );
	return 1;
}
static int l_anon_6( lua_State* L )
{ Vec3UD* a = Vec3_check( L, 1 ); Vec3UD* b = Vec3_check( L, 2 ); lua_pushnumber( L, a->x * b->x + a->y * b->y + a->z * b->z ); return 1; }

static int l_anon_5( lua_State* L )
{ Vec3UD* a = Vec3_check( L, 1 ); Vec3UD* b = Vec3_check( L, 2 ); float dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z; lua_pushnumber( L, sqrtf( dx * dx + dy * dy + dz * dz ) ); return 1; }

static int l_anon_4( lua_State* L )
{ Vec3UD* a = Vec3_check( L, 1 ); lua_pushnumber( L, a->x * a->x + a->y * a->y ); return 1; }

static int l_anon_3( lua_State* L )
{ Vec3UD* a = Vec3_check( L, 1 ); lua_pushnumber( L, a->x * a->x + a->y * a->y + a->z * a->z ); return 1; }

static int l_anon_2( lua_State* L )
{ Vec3UD* a = Vec3_check( L, 1 ); lua_pushnumber( L, sqrtf( a->x * a->x + a->y * a->y ) ); return 1; }

static int l_anon_1( lua_State* L )
{ Vec3UD* a = Vec3_check( L, 1 ); lua_pushnumber( L, sqrtf( a->x * a->x + a->y * a->y + a->z * a->z ) ); return 1; }

static int l_vec3_index( lua_State* L )
{
	Vec3UD* v = Vec3_check( L, 1 );
	const char* k = luaL_checkstring( L, 2 );
	if( !strcmp( k, "x" ) ) { lua_pushnumber( L, v->x ); return 1; }
	if( !strcmp( k, "y" ) ) { lua_pushnumber( L, v->y ); return 1; }
	if( !strcmp( k, "z" ) ) { lua_pushnumber( L, v->z ); return 1; }

	if( !strcmp( k, "length" ) ) { lua_pushcfunction( L, l_anon_1); return 1; }
	if( !strcmp( k, "length_2d" ) ) { lua_pushcfunction( L, l_anon_2); return 1; }
	if( !strcmp( k, "length_sqr" ) ) { lua_pushcfunction( L, l_anon_3); return 1; }
	if( !strcmp( k, "length_2d_sqr" ) ) { lua_pushcfunction( L, l_anon_4); return 1; }
	if( !strcmp( k, "dist_to" ) ) { lua_pushcfunction( L, l_anon_5); return 1; }
	if( !strcmp( k, "dot" ) ) { lua_pushcfunction( L, l_anon_6); return 1; }
	return 0;
}
static int l_vec3_newindex( lua_State* L )
{
	Vec3UD* v = Vec3_check( L, 1 );
	const char* k = luaL_checkstring( L, 2 );
	float val = ( float )luaL_checknumber( L, 3 );
	if( !strcmp( k, "x" ) ) { v->x = val; return 0; }
	if( !strcmp( k, "y" ) ) { v->y = val; return 0; }
	if( !strcmp( k, "z" ) ) { v->z = val; return 0; }
	return luaL_error( L, "vec3: no field '%s'", k );
}
static int l_vec3_add( lua_State* L ) { Vec3UD* a = Vec3_check( L, 1 ); Vec3UD* b = Vec3_check( L, 2 ); Vec3UD* r = ( Vec3UD* )lua_newuserdata( L, sizeof( Vec3UD ) ); r->x = a->x + b->x; r->y = a->y + b->y; r->z = a->z + b->z; luaL_getmetatable( L, "aw.vec3" ); lua_setmetatable( L, -2 ); return 1; }
static int l_vec3_sub( lua_State* L ) { Vec3UD* a = Vec3_check( L, 1 ); Vec3UD* b = Vec3_check( L, 2 ); Vec3UD* r = ( Vec3UD* )lua_newuserdata( L, sizeof( Vec3UD ) ); r->x = a->x - b->x; r->y = a->y - b->y; r->z = a->z - b->z; luaL_getmetatable( L, "aw.vec3" ); lua_setmetatable( L, -2 ); return 1; }
static int l_vec3_mul( lua_State* L )
{
	Vec3UD* a = Vec3_check( L, 1 );
	float s = ( float )luaL_checknumber( L, 2 );
	Vec3UD* r = ( Vec3UD* )lua_newuserdata( L, sizeof( Vec3UD ) );
	r->x = a->x * s; r->y = a->y * s; r->z = a->z * s;
	luaL_getmetatable( L, "aw.vec3" );
	lua_setmetatable( L, -2 );
	return 1;
}
static int l_vec3_tostring( lua_State* L )
{
	Vec3UD* v = Vec3_check( L, 1 );
	lua_pushfstring( L, "(%.2f, %.2f, %.2f)", v->x, v->y, v->z );
	return 1;
}

// ---- vector2 ----
static int l_vec2_new( lua_State* L )
{
	Vec2UD* v = ( Vec2UD* )lua_newuserdata( L, sizeof( Vec2UD ) );
	v->x = ( float )luaL_optnumber( L, 1, 0 );
	v->y = ( float )luaL_optnumber( L, 2, 0 );
	luaL_getmetatable( L, "aw.vec2" );
	lua_setmetatable( L, -2 );
	return 1;
}
static int l_vec2_index( lua_State* L )
{
	Vec2UD* v = Vec2_check( L, 1 );
	const char* k = luaL_checkstring( L, 2 );
	if( !strcmp( k, "x" ) ) { lua_pushnumber( L, v->x ); return 1; }
	if( !strcmp( k, "y" ) ) { lua_pushnumber( L, v->y ); return 1; }
	return 0;
}
static int l_vec2_newindex( lua_State* L )
{
	Vec2UD* v = Vec2_check( L, 1 );
	const char* k = luaL_checkstring( L, 2 );
	float val = ( float )luaL_checknumber( L, 3 );
	if( !strcmp( k, "x" ) ) { v->x = val; return 0; }
	if( !strcmp( k, "y" ) ) { v->y = val; return 0; }
	return luaL_error( L, "vector2: no field '%s'", k );
}
static int l_vec2_tostring( lua_State* L )
{
	Vec2UD* v = Vec2_check( L, 1 );
	lua_pushfstring( L, "(%.2f, %.2f)", v->x, v->y );
	return 1;
}

// ---- color ----
static int l_color_new( lua_State* L )
{
	ColorUD* c = ( ColorUD* )lua_newuserdata( L, sizeof( ColorUD ) );
	c->r = ( int )luaL_optinteger( L, 1, 255 );
	c->g = ( int )luaL_optinteger( L, 2, 255 );
	c->b = ( int )luaL_optinteger( L, 3, 255 );
	c->a = ( int )luaL_optinteger( L, 4, 255 );
	luaL_getmetatable( L, "aw.color" );
	lua_setmetatable( L, -2 );
	return 1;
}
static int l_color_index( lua_State* L )
{
	ColorUD* c = Color_check( L, 1 );
	const char* k = luaL_checkstring( L, 2 );
	if( !strcmp( k, "r" ) ) { lua_pushinteger( L, c->r ); return 1; }
	if( !strcmp( k, "g" ) ) { lua_pushinteger( L, c->g ); return 1; }
	if( !strcmp( k, "b" ) ) { lua_pushinteger( L, c->b ); return 1; }
	if( !strcmp( k, "a" ) ) { lua_pushinteger( L, c->a ); return 1; }
	return 0;
}
static int l_color_newindex( lua_State* L )
{
	ColorUD* c = Color_check( L, 1 );
	const char* k = luaL_checkstring( L, 2 );
	int val = ( int )luaL_checkinteger( L, 3 );
	if( !strcmp( k, "r" ) ) { c->r = val; return 0; }
	if( !strcmp( k, "g" ) ) { c->g = val; return 0; }
	if( !strcmp( k, "b" ) ) { c->b = val; return 0; }
	if( !strcmp( k, "a" ) ) { c->a = val; return 0; }
	return luaL_error( L, "color: no field '%s'", k );
}
static int l_color_tostring( lua_State* L )
{
	ColorUD* c = Color_check( L, 1 );
	lua_pushfstring( L, "color(%d, %d, %d, %d)", c->r, c->g, c->b, c->a );
	return 1;
}

//=========================== cvar binds: cfg.get / cfg.set ===========================

enum { B_BOOL, B_INT, B_FLOAT };
struct CVarBind { const char* name; int type; void* ptr; };
static const CVarBind s_binds[ ] =
{
	{ "rage.active", B_BOOL, &g_CVars.Aimbot.Active },
	{ "rage.autoshoot", B_BOOL, &g_CVars.Aimbot.AutoShoot },
	{ "rage.silent", B_BOOL, &g_CVars.Aimbot.Silent },
	{ "rage.silent_perfect", B_BOOL, &g_CVars.Aimbot.PerfectSilent },
	{ "rage.hitscan", B_BOOL, &g_CVars.Aimbot.HitScan },
	{ "rage.autowall", B_BOOL, &g_CVars.Aimbot.AutoWall },
	{ "rage.autostop", B_BOOL, &g_CVars.Aimbot.AutoStop },
	{ "rage.bodyawp", B_BOOL, &g_CVars.Aimbot.BodyAWP },
	{ "rage.mindamage", B_INT, &g_CVars.Aimbot.MinDamage },
	{ "rage.fov", B_INT, &g_CVars.Aimbot.AimFOV },
	{ "rage.backtrack", B_INT, &g_CVars.Aimbot.BacktrackTicks },
	{ "rage.forcemindmg", B_INT, &g_CVars.Aimbot.ForceMinDmgValue },
	{ "resolver.enabled", B_BOOL, &g_CVars.Aimbot.Resolver.Active },
	{ "resolver.smart", B_BOOL, &g_CVars.Aimbot.Resolver.Smart },
	{ "resolver.mode", B_INT, &g_CVars.Aimbot.Resolver.Mode },
	{ "resolver.type", B_INT, &g_CVars.Aimbot.Resolver.Type },
	{ "aa.enabled", B_BOOL, &g_CVars.Miscellaneous.AntiAim.Active },
	{ "aa.pitch", B_INT, &g_CVars.Miscellaneous.AntiAim.Pitch },
	{ "aa.yaw", B_INT, &g_CVars.Miscellaneous.AntiAim.Yaw },
	{ "aa.yaw_mode", B_INT, &g_CVars.Miscellaneous.AntiAim.Variation },
	{ "aa.real", B_FLOAT, &g_CVars.Miscellaneous.AntiAim.RealValue },
	{ "aa.fake", B_FLOAT, &g_CVars.Miscellaneous.AntiAim.FakeValue },
	{ "aa.flick", B_BOOL, &g_CVars.Miscellaneous.AntiAim.FlickEnable },
	{ "aa.flick_every", B_INT, &g_CVars.Miscellaneous.AntiAim.FlickTicks },
	{ "aa.flick_angle", B_FLOAT, &g_CVars.Miscellaneous.AntiAim.FlickAngle },
	{ "aa.flick_random", B_BOOL, &g_CVars.Miscellaneous.AntiAim.FlickRandom },
	{ "aa.flick_on_shot", B_BOOL, &g_CVars.Miscellaneous.AntiAim.FlickOnShot },
	{ "aa.at_targets", B_BOOL, &g_CVars.Miscellaneous.AntiAim.AtTargets },
	{ "aa.enemy_check", B_BOOL, &g_CVars.Miscellaneous.AntiAim.TurnOff },
	{ "fakelag.enabled", B_BOOL, &g_CVars.Miscellaneous.Fakelag.Active },
	{ "fakelag.ticks", B_INT, &g_CVars.Miscellaneous.Fakelag.Value },
	{ "fakelag.mode", B_INT, &g_CVars.Miscellaneous.Fakelag.Mode },
	{ "fakelag.in_attack", B_BOOL, &g_CVars.Miscellaneous.Fakelag.InAttack },
	{ "fakelag.air_only", B_BOOL, &g_CVars.Miscellaneous.Fakelag.AirOnly },
	{ "dt.enabled", B_BOOL, &g_CVars.Miscellaneous.DoubleTap },
	{ "dt.ticks", B_INT, &g_CVars.Miscellaneous.DoubleTapTicks },
	{ "dt.auto", B_BOOL, &g_CVars.Miscellaneous.DoubleTapAuto },
	{ "dt.mode", B_INT, &g_CVars.Miscellaneous.DoubleTapMode },
	{ "dt.ground_only", B_BOOL, &g_CVars.Miscellaneous.DoubleTapOnlyGround },
	{ "dt.delay_shot", B_BOOL, &g_CVars.Miscellaneous.DoubleTapDelayShot },
	{ "legit.enabled", B_BOOL, &g_CVars.Legit.Active },
	{ "legit.autoshoot", B_BOOL, &g_CVars.Legit.AutoShoot },
	{ "legit.silent", B_BOOL, &g_CVars.Legit.Silent },
	{ "legit.aim_type", B_INT, &g_CVars.Legit.AimType },
	{ "legit.key", B_INT, &g_CVars.Legit.Key },
	{ "legit.fov", B_INT, &g_CVars.Legit.AimFOV },
	{ "legit.smoothing", B_INT, &g_CVars.Legit.Smoothing },
	{ "legit.reaction_ms", B_INT, &g_CVars.Legit.ReactionMs },
	{ "legit.rcs", B_INT, &g_CVars.Legit.RCS },
	{ "legit.backtrack", B_INT, &g_CVars.Legit.BacktrackTicks },
	{ "legit.scoped_check", B_BOOL, &g_CVars.Legit.ScopedCheck },
	{ "legit.auto_scope", B_BOOL, &g_CVars.Legit.AutoScope },
	{ "legit.strafe", B_BOOL, &g_CVars.Legit.StrafeActive },
	{ "legit.strafe_power", B_INT, &g_CVars.Legit.StrafePower },
	{ "indicators", B_BOOL, &g_CVars.Visuals.Indicators },
	{ "esp.box", B_BOOL, &g_CVars.Visuals.ESP.Box },
	{ "esp.name", B_BOOL, &g_CVars.Visuals.ESP.Name },
	{ "esp.health", B_BOOL, &g_CVars.Visuals.ESP.Health },
	{ "esp.weapon", B_BOOL, &g_CVars.Visuals.ESP.Weapon },
	{ "esp.bone", B_BOOL, &g_CVars.Visuals.ESP.Bone },
	{ "esp.enemy_only", B_BOOL, &g_CVars.Visuals.ESP.EnemyOnly },
	{ "nospread", B_BOOL, &g_CVars.Accuracy.PerfectAccuracy },
	{ "nospread.mode", B_INT, &g_CVars.Accuracy.NoSpreadMode },
	{ "forceseed", B_BOOL, &g_CVars.Accuracy.ForceSeed },
	{ "bhop", B_BOOL, &g_CVars.Miscellaneous.BunnyHop },
	{ "autostrafe", B_BOOL, &g_CVars.Miscellaneous.AutoStrafe },
	{ "autostrafe.mode", B_INT, &g_CVars.Miscellaneous.AutoStrafeMode }, // r38: 0 classic, 1 directional
	{ "autostrafe.avoid_dist", B_INT, &g_CVars.Miscellaneous.StrafeAvoidDist }, // r38
	{ "circlestrafe", B_BOOL, &g_CVars.Miscellaneous.CircleStrafe },
	{ "autoknife", B_BOOL, &g_CVars.Miscellaneous.AutoKnife },
	{ "speedhack", B_BOOL, &g_CVars.Miscellaneous.Speedhack },
	{ "speedhack.amount", B_INT, &g_CVars.Miscellaneous.SpeedhackValue },
	{ "slowwalk", B_BOOL, &g_CVars.Miscellaneous.SlowWalk },
	{ "slowwalk.speed", B_INT, &g_CVars.Miscellaneous.SlowWalkSpeed },
	{ "thirdperson_key", B_INT, &g_CVars.Miscellaneous.ThirdPersonKey },
	{ "thirdperson_dist", B_INT, &g_CVars.Miscellaneous.ThirdPersonDist },
};
static const int s_bindCount = ( int )( sizeof( s_binds ) / sizeof( s_binds[ 0 ] ) );

static const CVarBind* FindBind( const char* name )
{
	for( int i = 0; i < s_bindCount; i++ )
		if( !strcmp( s_binds[ i ].name, name ) ) return &s_binds[ i ];
	return nullptr;
}

static int l_cfg_get( lua_State* L )
{
	const char* n = luaL_checkstring( L, 1 );
	const CVarBind* b = FindBind( n );
	if( !b ) return luaL_error( L, "cfg.get: unknown setting '%s'", n );
	switch( b->type )
	{
		case B_BOOL: lua_pushboolean( L, *( bool* )b->ptr ? 1 : 0 ); break;
		case B_INT: lua_pushinteger( L, *( int* )b->ptr ); break;
		default: lua_pushnumber( L, *( float* )b->ptr ); break;
	}
	return 1;
}

static int l_cfg_set( lua_State* L )
{
	const char* n = luaL_checkstring( L, 1 );
	const CVarBind* b = FindBind( n );
	if( !b ) return luaL_error( L, "cfg.set: unknown setting '%s'", n );
	switch( b->type )
	{
		case B_BOOL: *( bool* )b->ptr = ( lua_toboolean( L, 2 ) != 0 ); break;
		case B_INT: *( int* )b->ptr = ( int )lua_tointeger( L, 2 ); break;
		default: *( float* )b->ptr = ( float )lua_tonumber( L, 2 ); break;
	}
	return 0;
}

//=========================== game_event ===========================

static int l_event_get_name( lua_State* L )
{
	EventUD* e = ( EventUD* )luaL_checkudata( L, 1, "aw.event" );
	lua_pushstring( L, e->ev ? e->ev->GetName( ) : "" );
	return 1;
}
static int l_event_get_bool( lua_State* L )
{
	EventUD* e = ( EventUD* )luaL_checkudata( L, 1, "aw.event" );
	lua_pushboolean( L, e->ev->GetBool( luaL_checkstring( L, 2 ), ( bool )luaL_optinteger( L, 3, 0 ) ) ? 1 : 0 );
	return 1;
}
static int l_event_get_int( lua_State* L )
{
	EventUD* e = ( EventUD* )luaL_checkudata( L, 1, "aw.event" );
	lua_pushinteger( L, e->ev->GetInt( luaL_checkstring( L, 2 ), ( int )luaL_optinteger( L, 3, -1 ) ) );
	return 1;
}
static int l_event_get_float( lua_State* L )
{
	EventUD* e = ( EventUD* )luaL_checkudata( L, 1, "aw.event" );
	lua_pushnumber( L, e->ev->GetFloat( luaL_checkstring( L, 2 ), ( float )luaL_optnumber( L, 3, -1 ) ) );
	return 1;
}
static int l_event_get_string( lua_State* L )
{
	EventUD* e = ( EventUD* )luaL_checkudata( L, 1, "aw.event" );
	const char* s = e->ev->GetString( luaL_checkstring( L, 2 ), "" );
	lua_pushstring( L, s ? s : "" );
	return 1;
}
static int l_event_tostring( lua_State* L )
{
	EventUD* e = ( EventUD* )luaL_checkudata( L, 1, "aw.event" );
	lua_pushstring( L, e->ev ? e->ev->GetName( ) : "event" );
	return 1;
}

//=========================== con_var ===========================

static int l_cvar_get_name( lua_State* L )
{
	CvarUD* c = ( CvarUD* )luaL_checkudata( L, 1, "aw.cvar" );
	lua_pushstring( L, c->cv->GetName( ) );
	return 1;
}
static int l_cvar_get_int( lua_State* L )
{
	CvarUD* c = ( CvarUD* )luaL_checkudata( L, 1, "aw.cvar" );
	lua_pushinteger( L, c->cv->GetInt( ) );
	return 1;
}
static int l_cvar_get_float( lua_State* L )
{
	CvarUD* c = ( CvarUD* )luaL_checkudata( L, 1, "aw.cvar" );
	lua_pushnumber( L, c->cv->GetFloat( ) );
	return 1;
}
static int l_cvar_get_bool( lua_State* L )
{
	CvarUD* c = ( CvarUD* )luaL_checkudata( L, 1, "aw.cvar" );
	lua_pushboolean( L, c->cv->GetBool( ) ? 1 : 0 );
	return 1;
}
static int l_cvar_set_int( lua_State* L )
{
	CvarUD* c = ( CvarUD* )luaL_checkudata( L, 1, "aw.cvar" );
	c->cv->SetValue( ( int )luaL_checkinteger( L, 2 ) );
	return 0;
}
static int l_cvar_set_float( lua_State* L )
{
	CvarUD* c = ( CvarUD* )luaL_checkudata( L, 1, "aw.cvar" );
	c->cv->SetValue( ( float )luaL_checknumber( L, 2 ) );
	return 0;
}
static int l_cvar_set_string( lua_State* L )
{
	CvarUD* c = ( CvarUD* )luaL_checkudata( L, 1, "aw.cvar" );
	c->cv->SetValue( luaL_checkstring( L, 2 ) );
	return 0;
}
static int l_cvars_find( lua_State* L )
{
	if( !g_pCvar ) { lua_pushnil( L ); return 1; }
	ConVar* cv = g_pCvar->FindVar( luaL_checkstring( L, 1 ) );
	if( !cv ) { lua_pushnil( L ); return 1; }
	CvarUD* c = ( CvarUD* )lua_newuserdata( L, sizeof( CvarUD ) );
	c->cv = cv;
	luaL_getmetatable( L, "aw.cvar" );
	lua_setmetatable( L, -2 );
	return 1;
}

//=========================== animstate (computed live view) ===========================

// v34 server facts (see FrameStageNotify resolver): feet yaw = m_angRotation.y,
// eyes converge to it. We expose the real server values + locally tracked turn info.
static float s_asBaseline[ 65 ] = { 0.f };  // smoothed feet baseline
static bool  s_asInit[ 65 ] = { false };
static float s_asFirstFeet[ 65 ] = { 0.f };
static float s_asLastTurnMs[ 65 ] = { 0.f };

static void AnimSense( int idx, BasePlayer* e )
{
	if( idx <= 0 || idx > 64 || !e ) return;
	float feet = e->m_angRotation( ).y;
	if( !s_asInit[ idx ] )
	{
		s_asBaseline[ idx ] = feet;
		s_asFirstFeet[ idx ] = feet;
		s_asInit[ idx ] = true;
		return;
	}
	float delta = g_Stuff.GuwopNormalize( feet - s_asBaseline[ idx ] );
	float adelta = ( delta < 0.f ) ? -delta : delta;
	if( adelta > 10.f ) s_asLastTurnMs[ idx ] = ( float )GetTickCount( );
	if( adelta > 40.f ) s_asBaseline[ idx ] = feet; // server snapped (45-degree step): jump instantly
	else s_asBaseline[ idx ] = g_Stuff.GuwopNormalize( s_asBaseline[ idx ] + delta * 0.3f );
}

static int l_anim_index( lua_State* L )
{
	AnimUD* a = ( AnimUD* )luaL_checkudata( L, 1, "aw.animstate" );
	const char* k = luaL_checkstring( L, 2 );
	if( !a->ent ) return luaL_error( L, "animstate: entity is gone" );
	AnimSense( a->idx, a->ent );
	float feet = a->ent->m_angRotation( ).y;
	QAngle eye = a->ent->m_angEyeAngles( );
	if( !strcmp( k, "goal_feet_yaw" ) ) { lua_pushnumber( L, feet ); return 1; }
	if( !strcmp( k, "current_feet_yaw" ) ) { lua_pushnumber( L, s_asBaseline[ a->idx ] ); return 1; }
	if( !strcmp( k, "current_torso_yaw" ) )
	{
		float t = g_Stuff.GuwopNormalize( eye.y - feet );
		if( t > 45.f ) t = 45.f; if( t < -45.f ) t = -45.f;
		lua_pushnumber( L, t );
		return 1;
	}
	if( !strcmp( k, "last_turn_time" ) ) { lua_pushnumber( L, s_asLastTurnMs[ a->idx ] ); return 1; }
	if( !strcmp( k, "feet_yaw_init" ) ) { lua_pushnumber( L, s_asFirstFeet[ a->idx ] ); return 1; }
	if( !strcmp( k, "eye_yaw" ) ) { lua_pushnumber( L, eye.y ); return 1; }
	if( !strcmp( k, "eye_pitch" ) ) { lua_pushnumber( L, eye.x ); return 1; }
	return 0;
}
static int l_anim_newindex( lua_State* L )
{
	luaL_checkudata( L, 1, "aw.animstate" );
	return luaL_error( L, "animstate is read-only" );
}

//=========================== player / weapon ===========================

static BasePlayer* PlayerEnt( lua_State* L )
{
	PlayerUD* p = ( PlayerUD* )luaL_checkudata( L, 1, "aw.player" );
	if( p->idx <= 0 || p->idx > 64 ) return nullptr;
	return ( BasePlayer* ) g_pClientEntityList->GetClientEntity( p->idx );
}

static int l_anon_22( lua_State* L )
{ PlayerUD* p = ( PlayerUD* )luaL_checkudata( L, 1, "aw.player" ); WeaponUD* w = ( WeaponUD* )lua_newuserdata( L, sizeof( WeaponUD ) ); w->owner = p->ent; luaL_getmetatable( L, "aw.weapon" ); lua_setmetatable( L, -2 ); return 1; }

static int l_anon_21( lua_State* L )
{ PlayerUD* p = ( PlayerUD* )luaL_checkudata( L, 1, "aw.player" ); AnimUD* a = ( AnimUD* )lua_newuserdata( L, sizeof( AnimUD ) ); a->ent = p->ent; a->idx = p->idx; luaL_getmetatable( L, "aw.animstate" ); lua_setmetatable( L, -2 ); return 1; }

static int l_anon_20( lua_State* L )
{ BasePlayer* e = PlayerEnt( L ); int i = ( int )luaL_checkinteger( L, 2 ); float v = ( float )luaL_checknumber( L, 3 ); if( e && i >= 0 && i <= 23 ) *( float* )( ( DWORD_PTR )e + 0x518 + 4 * i ) = v; return 0; }

static int l_anon_19( lua_State* L )
{ BasePlayer* e = PlayerEnt( L ); int i = ( int )luaL_checkinteger( L, 2 ); if( !e || i < 0 || i > 23 ) { lua_pushnumber( L, 0 ); return 1; } lua_pushnumber( L, *( float* )( ( DWORD_PTR )e + 0x518 + 4 * i ) ); return 1; }

static int l_anon_18( lua_State* L )
{ PlayerUD* p = ( PlayerUD* )luaL_checkudata( L, 1, "aw.player" ); player_info_t info; if( p->idx <= 0 || p->idx > 64 || !g_pEngineClient->GetPlayerInfo( p->idx, &info ) ) { lua_pushstring( L, "" ); return 1; } lua_pushstring( L, info.name ); return 1; }

static int l_anon_17( lua_State* L )
{
			BasePlayer* e = PlayerEnt( L );
			int mt = 1; // stand
			if( e )
			{
				Vector v = e->GetVelocity( );
				float spd = sqrtf( v.x * v.x + v.y * v.y );
				bool ground = ( e->m_fFlags( ) & FL_ONGROUND ) != 0;
				if( !ground ) mt = 3;             // air
				else if( spd > 10.f ) mt = 2;     // walk
			}
			lua_pushinteger( L, mt );
			return 1;
		}

static int l_anon_16( lua_State* L )
{ BasePlayer* e = PlayerEnt( L ); QAngle a = e ? e->m_angEyeAngles( ) : QAngle( 0, 0, 0 ); Vec3UD* r = ( Vec3UD* )lua_newuserdata( L, sizeof( Vec3UD ) ); r->x = a.x; r->y = a.y; r->z = a.z; luaL_getmetatable( L, "aw.vec3" ); lua_setmetatable( L, -2 ); return 1; }

static int l_anon_15( lua_State* L )
{ BasePlayer* e = PlayerEnt( L ); Vector v = e ? e->m_vecOrigin( ) : Vector( 0, 0, 0 ); Vec3UD* r = ( Vec3UD* )lua_newuserdata( L, sizeof( Vec3UD ) ); r->x = v.x; r->y = v.y; r->z = v.z; luaL_getmetatable( L, "aw.vec3" ); lua_setmetatable( L, -2 ); return 1; }

static int l_anon_14( lua_State* L )
{ BasePlayer* e = PlayerEnt( L ); Vector v = e ? e->GetVelocity( ) : Vector( 0, 0, 0 ); Vec3UD* r = ( Vec3UD* )lua_newuserdata( L, sizeof( Vec3UD ) ); r->x = v.x; r->y = v.y; r->z = v.z; luaL_getmetatable( L, "aw.vec3" ); lua_setmetatable( L, -2 ); return 1; }

static int l_anon_13( lua_State* L )
{ BasePlayer* e = PlayerEnt( L ); lua_pushinteger( L, e ? e->m_iTeamNum( ) : 0 ); return 1; }

static int l_anon_12( lua_State* L )
{ BasePlayer* e = PlayerEnt( L ); lua_pushinteger( L, e ? e->m_iHealth( ) : 0 ); return 1; }

static int l_anon_11( lua_State* L )
{ BasePlayer* e = PlayerEnt( L ); int fov = e ? *( int* )( ( DWORD_PTR )e + 0xD1C ) : 0; lua_pushboolean( L, ( fov != 0 && fov != 90 ) ? 1 : 0 ); return 1; }

static int l_anon_10( lua_State* L )
{ BasePlayer* e = PlayerEnt( L ); lua_pushboolean( L, e && e->m_bHasHelmet( ) ? 1 : 0 ); return 1; }

static int l_anon_9( lua_State* L )
{ BasePlayer* e = PlayerEnt( L ); lua_pushboolean( L, e && e->m_lifeState( ) == 0 ? 1 : 0 ); return 1; }

static int l_anon_8( lua_State* L )
{ BasePlayer* e = PlayerEnt( L ); lua_pushboolean( L, e && e->IsDormant( ) ? 1 : 0 ); return 1; }

static int l_anon_7( lua_State* L )
{ PlayerUD* p = ( PlayerUD* )luaL_checkudata( L, 1, "aw.player" ); lua_pushinteger( L, p->idx ); return 1; }

static int l_pl_index( lua_State* L )
{
	PlayerUD* p = ( PlayerUD* )luaL_checkudata( L, 1, "aw.player" );
	const char* k = luaL_checkstring( L, 2 );
	if( !strcmp( k, "index" ) ) { lua_pushcfunction( L, l_anon_7); return 1; }
	if( !strcmp( k, "is_dormant" ) ) { lua_pushcfunction( L, l_anon_8); return 1; }
	if( !strcmp( k, "is_alive" ) ) { lua_pushcfunction( L, l_anon_9); return 1; }
	if( !strcmp( k, "has_helmet" ) ) { lua_pushcfunction( L, l_anon_10); return 1; }
	if( !strcmp( k, "is_scoped" ) ) { lua_pushcfunction( L, l_anon_11); return 1; }
	if( !strcmp( k, "get_health" ) ) { lua_pushcfunction( L, l_anon_12); return 1; }
	if( !strcmp( k, "get_team" ) ) { lua_pushcfunction( L, l_anon_13); return 1; }
	if( !strcmp( k, "get_velocity" ) ) { lua_pushcfunction( L, l_anon_14); return 1; }
	if( !strcmp( k, "get_origin" ) ) { lua_pushcfunction( L, l_anon_15); return 1; }
	if( !strcmp( k, "get_angles" ) ) { lua_pushcfunction( L, l_anon_16); return 1; }
	if( !strcmp( k, "get_movetype" ) )
	{
		lua_pushcfunction( L, l_anon_17);
		return 1;
	}
	if( !strcmp( k, "get_name" ) ) { lua_pushcfunction( L, l_anon_18); return 1; }
	if( !strcmp( k, "get_poseparam" ) ) { lua_pushcfunction( L, l_anon_19); return 1; }
	if( !strcmp( k, "set_poseparam" ) ) { lua_pushcfunction( L, l_anon_20); return 1; }
	if( !strcmp( k, "get_animstate" ) ) { lua_pushcfunction( L, l_anon_21); return 1; }
	if( !strcmp( k, "get_active_weapon" ) ) { lua_pushcfunction( L, l_anon_22); return 1; }
	return 0;
}

static int l_pl_tostring( lua_State* L )
{
	PlayerUD* p = ( PlayerUD* )luaL_checkudata( L, 1, "aw.player" );
	lua_pushfstring( L, "player(%d)", p->idx );
	return 1;
}

static CSWeapon* WeaponEnt( lua_State* L )
{
	WeaponUD* w = ( WeaponUD* )luaL_checkudata( L, 1, "aw.weapon" );
	if( !w->owner || w->owner->m_lifeState( ) != 0 ) return nullptr;
	return ( CSWeapon* )w->owner->GetActiveBaseCombatWeapon( );
}

static int l_anon_24( lua_State* L )
{ CSWeapon* w = WeaponEnt( L ); lua_pushinteger( L, w ? w->m_iClip1( ) : -1 ); return 1; }

static int l_anon_23( lua_State* L )
{ CSWeapon* w = WeaponEnt( L ); lua_pushnumber( L, w ? w->m_flNextPrimaryAttack( ) : 0.f ); return 1; }

static int l_wep_index( lua_State* L )
{
	const char* k = luaL_checkstring( L, 2 );
	if( !strcmp( k, "get_next_attack" ) ) { lua_pushcfunction( L, l_anon_23); return 1; }
	if( !strcmp( k, "get_clip1" ) ) { lua_pushcfunction( L, l_anon_24); return 1; }
	return 0;
}
static int l_wep_tostring( lua_State* L ) { lua_pushstring( L, "weapon" ); return 1; }

//=========================== drawing / utils / print ===========================

static int l_draw_text( lua_State* L )
{
	int x = ( int )luaL_checkinteger( L, 1 );
	int y = ( int )luaL_checkinteger( L, 2 );
	int r = ( int )luaL_checkinteger( L, 3 );
	int g = ( int )luaL_checkinteger( L, 4 );
	int b = ( int )luaL_checkinteger( L, 5 );
	int a = ( int )luaL_optinteger( L, 6, 255 );
	const char* text = luaL_checkstring( L, 7 );
	bool center = ( lua_toboolean( L, 8 ) != 0 );
	g_Drawing.String( center, x, y, Color( r, g, b, a ), "%s", text );
	return 0;
}

static int l_draw_filled_rect( lua_State* L )
{
	g_Drawing.FilledRect( ( int )luaL_checkinteger( L, 1 ), ( int )luaL_checkinteger( L, 2 ),
		( int )luaL_checkinteger( L, 3 ), ( int )luaL_checkinteger( L, 4 ),
		Color( ( int )luaL_checkinteger( L, 5 ), ( int )luaL_checkinteger( L, 6 ), ( int )luaL_checkinteger( L, 7 ), ( int )luaL_optinteger( L, 8, 255 ) ) );
	return 0;
}

static int l_draw_outlined_rect( lua_State* L )
{
	g_Drawing.OutlinedRect( ( int )luaL_checkinteger( L, 1 ), ( int )luaL_checkinteger( L, 2 ),
		( int )luaL_checkinteger( L, 3 ), ( int )luaL_checkinteger( L, 4 ),
		Color( ( int )luaL_checkinteger( L, 5 ), ( int )luaL_checkinteger( L, 6 ), ( int )luaL_checkinteger( L, 7 ), ( int )luaL_optinteger( L, 8, 255 ) ) );
	return 0;
}

static int l_draw_line( lua_State* L )
{
	g_Drawing.Line( ( int )luaL_checkinteger( L, 1 ), ( int )luaL_checkinteger( L, 2 ),
		( int )luaL_checkinteger( L, 3 ), ( int )luaL_checkinteger( L, 4 ),
		Color( ( int )luaL_checkinteger( L, 5 ), ( int )luaL_checkinteger( L, 6 ), ( int )luaL_checkinteger( L, 7 ), ( int )luaL_optinteger( L, 8, 255 ) ) );
	return 0;
}

static int l_draw_circle( lua_State* L )
{
	g_Drawing.Circle( ( int )luaL_checkinteger( L, 1 ), ( int )luaL_checkinteger( L, 2 ),
		( int )luaL_checkinteger( L, 3 ), ( int )luaL_optinteger( L, 8, 100 ),
		Color( ( int )luaL_checkinteger( L, 4 ), ( int )luaL_checkinteger( L, 5 ), ( int )luaL_checkinteger( L, 6 ), ( int )luaL_optinteger( L, 7, 255 ) ) );
	return 0;
}

static int l_utils_tick( lua_State* L ) { lua_pushinteger( L, g_iGameTicks ); return 1; }
static int l_utils_time( lua_State* L ) { lua_pushinteger( L, ( lua_Integer )GetTickCount( ) ); return 1; }

static int l_print( lua_State* L )
{
	std::string out;
	int n = lua_gettop( L );
	for( int i = 1; i <= n; i++ )
	{
		if( i > 1 ) out += "  ";
		out += lua_tostring( L, i ) ? lua_tostring( L, i ) : luaL_tolstring( L, i, nullptr );
		if( !lua_tostring( L, i ) ) lua_pop( L, 1 );
	}
	if( out.size( ) > 1000 ) { out.resize( 1000 ); out += " ..."; } // r35: vsprintf_s(2048) safety
	printconsole( "[lua] %s\n", out.c_str( ) );
	return 0;
}

// ---- ents (flat legacy API + object API) ----
static int l_ents_get( lua_State* L )
{
	int idx = ( int )luaL_checkinteger( L, 1 );
	if( idx <= 0 || idx > 64 || !g_pEngineClient || !g_pEngineClient->IsInGame( ) ) { lua_pushnil( L ); return 1; }
	BasePlayer* e = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( idx );
	if( !e ) { lua_pushnil( L ); return 1; }
	PlayerUD* p = ( PlayerUD* )lua_newuserdata( L, sizeof( PlayerUD ) );
	p->ent = e; p->idx = idx;
	luaL_getmetatable( L, "aw.player" );
	lua_setmetatable( L, -2 );
	return 1;
}

static int l_ents_local_player( lua_State* L )
{
	if( !g_pEngineClient || !g_pEngineClient->IsInGame( ) ) { lua_pushnil( L ); return 1; }
	int idx = g_pEngineClient->GetLocalPlayer( );
	BasePlayer* e = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( idx );
	if( !e ) { lua_pushnil( L ); return 1; }
	PlayerUD* p = ( PlayerUD* )lua_newuserdata( L, sizeof( PlayerUD ) );
	p->ent = e; p->idx = idx;
	luaL_getmetatable( L, "aw.player" );
	lua_setmetatable( L, -2 );
	return 1;
}

static int l_ents_local( lua_State* L )
{
	if( !g_pEngineClient || !g_pEngineClient->IsInGame( ) ) { lua_pushnil( L ); return 1; }
	lua_pushinteger( L, g_pEngineClient->GetLocalPlayer( ) );
	return 1;
}

static int l_ents_valid( lua_State* L )
{
	int idx = ( int )luaL_checkinteger( L, 1 );
	BasePlayer* e = ( idx > 0 && idx <= 64 ) ? ( BasePlayer* ) g_pClientEntityList->GetClientEntity( idx ) : nullptr;
	lua_pushboolean( L, e && !e->IsDormant( ) ? 1 : 0 );
	return 1;
}

static int l_ents_alive( lua_State* L )
{
	int idx = ( int )luaL_checkinteger( L, 1 );
	BasePlayer* e = ( idx > 0 && idx <= 64 ) ? ( BasePlayer* ) g_pClientEntityList->GetClientEntity( idx ) : nullptr;
	lua_pushboolean( L, e && e->m_lifeState( ) == 0 ? 1 : 0 );
	return 1;
}

static int l_ents_health( lua_State* L )
{
	int idx = ( int )luaL_checkinteger( L, 1 );
	BasePlayer* e = ( idx > 0 && idx <= 64 ) ? ( BasePlayer* ) g_pClientEntityList->GetClientEntity( idx ) : nullptr;
	lua_pushinteger( L, e ? e->m_iHealth( ) : 0 );
	return 1;
}

static int l_ents_team( lua_State* L )
{
	int idx = ( int )luaL_checkinteger( L, 1 );
	BasePlayer* e = ( idx > 0 && idx <= 64 ) ? ( BasePlayer* ) g_pClientEntityList->GetClientEntity( idx ) : nullptr;
	lua_pushinteger( L, e ? e->m_iTeamNum( ) : 0 );
	return 1;
}

static int l_ents_pos( lua_State* L )
{
	int idx = ( int )luaL_checkinteger( L, 1 );
	BasePlayer* e = ( idx > 0 && idx <= 64 ) ? ( BasePlayer* ) g_pClientEntityList->GetClientEntity( idx ) : nullptr;
	if( !e ) { lua_pushnil( L ); lua_pushnil( L ); lua_pushnil( L ); return 3; }
	Vector p = e->m_vecOrigin( );
	lua_pushnumber( L, p.x ); lua_pushnumber( L, p.y ); lua_pushnumber( L, p.z );
	return 3;
}

static int l_ents_flags( lua_State* L )
{
	int idx = ( int )luaL_checkinteger( L, 1 );
	BasePlayer* e = ( idx > 0 && idx <= 64 ) ? ( BasePlayer* ) g_pClientEntityList->GetClientEntity( idx ) : nullptr;
	lua_pushinteger( L, e ? e->m_fFlags( ) : 0 );
	return 1;
}

static int l_ents_dormant( lua_State* L )
{
	int idx = ( int )luaL_checkinteger( L, 1 );
	BasePlayer* e = ( idx > 0 && idx <= 64 ) ? ( BasePlayer* ) g_pClientEntityList->GetClientEntity( idx ) : nullptr;
	lua_pushboolean( L, e && e->IsDormant( ) ? 1 : 0 );
	return 1;
}

static int l_ents_name( lua_State* L )
{
	int idx = ( int )luaL_checkinteger( L, 1 );
	player_info_t info;
	if( idx <= 0 || idx > 64 || !g_pEngineClient->GetPlayerInfo( idx, &info ) ) { lua_pushstring( L, "" ); return 1; }
	lua_pushstring( L, info.name );
	return 1;
}

//=========================== ui: script menus (SCRIPTS tab) ===========================

// per-script element state lives in each lua_State registry, keyed by element label
static const char s_uiKeyHolder = 0;

static void UiTable( lua_State* L )
{
	lua_pushlightuserdata( L, ( void* )&s_uiKeyHolder );
	lua_gettable( L, LUA_REGISTRYINDEX );
	if( !lua_istable( L, -1 ) )
	{
		lua_pop( L, 1 );
		lua_newtable( L );
		lua_pushlightuserdata( L, ( void* )&s_uiKeyHolder );
		lua_pushvalue( L, -2 );
		lua_settable( L, LUA_REGISTRYINDEX );
	}
}

static int l_ui_checkbox( lua_State* L )
{
	const char* label = luaL_checkstring( L, 1 );
	bool v = ( lua_toboolean( L, 2 ) != 0 );
	UiTable( L );
	lua_pushstring( L, label ); lua_gettable( L, -2 );
	if( lua_isboolean( L, -1 ) ) v = ( lua_toboolean( L, -1 ) != 0 );
	lua_pop( L, 1 );
	ImGui::Checkbox( label, &v );
	lua_pushstring( L, label ); lua_pushboolean( L, v ? 1 : 0 ); lua_settable( L, -3 );
	lua_pushboolean( L, v ? 1 : 0 );
	return 1;
}

static int l_ui_slider_int( lua_State* L )
{
	const char* label = luaL_checkstring( L, 1 );
	int mn = ( int )luaL_checkinteger( L, 2 );
	int mx = ( int )luaL_checkinteger( L, 3 );
	int v = ( int )luaL_optinteger( L, 4, mn );
	UiTable( L );
	lua_pushstring( L, label ); lua_gettable( L, -2 );
	if( lua_isnumber( L, -1 ) ) v = ( int )lua_tointeger( L, -1 );
	lua_pop( L, 1 );
	ImGui::SliderInt( label, &v, mn, mx );
	lua_pushstring( L, label ); lua_pushinteger( L, v ); lua_settable( L, -3 );
	lua_pushinteger( L, v );
	return 1;
}

static int l_ui_slider_float( lua_State* L )
{
	const char* label = luaL_checkstring( L, 1 );
	float mn = ( float )luaL_checknumber( L, 2 );
	float mx = ( float )luaL_checknumber( L, 3 );
	float v = ( float )luaL_optnumber( L, 4, mn );
	UiTable( L );
	lua_pushstring( L, label ); lua_gettable( L, -2 );
	if( lua_isnumber( L, -1 ) ) v = ( float )lua_tonumber( L, -1 );
	lua_pop( L, 1 );
	ImGui::SliderFloat( label, &v, mn, mx, "%.2f" );
	lua_pushstring( L, label ); lua_pushnumber( L, v ); lua_settable( L, -3 );
	lua_pushnumber( L, v );
	return 1;
}

static int l_ui_combo( lua_State* L )
{
	const char* label = luaL_checkstring( L, 1 );
	luaL_checktype( L, 2, LUA_TTABLE );
	int n = ( int )lua_rawlen( L, 2 );
	if( n <= 0 ) return luaL_error( L, "ui.combo: items table is empty" );
	std::vector<std::string> items;
	for( int i = 1; i <= n; i++ )
	{
		lua_rawgeti( L, 2, i );
		items.push_back( lua_tostring( L, -1 ) ? lua_tostring( L, -1 ) : "?" );
		lua_pop( L, 1 );
	}
	int v = ( int )luaL_optinteger( L, 3, 0 );
	UiTable( L );
	lua_pushstring( L, label ); lua_gettable( L, -2 );
	if( lua_isnumber( L, -1 ) )
	{
		int t = ( int )lua_tointeger( L, -1 );
		if( t >= 0 && t < n ) v = t;
	}
	lua_pop( L, 1 );
	if( v < 0 || v >= n ) v = 0;
	std::vector<const char*> ptrs;
	for( size_t i = 0; i < items.size( ); i++ ) ptrs.push_back( items[ i ].c_str( ) );
	ImGui::Combo( label, &v, ptrs.data( ), n );
	lua_pushstring( L, label ); lua_pushinteger( L, v ); lua_settable( L, -3 );
	lua_pushinteger( L, v );
	return 1;
}

static int l_ui_button( lua_State* L )
{
	const char* label = luaL_checkstring( L, 1 );
	lua_pushboolean( L, ImGui::Button( label ) ? 1 : 0 );
	return 1;
}

static int l_ui_text( lua_State* L )
{
	ImGui::TextUnformatted( luaL_checkstring( L, 1 ) );
	return 0;
}

static int l_ui_header( lua_State* L )
{
	ImGui::TextColored( ImVec4( 0.702f, 0.180f, 0.180f, 1.00f ), "%s", luaL_checkstring( L, 1 ) );
	ImGui::Separator( );
	return 0;
}

static int l_ui_separator( lua_State* L ) { ImGui::Separator( ); return 0; }
static int l_ui_same_line( lua_State* L ) { ImGui::SameLine( ); return 0; }

static int l_ui_get( lua_State* L )
{
	const char* label = luaL_checkstring( L, 1 );
	UiTable( L );
	lua_pushstring( L, label ); lua_gettable( L, -2 );
	if( lua_isnoneornil( L, -1 ) )
	{
		lua_pop( L, 1 );
		lua_pushvalue( L, 2 ); // default
	}
	return 1;
}

static int l_ui_set( lua_State* L )
{
	const char* label = luaL_checkstring( L, 1 );
	UiTable( L );
	lua_pushstring( L, label );
	lua_pushvalue( L, 2 );
	lua_settable( L, -3 );
	return 0;
}

//=========================== input / extra utils ===========================

static int l_input_is_down( lua_State* L )
{
	int vk = ( int )luaL_checkinteger( L, 1 );
	if( vk <= 0 || vk > 255 ) { lua_pushboolean( L, 0 ); return 1; }
	lua_pushboolean( L, ( GetAsyncKeyState( vk ) & 0x8000 ) ? 1 : 0 );
	return 1;
}

static int l_utils_curtime( lua_State* L ) { lua_pushnumber( L, g_pGlobals ? g_pGlobals->curtime : 0.f ); return 1; }
static int l_utils_frametime( lua_State* L ) { lua_pushnumber( L, g_pGlobals ? g_pGlobals->frametime : 0.f ); return 1; }
static int l_utils_ipt( lua_State* L ) { lua_pushnumber( L, g_pGlobals ? g_pGlobals->interval_per_tick : 0.f ); return 1; }

static int l_cfg_list( lua_State* L )
{
	lua_newtable( L );
	for( int i = 0; i < s_bindCount; i++ )
	{
		lua_pushstring( L, s_binds[ i ].name );
		lua_rawseti( L, -2, i + 1 );
	}
	return 1;
}

//=========================== ents.get_all / draw.world_to_screen ===========================

static int l_ents_get_all( lua_State* L )
{
	lua_newtable( L );
	int out = 0;
	if( !g_pEngineClient || !g_pEngineClient->IsInGame( ) ) return 1;
	for( int i = 1; i <= 64; i++ )
	{
		BasePlayer* e = ( BasePlayer* ) g_pClientEntityList->GetClientEntity( i );
		if( !e ) continue;
		PlayerUD* p = ( PlayerUD* )lua_newuserdata( L, sizeof( PlayerUD ) );
		p->ent = e; p->idx = i;
		luaL_getmetatable( L, "aw.player" );
		lua_setmetatable( L, -2 );
		lua_rawseti( L, -2, ++out );
	}
	return 1;
}

static int l_draw_w2s( lua_State* L )
{
	Vec3UD* w = Vec3_check( L, 1 );
	Vector world( w->x, w->y, w->z ), screen;
	if( !g_Stuff.WorldToScreen( world, screen ) ) { lua_pushboolean( L, 0 ); return 1; }
	lua_pushboolean( L, 1 );
	lua_pushnumber( L, screen.x );
	lua_pushnumber( L, screen.y );
	return 3;
}

//=========================== registration ===========================

// r47: the r23-r33 lua function definitions live in namespace LuaAPI (see below);
// declare them here so the registration code can reference LuaAPI::name.
namespace LuaAPI
{
	static int l_utils_latency( lua_State* L );
	static int l_utils_choke( lua_State* L );
	static int l_input_cursor( lua_State* L );
	static int l_ents_hitbox( lua_State* L );
	static int l_ents_eye_angles( lua_State* L );
	static int l_draw_screen_size( lua_State* L );
	static int l_cmd_tick( lua_State* L );
	static int l_cmd_set_viewangles( lua_State* L );
	static int l_cmd_get_viewangles( lua_State* L );
	static int l_cmd_set_move( lua_State* L );
	static int l_cmd_get_move( lua_State* L );
	static int l_cmd_set_buttons( lua_State* L );
	static int l_cmd_get_buttons( lua_State* L );
	static int l_cmd_set_sendpacket( lua_State* L );
	static int l_cmd_get_sendpacket( lua_State* L );
}

static void RegisterAPI( lua_State* L )
{
	// print -> cheat console
	lua_register( L, "print", l_print );

	// value type metatables
	struct MtReg { const char* name; const luaL_Reg* regs; };
	static const luaL_Reg vec3Regs[ ] =
	{
		{ "__index", l_vec3_index }, { "__newindex", l_vec3_newindex },
		{ "__add", l_vec3_add }, { "__sub", l_vec3_sub }, { "__mul", l_vec3_mul },
		{ "__tostring", l_vec3_tostring }, { nullptr, nullptr }
	};
	static const luaL_Reg vec2Regs[ ] =
	{
		{ "__index", l_vec2_index }, { "__newindex", l_vec2_newindex },
		{ "__tostring", l_vec2_tostring }, { nullptr, nullptr }
	};
	static const luaL_Reg colorRegs[ ] =
	{
		{ "__index", l_color_index }, { "__newindex", l_color_newindex },
		{ "__tostring", l_color_tostring }, { nullptr, nullptr }
	};
	static const luaL_Reg playerRegs[ ] =
	{
		{ "__index", l_pl_index }, { "__tostring", l_pl_tostring }, { nullptr, nullptr }
	};
	static const luaL_Reg wepRegs[ ] =
	{
		{ "__index", l_wep_index }, { "__tostring", l_wep_tostring }, { nullptr, nullptr }
	};
	static const luaL_Reg animRegs[ ] =
	{
		{ "__index", l_anim_index }, { "__newindex", l_anim_newindex }, { nullptr, nullptr }
	};
	static const luaL_Reg eventRegs[ ] =
	{
		{ "get_name", l_event_get_name }, { "get_bool", l_event_get_bool },
		{ "get_int", l_event_get_int }, { "get_float", l_event_get_float },
		{ "get_string", l_event_get_string },
		{ "__index", l_event_get_name }, { "__tostring", l_event_tostring }, { nullptr, nullptr }
	};
	static const luaL_Reg cvarRegs[ ] =
	{
		{ "get_name", l_cvar_get_name }, { "get_int", l_cvar_get_int },
		{ "get_float", l_cvar_get_float }, { "get_bool", l_cvar_get_bool },
		{ "set_value_int", l_cvar_set_int }, { "set_value_float", l_cvar_set_float },
		{ "set_value_string", l_cvar_set_string },
		{ "__index", l_cvar_get_name }, { "__tostring", l_cvar_get_name }, { nullptr, nullptr }
	};

	const MtReg mts[ ] =
	{
		{ "aw.vec3", vec3Regs }, { "aw.vec2", vec2Regs }, { "aw.color", colorRegs },
		{ "aw.player", playerRegs }, { "aw.weapon", wepRegs },
		{ "aw.animstate", animRegs }, { "aw.event", eventRegs }, { "aw.cvar", cvarRegs },
	};
	for( int i = 0; i < ( int )( sizeof( mts ) / sizeof( mts[ 0 ] ) ); i++ )
	{
		luaL_newmetatable( L, mts[ i ].name );
		luaL_setfuncs( L, mts[ i ].regs, 0 );
		lua_pop( L, 1 );
	}

	// constructors: vector3 / vector2 / color
	lua_newtable( L );
	lua_pushcfunction( L, l_vec3_new ); lua_setfield( L, -2, "new" );
	lua_setglobal( L, "vector3" );
	lua_newtable( L );
	lua_pushcfunction( L, l_vec2_new ); lua_setfield( L, -2, "new" );
	lua_setglobal( L, "vector2" );
	lua_newtable( L );
	lua_pushcfunction( L, l_color_new ); lua_setfield( L, -2, "new" );
	lua_setglobal( L, "color" );

	// cfg
	lua_newtable( L );
	lua_pushcfunction( L, l_cfg_get ); lua_setfield( L, -2, "get" );
	lua_pushcfunction( L, l_cfg_set ); lua_setfield( L, -2, "set" );
	lua_pushcfunction( L, l_cfg_list ); lua_setfield( L, -2, "list" );
	lua_setglobal( L, "cfg" );

	// ui (script menus in the SCRIPTS tab)
	lua_newtable( L );
	lua_pushcfunction( L, l_ui_checkbox ); lua_setfield( L, -2, "checkbox" );
	lua_pushcfunction( L, l_ui_slider_int ); lua_setfield( L, -2, "slider_int" );
	lua_pushcfunction( L, l_ui_slider_float ); lua_setfield( L, -2, "slider_float" );
	lua_pushcfunction( L, l_ui_combo ); lua_setfield( L, -2, "combo" );
	lua_pushcfunction( L, l_ui_button ); lua_setfield( L, -2, "button" );
	lua_pushcfunction( L, l_ui_text ); lua_setfield( L, -2, "text" );
	lua_pushcfunction( L, l_ui_header ); lua_setfield( L, -2, "header" );
	lua_pushcfunction( L, l_ui_separator ); lua_setfield( L, -2, "separator" );
	lua_pushcfunction( L, l_ui_same_line ); lua_setfield( L, -2, "same_line" );
	lua_pushcfunction( L, l_ui_get ); lua_setfield( L, -2, "get" );
	lua_pushcfunction( L, l_ui_set ); lua_setfield( L, -2, "set" );
	lua_setglobal( L, "ui" );

	// input
	lua_newtable( L );
	lua_pushcfunction( L, l_input_is_down ); lua_setfield( L, -2, "is_down" );
	lua_pushcfunction( L, LuaAPI::l_input_cursor ); lua_setfield( L, -2, "cursor" ); // r33
	lua_setglobal( L, "input" );

	// cvars
	lua_newtable( L );
	lua_pushcfunction( L, l_cvars_find ); lua_setfield( L, -2, "find" );
	lua_setglobal( L, "cvars" );

	// client_frame_stage enum (SDK ClientFrameStage_t)
	lua_newtable( L );
	lua_pushinteger( L, -1 ); lua_setfield( L, -2, "FRAME_UNDEFINED" );
	lua_pushinteger( L, 0 ); lua_setfield( L, -2, "FRAME_START" );
	lua_pushinteger( L, 1 ); lua_setfield( L, -2, "FRAME_NET_UPDATE_START" );
	lua_pushinteger( L, 2 ); lua_setfield( L, -2, "FRAME_NET_UPDATE_POSTDATAUPDATE_START" );
	lua_pushinteger( L, 3 ); lua_setfield( L, -2, "FRAME_NET_UPDATE_POSTDATAUPDATE_END" );
	lua_pushinteger( L, 4 ); lua_setfield( L, -2, "FRAME_NET_UPDATE_END" );
	lua_pushinteger( L, 5 ); lua_setfield( L, -2, "FRAME_RENDER_START" );
	lua_pushinteger( L, 6 ); lua_setfield( L, -2, "FRAME_RENDER_END" );
	lua_setglobal( L, "client_frame_stage" );

	// draw
	lua_newtable( L );
	lua_pushcfunction( L, l_draw_text ); lua_setfield( L, -2, "text" );
	lua_pushcfunction( L, l_draw_filled_rect ); lua_setfield( L, -2, "filled_rect" );
	lua_pushcfunction( L, l_draw_outlined_rect ); lua_setfield( L, -2, "outlined_rect" );
	lua_pushcfunction( L, l_draw_line ); lua_setfield( L, -2, "line" );
	lua_pushcfunction( L, l_draw_circle ); lua_setfield( L, -2, "circle" );
	lua_pushcfunction( L, l_draw_w2s ); lua_setfield( L, -2, "world_to_screen" );
	lua_pushcfunction( L, LuaAPI::l_draw_screen_size ); lua_setfield( L, -2, "get_screen_size" ); // r23
	lua_setglobal( L, "draw" );

	// ents
	lua_newtable( L );
	lua_pushcfunction( L, l_ents_get ); lua_setfield( L, -2, "get" );
	lua_pushcfunction( L, l_ents_local_player ); lua_setfield( L, -2, "local_player" );
	lua_pushcfunction( L, l_ents_local ); lua_setfield( L, -2, "local" );
	lua_pushcfunction( L, l_ents_valid ); lua_setfield( L, -2, "valid" );
	lua_pushcfunction( L, l_ents_alive ); lua_setfield( L, -2, "alive" );
	lua_pushcfunction( L, l_ents_health ); lua_setfield( L, -2, "health" );
	lua_pushcfunction( L, l_ents_team ); lua_setfield( L, -2, "team" );
	lua_pushcfunction( L, l_ents_pos ); lua_setfield( L, -2, "pos" );
	lua_pushcfunction( L, l_ents_flags ); lua_setfield( L, -2, "flags" );
	lua_pushcfunction( L, l_ents_dormant ); lua_setfield( L, -2, "dormant" );
	lua_pushcfunction( L, l_ents_name ); lua_setfield( L, -2, "name" );
	lua_pushcfunction( L, l_ents_get_all ); lua_setfield( L, -2, "get_all" );
	lua_pushcfunction( L, LuaAPI::l_ents_eye_angles ); lua_setfield( L, -2, "eye_angles" ); // r23
	lua_pushcfunction( L, LuaAPI::l_ents_hitbox ); lua_setfield( L, -2, "hitbox" ); // r23
	lua_setglobal( L, "ents" );

	// utils
	lua_newtable( L );
	lua_pushcfunction( L, l_utils_tick ); lua_setfield( L, -2, "tick" );
	lua_pushcfunction( L, l_utils_time ); lua_setfield( L, -2, "time" );
	lua_pushcfunction( L, l_utils_curtime ); lua_setfield( L, -2, "curtime" );
	lua_pushcfunction( L, l_utils_frametime ); lua_setfield( L, -2, "frametime" );
	lua_pushcfunction( L, l_utils_ipt ); lua_setfield( L, -2, "interval_per_tick" );
	lua_pushcfunction( L, LuaAPI::l_utils_latency ); lua_setfield( L, -2, "latency" ); // r23
	lua_pushcfunction( L, LuaAPI::l_utils_choke ); lua_setfield( L, -2, "choke" ); // r23
	lua_setglobal( L, "utils" );


	// r25: aw.cmd metatable (methods callable directly on the userdata)
	luaL_newmetatable( L, "aw.cmd" );
	static const luaL_Reg cmdRegs[ ] =
	{
		{ "get_viewangles", LuaAPI::l_cmd_get_viewangles }, { "set_viewangles", LuaAPI::l_cmd_set_viewangles },
		{ "get_buttons", LuaAPI::l_cmd_get_buttons }, { "set_buttons", LuaAPI::l_cmd_set_buttons },
		{ "get_move", LuaAPI::l_cmd_get_move }, { "set_move", LuaAPI::l_cmd_set_move },
		{ "tick", LuaAPI::l_cmd_tick },
		{ "get_sendpacket", LuaAPI::l_cmd_get_sendpacket }, { "set_sendpacket", LuaAPI::l_cmd_set_sendpacket }, // r33
		{ nullptr, nullptr }
	};
	luaL_setfuncs( L, cmdRegs, 0 );
	lua_pushvalue( L, -1 );
	lua_setfield( L, -2, "__index" ); // enable method call syntax cmd:method()
	lua_pop( L, 1 );

	// hint table
	lua_newtable( L );
	lua_pushstring( L, "awesware lua 2.4 - scripts in C:\\Awesware\\scripts" );
	lua_setfield( L, -2, "version" );
	lua_setglobal( L, "awes" );
}

// ---- auto-written sample when the scripts dir is empty ----
static const char* EXAMPLE_SCRIPT =
"-- awesware lua 2.4 example - Reload Scripts in the SCRIPTS tab after edits\n"
"\n"
"function on_menu( )\n"
"  ui.header( \"example hud\" )\n"
"  ui.checkbox( \"show hud\", true )\n"
"  ui.slider_int( \"x offset\", 0, 300, 8 )\n"
"  ui.combo( \"style\", { \"red\", \"green\", \"white\" }, 0 )\n"
"  if ui.button( \"say hi\" ) then print( \"hi from lua!\" ) end\n"
"end\n"
"\n"
"function on_paint( )\n"
"  if not ui.get( \"show hud\", true ) then return end\n"
"  local me = ents.local_player( )\n"
"  if not me or not me:is_alive( ) then return end\n"
"  local x = ui.get( \"x offset\", 8 )\n"
"  local style = ui.get( \"style\", 0 )\n"
"  local r, g, b = 255, 60, 60\n"
"  if style == 1 then r, g, b = 80, 220, 80 end\n"
"  if style == 2 then r, g, b = 255, 255, 255 end\n"
"  draw.filled_rect( x, 8, 150, 34, 20, 20, 20, 160 )\n"
"  draw.text( x + 6, 12, r, g, b, 255, \"hp: \" .. me:get_health( ) )\n"
"  local anim = me:get_animstate( )\n"
"  draw.text( x + 6, 26, 200, 200, 200, 255, string.format( \"feet: %.0f\", anim.goal_feet_yaw ) )\n"
"end\n"
"\n"
"function on_event( ev )\n"
"  if ev:get_name( ) == \"player_hurt\" then\n"
"    print( \"hurt, dmg:\", ev:get_int( \"dmg_health\" ) )\n"
"  end\n"
"end\n";

// ---- load / unload ----
static void LoadAll( void )
{
	CConfig::EnsureDirs( );
	std::string dir = CConfig::ScriptDir( );

	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA( ( dir + "*.lua" ).c_str( ), &fd );
	if( h == INVALID_HANDLE_VALUE )
	{
		FILE* f = fopen( ( dir + "example.lua" ).c_str( ), "w" );
		if( f ) { fputs( EXAMPLE_SCRIPT, f ); fclose( f ); }
		return;
	}

	do
	{
		LuaScript s;
		s.L = nullptr;
		s.hasTick = s.hasPaint = s.hasEvent = s.hasFrameStage = s.hasMenu = s.hasShot = s.hasCreateMove = false;
		s.status = "OK";
		std::string file( fd.cFileName );
		if( file.size( ) <= 4 ) continue;
		s.name = file.substr( 0, file.size( ) - 4 );

		lua_State* L = luaL_newstate( );
		if( !L ) { s.status = "out of memory"; s_scripts.push_back( s ); continue; }

		luaL_openlibs( L );
		Sandbox( L );
		RegisterAPI( L );
		lua_sethook( L, CountHook, LUA_MASKCOUNT, 5000000 );

		if( luaL_dofile( L, ( dir + file ).c_str( ) ) != LUA_OK )
		{
			const char* e = lua_tostring( L, -1 );
			s.status = e ? e : "load error";
			printconsole( "[lua] %s: %s\n", s.name.c_str( ), s.status.c_str( ) );
			lua_pop( L, 1 );
			lua_close( L );
		}
		else
		{
			const char* cbs[ ] = { "on_tick", "on_paint", "on_event", "on_frame_stage", "on_menu", "on_shot", "on_create_move" }; // r23/r25
			bool* flags[ ] = { &s.hasTick, &s.hasPaint, &s.hasEvent, &s.hasFrameStage, &s.hasMenu, &s.hasShot, &s.hasCreateMove }; // r23/r25
			for( int c = 0; c < 7; c++ ) // r23/r25: +on_shot +on_create_move
			{
				lua_getglobal( L, cbs[ c ] );
				*flags[ c ] = lua_isfunction( L, -1 ) ? true : false;
				lua_pop( L, 1 );
			}
			s.L = L;
			printconsole( "[Awesware] lua: loaded '%s'%s%s%s%s\n", s.name.c_str( ),
				s.hasTick ? " [tick]" : "", s.hasPaint ? " [paint]" : "",
				s.hasEvent ? " [event]" : "", s.hasFrameStage ? " [frame]" : "",
				s.hasMenu ? " [menu]" : "" );
		}
		s_scripts.push_back( s );
	} while( FindNextFileA( h, &fd ) );
	FindClose( h );

	int ok = 0;
	for( size_t i = 0; i < s_scripts.size( ); i++ ) if( s_scripts[ i ].L ) ok++;
	printconsole( "[Awesware] Lua: %d/%d script(s) active\n", ok, ( int )s_scripts.size( ) );
}

namespace LuaAPI
{
	void Shutdown( void )
	{
		for( size_t i = 0; i < s_scripts.size( ); i++ )
			if( s_scripts[ i ].L ) lua_close( s_scripts[ i ].L );
		s_scripts.clear( );
		s_inited = false;
	}

	// ---- r23 additions ----
static int l_draw_screen_size( lua_State* L )
{
	int w = 0, h = 0;
	if( g_pEngineClient ) g_pEngineClient->GetScreenSize( w, h );
	lua_pushinteger( L, w );
	lua_pushinteger( L, h );
	return 2;
}

static int l_utils_latency( lua_State* L )
{
	float lat = 0.f;
	INetChannelInfo* nci = ( g_pEngineClient ) ? g_pEngineClient->GetNetChannelInfo( ) : 0;
	if( nci ) lat = nci->GetLatency( 0 );
	lua_pushnumber( L, lat );
	return 1;
}

static int l_utils_choke( lua_State* L )
{
	lua_pushinteger( L, g_iChokedTicks );
	return 1;
}

static int l_ents_eye_angles( lua_State* L )
{
	int idx = ( int )luaL_checkinteger( L, 1 );
	BasePlayer* e = ( idx >= 1 && g_pGlobals && idx <= g_pGlobals->maxClients ) ? ( BasePlayer* )g_pClientEntityList->GetClientEntity( idx ) : 0;
	if( !e ) return 0;
	QAngle a = e->m_angEyeAngles( );
	lua_pushnumber( L, a.x );
	lua_pushnumber( L, a.y );
	return 2;
}

static int l_ents_hitbox( lua_State* L )
{
	int idx = ( int )luaL_checkinteger( L, 1 );
	int hb = ( int )luaL_checkinteger( L, 2 );
	BasePlayer* e = ( idx >= 1 && g_pGlobals && idx <= g_pGlobals->maxClients ) ? ( BasePlayer* )g_pClientEntityList->GetClientEntity( idx ) : 0;
	if( !e ) return 0;
	matrix3x4_t m[ 128 ];
	if( !e->SetupBones( m, 128, 0x100, e->m_flSimulationTime( ) ) ) return 0;
	void* model = e->GetModel( );
	if( !model ) return 0;
	studiohdr_t* hdr = g_pModelInfo->GetStudiomodel( model );
	if( !hdr ) return 0;
	mstudiohitboxset_t* set = hdr->pHitboxSet( e->m_nHitboxSet( ) );
	if( !set ) return 0;
	mstudiobbox_t* bb = set->pHitbox( hb );
	if( !bb ) return 0;
	Vector c = ( bb->bbmin + bb->bbmax ) * 0.5f;
	Vector out;
	VectorTransform( c, m[ bb->bone ], out );
	lua_pushnumber( L, out.x );
	lua_pushnumber( L, out.y );
	lua_pushnumber( L, out.z );
	return 3;
}


struct CmdUD { CUserCmd* cmd; }; // r25

static int l_cmd_get_viewangles( lua_State* L )
{
	CmdUD* c = ( CmdUD* )luaL_checkudata( L, 1, "aw.cmd" );
	Vec3UD* r = ( Vec3UD* )lua_newuserdata( L, sizeof( Vec3UD ) );
	r->x = c->cmd->viewangles.x; r->y = c->cmd->viewangles.y; r->z = c->cmd->viewangles.z;
	luaL_getmetatable( L, "aw.vec3" );
	lua_setmetatable( L, -2 );
	return 1;
}

static int l_cmd_set_viewangles( lua_State* L )
{
	CmdUD* c = ( CmdUD* )luaL_checkudata( L, 1, "aw.cmd" );
	c->cmd->viewangles.x = ( float )luaL_checknumber( L, 2 );
	c->cmd->viewangles.y = ( float )luaL_checknumber( L, 3 );
	c->cmd->viewangles.z = ( float )luaL_optnumber( L, 4, 0 );
	return 0;
}

static int l_cmd_get_buttons( lua_State* L )
{
	CmdUD* c = ( CmdUD* )luaL_checkudata( L, 1, "aw.cmd" );
	lua_pushinteger( L, c->cmd->buttons );
	return 1;
}

static int l_cmd_set_buttons( lua_State* L )
{
	CmdUD* c = ( CmdUD* )luaL_checkudata( L, 1, "aw.cmd" );
	c->cmd->buttons = ( int )luaL_checkinteger( L, 2 );
	return 0;
}

static int l_cmd_get_move( lua_State* L )
{
	CmdUD* c = ( CmdUD* )luaL_checkudata( L, 1, "aw.cmd" );
	const char* k = luaL_checkstring( L, 2 );
	if( !strcmp( k, "forward" ) ) { lua_pushnumber( L, c->cmd->forwardmove ); return 1; }
	if( !strcmp( k, "side" ) ) { lua_pushnumber( L, c->cmd->sidemove ); return 1; }
	if( !strcmp( k, "up" ) ) { lua_pushnumber( L, c->cmd->upmove ); return 1; }
	return luaL_error( L, "cmd:get_move: expected forward/side/up" );
}

static int l_cmd_set_move( lua_State* L )
{
	CmdUD* c = ( CmdUD* )luaL_checkudata( L, 1, "aw.cmd" );
	const char* k = luaL_checkstring( L, 2 );
	float v = ( float )luaL_checknumber( L, 3 );
	if( !strcmp( k, "forward" ) ) { c->cmd->forwardmove = v; return 0; }
	if( !strcmp( k, "side" ) ) { c->cmd->sidemove = v; return 0; }
	if( !strcmp( k, "up" ) ) { c->cmd->upmove = v; return 0; }
	return luaL_error( L, "cmd:set_move: expected forward/side/up" );
}

static int l_cmd_tick( lua_State* L )
{
	CmdUD* c = ( CmdUD* )luaL_checkudata( L, 1, "aw.cmd" );
	lua_pushinteger( L, c->cmd->tick_count );
	return 1;
}

// ---- r33: sendpacket access (packet/fakelag control from lua) ----
static int l_cmd_get_sendpacket( lua_State* L )
{
	lua_pushboolean( L, g_bSendPacket ? 1 : 0 );
	return 1;
}

static int l_cmd_set_sendpacket( lua_State* L )
{
	g_bSendPacket = lua_toboolean( L, 1 ) != 0;
	return 0;
}

// ---- r33: cursor position for script-drawn huds/menus ----
static int l_input_cursor( lua_State* L )
{
	ImVec2 mp = ImGui::GetIO( ).MousePos;
	lua_pushinteger( L, ( int )mp.x );
	lua_pushinteger( L, ( int )mp.y );
	return 2;
}

void Init( void )
	{
		Shutdown( );
		LoadAll( );
		s_inited = true;
	}

	void Reload( void )
	{
		Init( );
	}

	static void CallFn( LuaScript& s, const char* fn )
	{
		if( !s.L || s.status != "OK" ) return;
		lua_getglobal( s.L, fn );
		if( !lua_isfunction( s.L, -1 ) ) { lua_pop( s.L, 1 ); return; }
		if( lua_pcall( s.L, 0, 0, 0 ) != LUA_OK )
		{
			const char* e = lua_tostring( s.L, -1 );
			s.status = e ? e : "runtime error";
			printconsole( "[lua] %s: %s\n", s.name.c_str( ), s.status.c_str( ) );
			lua_pop( s.L, 1 );
		}
	}

	static void CallFn1( LuaScript& s, const char* fn, void ( *pushArg )( lua_State*, void* ), void* arg )
	{
		if( !s.L || s.status != "OK" ) return;
		lua_getglobal( s.L, fn );
		if( !lua_isfunction( s.L, -1 ) ) { lua_pop( s.L, 1 ); return; }
		pushArg( s.L, arg );
		if( lua_pcall( s.L, 1, 0, 0 ) != LUA_OK )
		{
			const char* e = lua_tostring( s.L, -1 );
			s.status = e ? e : "runtime error";
			printconsole( "[lua] %s: %s\n", s.name.c_str( ), s.status.c_str( ) );
			lua_pop( s.L, 1 );
		}
	}

	static void PushEventArg( lua_State* L, void* arg )
	{
		EventUD* e = ( EventUD* )lua_newuserdata( L, sizeof( EventUD ) );
		e->ev = ( IGameEvent* )arg;
		luaL_getmetatable( L, "aw.event" );
		lua_setmetatable( L, -2 );
	}

	static void PushIntArg( lua_State* L, void* arg )
	{
		lua_pushinteger( L, ( lua_Integer )( intptr_t )arg );
	}

	void Tick( void )
	{
		if( !s_inited ) { Init( ); return; }
		LuaLock( );
		for( size_t i = 0; i < s_scripts.size( ); i++ )
			if( s_scripts[ i ].hasTick ) CallFn( s_scripts[ i ], "on_tick" );
		LuaUnlock( );
	}

	void Paint( void )
	{
		if( !s_inited ) return;
		LuaLock( );
		for( size_t i = 0; i < s_scripts.size( ); i++ )
			if( s_scripts[ i ].hasPaint ) CallFn( s_scripts[ i ], "on_paint" );
		LuaUnlock( );
	}

	void Event( void* eventPtr )
	{
		if( !s_inited || !eventPtr ) return;
		LuaLock( );
		for( size_t i = 0; i < s_scripts.size( ); i++ )
			if( s_scripts[ i ].hasEvent ) CallFn1( s_scripts[ i ], "on_event", PushEventArg, eventPtr );
		LuaUnlock( );
	}

	void FrameStage( int stage )
	{
		if( !s_inited ) return;
		LuaLock( );
		for( size_t i = 0; i < s_scripts.size( ); i++ )
			if( s_scripts[ i ].hasFrameStage ) CallFn1( s_scripts[ i ], "on_frame_stage", PushIntArg, ( void* )( intptr_t )stage );
		LuaUnlock( );
	}

	// r23: on_shot(targetIndex) - dispatched when WE commit a live shot
	void Shot( int targetIndex )
	{
		if( !s_inited ) return;
		LuaLock( );
		for( size_t i = 0; i < s_scripts.size( ); i++ )
			if( s_scripts[ i ].hasShot ) CallFn1( s_scripts[ i ], "on_shot", PushIntArg, ( void* )( intptr_t )targetIndex );
		LuaUnlock( );
	}

	// r25: on_create_move(cmd) - script access to the live command (angles/buttons/move)
	void CreateMove( void* cmdPtr )
	{
		if( !s_inited || !cmdPtr ) return;
		LuaLock( );
		for( size_t i = 0; i < s_scripts.size( ); i++ )
		{
			if( !s_scripts[ i ].hasCreateMove || !s_scripts[ i ].L || s_scripts[ i ].status != "OK" ) continue;
			lua_getglobal( s_scripts[ i ].L, "on_create_move" );
			if( !lua_isfunction( s_scripts[ i ].L, -1 ) ) { lua_pop( s_scripts[ i ].L, 1 ); continue; }
			CmdUD* c = ( CmdUD* )lua_newuserdata( s_scripts[ i ].L, sizeof( CmdUD ) );
			c->cmd = ( CUserCmd* )cmdPtr;
			luaL_getmetatable( s_scripts[ i ].L, "aw.cmd" );
			lua_setmetatable( s_scripts[ i ].L, -2 );
			if( lua_pcall( s_scripts[ i ].L, 1, 0, 0 ) != LUA_OK )
			{
				const char* e = lua_tostring( s_scripts[ i ].L, -1 );
				s_scripts[ i ].status = e ? e : "runtime error";
				printconsole( "[lua] %s: %s\n", s_scripts[ i ].name.c_str( ), s_scripts[ i ].status.c_str( ) );
				lua_pop( s_scripts[ i ].L, 1 );
			}
		}
		LuaUnlock( );
	}

	void Menu( int index )
	{
		if( !s_inited || index < 0 || index >= ( int )s_scripts.size( ) ) return;
		LuaLock( );
		CallFn( s_scripts[ index ], "on_menu" );
		LuaUnlock( );
	}

	bool ScriptHasMenu( int index )
	{
		if( index < 0 || index >= ( int )s_scripts.size( ) ) return false;
		return s_scripts[ index ].hasMenu && s_scripts[ index ].status == "OK";
	}

	int ScriptCount( void ) { return ( int )s_scripts.size( ); }

	const char* ScriptName( int i )
	{
		if( i < 0 || i >= ( int )s_scripts.size( ) ) return "";
		return s_scripts[ i ].name.c_str( );
	}

	const char* ScriptStatus( int i )
	{
		if( i < 0 || i >= ( int )s_scripts.size( ) ) return "";
		return s_scripts[ i ].status.c_str( );
	}
}
