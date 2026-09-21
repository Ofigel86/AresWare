#pragma once

#include "Detour.hpp"
#include "VmtSwap.hpp"

#include <windows.h>

#include <cstdint>

#define ConCat( x, y )	x##y
#define Macro( x, y )	ConCat( x, y )
#define MakePad( size )	std::uint8_t Macro( __pad, __COUNTER__ )[ size ]
namespace memory
{

	template< const unsigned int I, typename T >
	inline auto vget(void* instance) -> T
	{
		auto data = *reinterpret_cast<std::uintptr_t**>(instance);
		return (T)data[I];
	}

	template< const unsigned int I, typename T, typename... ArgsT >
	inline auto vcall(void* instance, ArgsT... args) -> T
	{
		auto procedure = vget< I, T(__thiscall*)(void*, ArgsT...) >(instance);
		return procedure(instance, args...);
	}

}
namespace Memory
{
	template< typename T >
	inline T VCall( void* instance, std::size_t index )
	{
		return ( T )( *( void*** )instance )[ index ];
	}

	template< typename T >
	inline void SafeDelete( T object )
	{
		if( object )
			delete object;

		object = nullptr;
	}

	template< typename T >
	inline void SafeDeleteArray( T object )
	{
		if( object )
			delete[ ] object;

		object = nullptr;
	}

	template< typename T >
	inline void SafeReset( T object )
	{
		if( object )
			object.reset();

		object = nullptr;
	}

	template< typename T >
	inline void SafeRelease( T object )
	{
		if( object )
			object->Release();

		object = nullptr;
	}

	template< typename T >
	inline T Read( std::uintptr_t address, const T& def = T() )
	{
		__try
		{
			return *( T* )( address );
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
			return def;
		}
	}

	template< typename T >
	inline bool Write( std::uintptr_t address, const T& value )
	{
		__try
		{
			*( T* )( address ) = value;

			return true;
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
			return false;
		}
	}

	extern std::uintptr_t PatternScan( const char* szMod, const char* szPattern );
}