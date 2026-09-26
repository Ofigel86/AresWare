#include "ModuleBases.h"
#include "Logger.h"

DWORD g_dwBaseEngineDll = 0;
DWORD g_dwBaseServerDll = 0;
DWORD g_dwBaseClientDll = 0;

void ResolveModuleBases( void )
{
	g_dwBaseEngineDll = ( DWORD )GetModuleHandleA( "engine.dll" );
	g_dwBaseClientDll = ( DWORD )GetModuleHandleA( "client.dll" );

	// server.dll exists only in-game (listen server); keep the old value if we
	// resolved it before, so a menu->game transition never loses it.
	DWORD dwServer = ( DWORD )GetModuleHandleA( "server.dll" );
	if( dwServer ) g_dwBaseServerDll = dwServer;

	Logger::Write( "Module bases: engine.dll=0x%08X client.dll=0x%08X server.dll=0x%08X%s",
		g_dwBaseEngineDll, g_dwBaseClientDll, g_dwBaseServerDll,
		g_dwBaseServerDll ? "" : " (not loaded yet)" );

	if( g_dwBaseEngineDll != 0x20000000 )
		Logger::Write( "NOTE: engine.dll rebased from 0x20000000 - offsets now follow the real base" );
	if( g_dwBaseClientDll != 0x24000000 )
		Logger::Write( "NOTE: client.dll rebased from 0x24000000 - offsets now follow the real base" );
	if( g_dwBaseServerDll && g_dwBaseServerDll != 0x22000000 )
		Logger::Write( "NOTE: server.dll rebased from 0x22000000 - offsets now follow the real base" );
}

bool AddressInModule( DWORD address, DWORD moduleBase )
{
	if( !moduleBase || !address ) return false;

	// must belong to that module image: address - base has to be a sane RVA
	if( address < moduleBase ) return false;

	MEMORY_BASIC_INFORMATION mbi;
	if( !VirtualQuery( ( LPCVOID )address, &mbi, sizeof( mbi ) ) ) return false;
	if( mbi.State != MEM_COMMIT ) return false;
	if( ( DWORD )mbi.AllocationBase != moduleBase ) return false;

	const DWORD dwExec = PAGE_EXECUTE | PAGE_EXECUTE_READ |
		PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
	return ( mbi.Protect & dwExec ) != 0;
}
