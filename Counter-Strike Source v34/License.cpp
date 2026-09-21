#include "License.hpp"

#include <windows.h>

#include "Valve.hpp" // ModuleD, GetProcedure (ручной резолв без IAT)
#include "Crypt.hpp" // XorStr
#include "Debug.hpp" // DPRINT

namespace License
{
	// ------------------------------------------------------------------------
	// Белый список разрешённых HWID-ключей.
	// Свой ключ можно узнать так: включить PRIVATE_BUILD, инжектнуть DLL —
	// в Debug-сборке ключ напечатается через DPRINT при отказе в доступе.
	// ------------------------------------------------------------------------
	static const int AllowedKeys[] =
	{
		27092743, // rraggerr (оригинальный ключ автора)
		// 39328778,  // nikita
		// 38245117,  // danil
		// 24026354,  // aleksey
		// 31407359,  // nastya
		// 57364604,  // pavel
		// 317000998, // hitman
		// 79770357,  // daniil12312386
	};

	// Магические константы оригинальной формулы.
	// НЕ МЕНЯТЬ — иначе старые ключи перестанут подходить.
	static const DWORD KeyMagicDiv = 6983;
	static const DWORD KeyMagicAdd = 19827;

	typedef BOOL( WINAPI* GetDiskFreeSpaceAFn )( LPCSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD );
	typedef VOID( WINAPI* GetSystemInfoFn )( LPSYSTEM_INFO );

	static DWORD GetSystemPart()
	{
		SYSTEM_INFO sysinfo = { 0 };

		auto getSystemInfo = ( GetSystemInfoFn )GetProcedure(
			ModuleD( XorStr( "kernel32.dll" ) ), XorStr( "GetSystemInfo" ) );

		if( !getSystemInfo )
			return 0;

		getSystemInfo( &sysinfo );

		// Оригинал: ((((Cores * Type) * 6983) + 19827) + Revision)
		return ( ( ( sysinfo.dwNumberOfProcessors * sysinfo.dwProcessorType ) * KeyMagicDiv )
			+ KeyMagicAdd ) + sysinfo.wProcessorRevision;
	}

	static DWORD GetDiskPart()
	{
		DWORD sectorsPerCluster = 0;
		DWORD bytesPerSector = 0;
		DWORD freeClusters = 0;
		DWORD totalClusters = 0;

		auto getDiskFreeSpace = ( GetDiskFreeSpaceAFn )GetProcedure(
			ModuleD( XorStr( "kernel32.dll" ) ), XorStr( "GetDiskFreeSpaceA" ) );

		if( !getDiskFreeSpace )
			return 0;

		if( !getDiskFreeSpace( XorStr( "C:\\" ), &sectorsPerCluster, &bytesPerSector, &freeClusters, &totalClusters ) )
			return 0;

		// Внимание: порядок операций важен (целочисленная арифметика DWORD)!
		// Оригинал: (sectors *= bytes *= clusters /= 6983) += 19827,
		// что раскрывается справа налево именно так:
		totalClusters /= KeyMagicDiv;
		bytesPerSector *= totalClusters;
		sectorsPerCluster *= bytesPerSector;

		return sectorsPerCluster + KeyMagicAdd;
	}

	int GenerateKey()
	{
		return static_cast< int >( GetDiskPart() + GetSystemPart() );
	}

	bool IsAllowed( int key )
	{
		for( auto allowed : AllowedKeys )
		{
			if( allowed == key )
				return true;
		}

		return false;
	}

	bool Check()
	{
		const int key = GenerateKey();

		if( IsAllowed( key ) )
		{
			DPRINT( XorStr( "[License] Activated, key: %d" ), key );
			return true;
		}

		// Ключ печатается специально: добавьте его в AllowedKeys выше,
		// чтобы активировать этот ПК.
		DPRINT( XorStr( "[License] NOT activated, your key: %d" ), key );
		return false;
	}
}
