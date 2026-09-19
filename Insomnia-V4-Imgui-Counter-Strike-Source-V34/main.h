// BUILD MARKER r46 (2026-09-19): pixel-perfect icons from the reference shots injected into the font atlas + drawn in the V3 tab bar and weapon strip.
// BUILD MARKER r45 (2026-09-19): per-weapon-group rage settings (Pistol/SMG/Rifle/Shotgun/Sniper) - overlay applied around the rage aimbot call.
// BUILD MARKER r41 (2026-09-19): slow walk default bind = SHIFT (combo index 6 -> VK 0x10).
// BUILD MARKER r25 (2026-09-18): on_create_move dispatch + interp-off (Segregation 0x36200) + BestDamage full-scan + spectator list + dual hitbox selectors (old base combo removed).
// BUILD MARKER r9 (2026-09-18): NATIVE CHOKE - CanPacket(slot 52) gate; engine skips CL_SendMove itself (SetChoked) so fakelag/DT backlog never loses CLC_Move.
#pragma once

#include <algorithm>
#include <string>
#include <memory>
#include <array>
#include "XOR.h"
#include "SdkIncludes.h"
using namespace Valve;

#define BASE_ENGINE 0x20000000
#define BASE_SERVER 0x22000000
#define BASE_CLIENT 0x24000000
#define HISTORY_MAX 128
#define MULTIPLAYER_BACKUP 90
#define LAGCOMP_MAX 64

#define ASSIGNXIFNZERODO(x, y) x = y; if (x)
#define ASSIGNVARANDIFNZERODO(x, y) auto x = y; if (x)
#define DESTORY_INTERFACE(x) if(x) { delete x; x = 0; }
#define ZEROPOINTER(x) if(x) x = 0;

#define TICK_INTERVAL 0.015
#define TIME_TO_TICKS( dt )	( ( int )( 0.5f + ( float )( dt ) / g_pGlobals->interval_per_tick ) )
#define TICKS_TO_TIME( t )		( TICK_INTERVAL * ( t ) )
#define ROUND_TO_TICKS( t )		( TICK_INTERVAL * TIME_TO_TICKS( t ) )
#define TICK_NEVER_THINK		(-1)

#define makeptr( cast, ptr, addValue ) (cast)( (DWORD_PTR)(ptr) + (DWORD_PTR)(addValue))

static char* memdup( const char* s, size_t n )
{
	char* t = ( char* )malloc( n );
	memcpy( t, s, n );
	return t;
}
#define _memdup( object ) memdup( object, sizeof( object ) )

enum
{
	MAX_JOYSTICKS = 1,
	MOUSE_BUTTON_COUNT = 5,
	MAX_NOVINT_DEVICES = 2,
};

enum JoystickAxis_t
{
	JOY_AXIS_X = 0,
 	JOY_AXIS_Y,
	JOY_AXIS_Z,
	JOY_AXIS_R,
	JOY_AXIS_U,
	JOY_AXIS_V,
	MAX_JOYSTICK_AXES,
};

enum
{
	JOYSTICK_MAX_BUTTON_COUNT = 32,
	JOYSTICK_POV_BUTTON_COUNT = 4,
	JOYSTICK_AXIS_BUTTON_COUNT = MAX_JOYSTICK_AXES * 2,
};

#define JOYSTICK_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_BUTTON + ((_joystick) * JOYSTICK_MAX_BUTTON_COUNT) + (_button) )
#define JOYSTICK_POV_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_POV_BUTTON + ((_joystick) * JOYSTICK_POV_BUTTON_COUNT) + (_button) )
#define JOYSTICK_AXIS_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_AXIS_BUTTON + ((_joystick) * JOYSTICK_AXIS_BUTTON_COUNT) + (_button) )

#define JOYSTICK_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_BUTTON_INTERNAL( _joystick, _button ) )
#define JOYSTICK_POV_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_POV_BUTTON_INTERNAL( _joystick, _button ) )
#define JOYSTICK_AXIS_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_AXIS_BUTTON_INTERNAL( _joystick, _button ) )

enum ButtonCode_t
{
	BUTTON_CODE_INVALID = -1,
	BUTTON_CODE_NONE = 0,

	KEY_FIRST = 0,

	KEY_NONE = KEY_FIRST,
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_PAD_0,
	KEY_PAD_1,
	KEY_PAD_2,
	KEY_PAD_3,
	KEY_PAD_4,
	KEY_PAD_5,
	KEY_PAD_6,
	KEY_PAD_7,
	KEY_PAD_8,
	KEY_PAD_9,
	KEY_PAD_DIVIDE,
	KEY_PAD_MULTIPLY,
	KEY_PAD_MINUS,
	KEY_PAD_PLUS,
	KEY_PAD_ENTER,
	KEY_PAD_DECIMAL,
	KEY_LBRACKET,
	KEY_RBRACKET,
	KEY_SEMICOLON,
	KEY_APOSTROPHE,
	KEY_BACKQUOTE,
	KEY_COMMA,
	KEY_PERIOD,
	KEY_SLASH,
	KEY_BACKSLASH,
	KEY_MINUS,
	KEY_EQUAL,
	KEY_ENTER,
	KEY_SPACE,
	KEY_BACKSPACE,
	KEY_TAB,
	KEY_CAPSLOCK,
	KEY_NUMLOCK,
	KEY_ESCAPE,
	KEY_SCROLLLOCK,
	KEY_INSERT,
	KEY_DELETE,
	KEY_HOME,
	KEY_END,
	KEY_PAGEUP,
	KEY_PAGEDOWN,
	KEY_BREAK,
	KEY_LSHIFT,
	KEY_RSHIFT,
	KEY_LALT,
	KEY_RALT,
	KEY_LCONTROL,
	KEY_RCONTROL,
	KEY_LWIN,
	KEY_RWIN,
	KEY_APP,
	KEY_UP,
	KEY_LEFT,
	KEY_DOWN,
	KEY_RIGHT,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_CAPSLOCKTOGGLE,
	KEY_NUMLOCKTOGGLE,
	KEY_SCROLLLOCKTOGGLE,

	KEY_LAST = KEY_SCROLLLOCKTOGGLE,
	KEY_COUNT = KEY_LAST - KEY_FIRST + 1,

	// Mouse
	MOUSE_FIRST = KEY_LAST + 1,

	MOUSE_LEFT = MOUSE_FIRST,
	MOUSE_RIGHT,
	MOUSE_MIDDLE,
	MOUSE_4,
	MOUSE_5,
	MOUSE_WHEEL_UP,		// A fake button which is 'pressed' and 'released' when the wheel is moved up 
	MOUSE_WHEEL_DOWN,	// A fake button which is 'pressed' and 'released' when the wheel is moved down

	MOUSE_LAST = MOUSE_WHEEL_DOWN,
	MOUSE_COUNT = MOUSE_LAST - MOUSE_FIRST + 1,

	// Joystick
	JOYSTICK_FIRST = MOUSE_LAST + 1,

	JOYSTICK_FIRST_BUTTON = JOYSTICK_FIRST,
	JOYSTICK_LAST_BUTTON = JOYSTICK_BUTTON_INTERNAL( MAX_JOYSTICKS-1, JOYSTICK_MAX_BUTTON_COUNT-1 ),
	JOYSTICK_FIRST_POV_BUTTON,
	JOYSTICK_LAST_POV_BUTTON = JOYSTICK_POV_BUTTON_INTERNAL( MAX_JOYSTICKS-1, JOYSTICK_POV_BUTTON_COUNT-1 ),
	JOYSTICK_FIRST_AXIS_BUTTON,
	JOYSTICK_LAST_AXIS_BUTTON = JOYSTICK_AXIS_BUTTON_INTERNAL( MAX_JOYSTICKS-1, JOYSTICK_AXIS_BUTTON_COUNT-1 ),

	JOYSTICK_LAST = JOYSTICK_LAST_AXIS_BUTTON,

#if !defined ( _X360 )
	NOVINT_FIRST = JOYSTICK_LAST + 2, // plus 1 missing key. +1 seems to cause issues on the first button.
	
	NOVINT_LOGO_0 = NOVINT_FIRST,
	NOVINT_TRIANGLE_0,
	NOVINT_BOLT_0,
	NOVINT_PLUS_0,
	NOVINT_LOGO_1,
	NOVINT_TRIANGLE_1,
	NOVINT_BOLT_1,
	NOVINT_PLUS_1,
	
	NOVINT_LAST = NOVINT_PLUS_1,
#endif

	BUTTON_CODE_LAST,
	BUTTON_CODE_COUNT = BUTTON_CODE_LAST - KEY_FIRST + 1,

	// Helpers for XBox 360
	KEY_XBUTTON_UP = JOYSTICK_FIRST_POV_BUTTON,	// POV buttons
	KEY_XBUTTON_RIGHT,
	KEY_XBUTTON_DOWN,
	KEY_XBUTTON_LEFT,

	KEY_XBUTTON_A = JOYSTICK_FIRST_BUTTON,		// Buttons
	KEY_XBUTTON_B,
	KEY_XBUTTON_X,
	KEY_XBUTTON_Y,
	KEY_XBUTTON_LEFT_SHOULDER,
	KEY_XBUTTON_RIGHT_SHOULDER,
	KEY_XBUTTON_BACK,
	KEY_XBUTTON_START,
	KEY_XBUTTON_STICK1,
	KEY_XBUTTON_STICK2,

	KEY_XSTICK1_RIGHT = JOYSTICK_FIRST_AXIS_BUTTON,	// XAXIS POSITIVE
	KEY_XSTICK1_LEFT,							// XAXIS NEGATIVE
	KEY_XSTICK1_DOWN,							// YAXIS POSITIVE
	KEY_XSTICK1_UP,								// YAXIS NEGATIVE
	KEY_XBUTTON_LTRIGGER,						// ZAXIS POSITIVE
	KEY_XBUTTON_RTRIGGER,						// ZAXIS NEGATIVE
	KEY_XSTICK2_RIGHT,							// UAXIS POSITIVE
	KEY_XSTICK2_LEFT,							// UAXIS NEGATIVE
	KEY_XSTICK2_DOWN,							// VAXIS POSITIVE
	KEY_XSTICK2_UP,								// VAXIS NEGATIVE
};

#include "Main.h"

#include <time.h>
#include "ade32.h"
#include "detours2.h"
#include "VMTHook.h"
#include "Hook.h"
#include "LuaAPI.h"
#include "Config.h"
#include "Stuff.h"
#include "Whitelist.h"
#include "Prediction.h"
#include "NoSpread.h"
#include "Aimbot.h"
#include "GameEventManager.h"
#include "Drawing.h"
#include "GUI.h"
#include "Recorder.h"
#include "Macro.h"

extern char *m_pszDllPath;
extern int mouse_x;
extern int mouse_y;
extern bool bMouse;
extern void printconsole( const char*, ... );

inline void**& getvtable( void* inst, size_t offset = 0 )
{
	return *reinterpret_cast<void***>( (size_t)inst + offset );
}
inline const void** getvtable( const void* inst, size_t offset = 0 )
{
	return *reinterpret_cast<const void***>( (size_t)inst + offset );
}
template< typename Fn >
inline Fn getvfunc( const void* inst, size_t index, size_t offset = 0 )
{
	return reinterpret_cast<Fn>( getvtable( inst, offset )[ index ] );
}
template< typename type >
type GetFuncModule( char * szModule, char * szFunc, DWORD dwPadd = 0 )
{
	return ( type )( ( DWORD )GetProcAddress( GetModuleHandleA( szModule ), szFunc ) + dwPadd );
}

/*
typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
 
typedef struct _PEB_LDR_DATA
{
	ULONG Length;
	BOOLEAN Initialized;
	PVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;
 
typedef struct _LDR_MODULE
{
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID BaseAddress;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	SHORT LoadCount;
	SHORT TlsIndex;
	LIST_ENTRY HashTableEntry;
	ULONG TimeDateStamp;
} LDR_MODULE, *PLDR_MODULE;
 
void x_wcstombs( wchar_t* src, size_t len, char* dest )
{
	int i;
	char tmpc;
	if( !src || !dest ) return;

	// iterate our wide str
	for( i = 0; i < len; i++ )
	{
		// truncate to 8-bit
		tmpc = src[ i ] & 255;
		if( (tmpc >= 'A') && (tmpc <= 'Z') ) tmpc += 32;
		// copy to buffer
		dest[ i ] = tmpc;
	}

	// zero-terminator fix
	dest[ len ] = '\0';
}
 
DWORD_PTR GetModuleBase( const char* lpModuleName )
{
	int iLenght;
	char lpBuffer[ 255 ];
	DWORD_PTR dwPEB;
	PEB_LDR_DATA* pPEB;
	LIST_ENTRY* pHead, *pNode;
	LDR_MODULE* pModule;
	
	dwPEB = *makeptr( DWORD_PTR*, __readfsdword( 0x18 ), 0x30 );
	dwPEB = *makeptr( DWORD_PTR*, dwPEB, 0xC );
	
	if( !dwPEB ) return NULL;
	
	pPEB = ( PEB_LDR_DATA* ) dwPEB;
	pHead = &pPEB->InLoadOrderModuleList;
	
	for( pNode = pHead->Flink; pNode != pHead; pNode = pNode->Flink )
	{
		pModule = CONTAINING_RECORD( pNode, LDR_MODULE, InLoadOrderModuleList );
		
		if( pModule )
		{
			iLenght = pModule->BaseDllName.Length >> 1;
			x_wcstombs( pModule->BaseDllName.Buffer, iLenght, lpBuffer );
			
			if( !strcmp( lpModuleName, lpBuffer ) )
			{
				return ( DWORD_PTR ) pModule->BaseAddress;
			}
		}
	}
}
*/

extern void CL_Move( void );
extern void FX_FireBullets( void );
extern void CL_RunPrediction( void );
extern void ClientInterpolation( void );
#ifdef _MSC_VER
extern void Hooked_SetViewAngles( void );
#else
extern "C" void Hooked_SetViewAngles( void );
#endif

extern float __stdcall Hooked_GetLastTimeStamp( );
extern CUserCmd* __stdcall Hooked_GetUserCmd( int );
extern bool __fastcall Hooked_InPrediction( void*, int );
extern void __fastcall Hooked_OverrideView( void*, void*, CViewSetup* );
extern void __fastcall Hooked_Update( void*, void*, bool, bool, int, int );
extern int __stdcall Hooked_IN_KeyEvent( int, int, const char* );
extern void __fastcall Hooked_FrameStageNotify( void*, void*, ClientFrameStage_t );
extern int __stdcall Hooked_DrawModelEx( ModelRenderInfo_t& );
#ifdef _MSC_VER
extern void __fastcall Hooked_CreateMove( void*, void*, int, float, bool );
#else
extern "C" void Hooked_CreateMove( void*, void*, int, float, bool );
#endif
extern void __fastcall Hooked_RunCommand( void*, void*, BasePlayer*, CUserCmd*, IMoveHelper* );
extern bool __fastcall Hooked_ClientModeCreateMove( void*, int, float, CUserCmd* );
extern void __fastcall Hooked_PaintTraverse( void*, int, unsigned int, bool, bool );
extern IMaterial* __stdcall Hooked_FindMaterial( const char*, const char*, bool, const char* );
extern void Resolver_OnNetYaw( int, float );
extern void Resolver_OnShot( int );
extern void Resolver_OnHit( int );
extern void Resolver_OnDeath( int );
extern void AIResolver_Reset( void );
extern void AIAA_OnLocalHurt( int );
extern void AIAA_Reset( void );
extern int AIAA_GetStyle( void );
extern int AIAA_GetState( void );
extern int AIC_GetInfo( void );
extern void AIC_Reset( void );
extern void Legit_Begin( void );
extern void Legit_End( void );
extern void Legit_Trigger( CUserCmd*, BasePlayer*, CSWeapon* );
extern bool Legit_IsActive( void );
extern int Aimbot_RageGroupForWeapon( BasePlayer* ); // r45
extern void GUI_BuildIconsV3( void ); // r46: atlas icon injection
extern void Legit_SmoothAim( CUserCmd*, BasePlayer* );
extern void Legit_StandaloneRCS( CUserCmd*, BasePlayer* );
extern void Legit_Strafe( CUserCmd*, BasePlayer* );
extern void Legit_OnKill( void );
extern void Legit_AutoPistol( CUserCmd*, CSWeapon* );
extern float g_flAARealYaw;
extern float g_flAAFakeYaw;
extern HMODULE g_hModule;
extern "C" bool g_bSendPacket;
extern bool g_NetchanHooked;
extern bool g_CanPacketHooked;
extern void ClientInterpolation( void ); // r25: enemy interp bypass // r9: native CanPacket choke gate live
extern void Netchan_UpdateHook( void );
extern void Log_OnShoot( int, CUserCmd* );
extern void Log_OnHitConfirm( int );
extern int g_iChokedTicks; // live choke counter, maintained by the SendDatagram hook

// key-combo index (0 = Auto/Off, 1..5 = Mouse 1..5) -> virtual-key code.
// NOTE: the raw combo index is NOT a VK code (index 3 would be VK_CANCEL) - always map through here.
inline int ComboKeyVK( int idx )
{
	// r41: index 6 = SHIFT (0x10) - used by the slow walk key
	static const int vk[ 7 ] = { 0, 0x01, 0x02, 0x04, 0x05, 0x06, 0x10 };
	if( idx < 0 || idx > 6 ) return 0;
	return vk[ idx ];
}
inline bool ComboKeyDown( int idx )
{
	int v = ComboKeyVK( idx );
	return v > 0 && ( GetAsyncKeyState( v ) & 0x8000 ) != 0;
}
// speedhack hold-key combo: E / Mouse 4 / Mouse 5 / ALT / SHIFT / X (0 = E, preserves old behavior)
inline int SpeedhackKeyVK( int idx )
{
	static const int vk[ 6 ] = { 0x45, 0x05, 0x06, 0x12, 0x10, 0x58 };
	if( idx < 0 || idx > 5 ) return 0x45;
	return vk[ idx ];
}
extern void Log_Tick( void );