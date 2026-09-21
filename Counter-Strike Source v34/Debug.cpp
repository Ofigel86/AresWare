#include "Debug.hpp"

#include <windows.h>

#include <cstdio>
#include <cstdarg>
#include <mutex>

namespace Debug
{
	namespace
	{
		std::mutex	g_LogMutex;
		FILE*		g_pLogFile = nullptr;
		DWORD		g_dwStartTick = 0;

		// Безопасное форматирование: никогда не пишем за границы буфера и
		// корректно обрабатываем длинные строки (обрезаем, а не падаем).
		int FormatV( char* szOut, std::size_t nOut, const char* fmt, va_list args )
		{
			if( !szOut || nOut == 0 )
				return -1;

			int iRet = _vsnprintf_s( szOut, nOut, _TRUNCATE, fmt, args );

			if( iRet < 0 )
			{
				szOut[ nOut - 1 ] = '\0';
				return -1;
			}

			return iRet;
		}
	}

	void Print( const char* fmt, ... )
	{
		char szFormat[ 2048 ] = {};

		va_list args;
		va_start( args, fmt );
		FormatV( szFormat, sizeof( szFormat ), fmt, args );
		va_end( args );

		OutputDebugStringA( szFormat );
		OutputDebugStringA( "\n" );
	}

	void LogOpen( const char* szPath )
	{
		std::lock_guard< std::mutex > guard( g_LogMutex );

		if( g_pLogFile )
		{
			fclose( g_pLogFile );
			g_pLogFile = nullptr;
		}

		if( !szPath )
			return;

		if( fopen_s( &g_pLogFile, szPath, "w" ) != 0 )
			g_pLogFile = nullptr;

		if( g_pLogFile )
		{
			g_dwStartTick = GetTickCount();

			SYSTEMTIME st = {};
			GetLocalTime( &st );

			fprintf( g_pLogFile, "==== AresWare log %04u-%02u-%02u %02u:%02u:%02u ====\n",
				( unsigned )st.wYear, ( unsigned )st.wMonth, ( unsigned )st.wDay,
				( unsigned )st.wHour, ( unsigned )st.wMinute, ( unsigned )st.wSecond );
			fflush( g_pLogFile );
		}
	}

	void LogClose()
	{
		std::lock_guard< std::mutex > guard( g_LogMutex );

		if( g_pLogFile )
		{
			fclose( g_pLogFile );
			g_pLogFile = nullptr;
		}
	}

	void Log( const char* fmt, ... )
	{
		char szFormat[ 2048 ] = {};

		va_list args;
		va_start( args, fmt );
		FormatV( szFormat, sizeof( szFormat ), fmt, args );
		va_end( args );

		OutputDebugStringA( szFormat );
		OutputDebugStringA( "\n" );

		std::lock_guard< std::mutex > guard( g_LogMutex );

		if( g_pLogFile )
		{
			const DWORD dwElapsed = GetTickCount() - g_dwStartTick;

			fprintf( g_pLogFile, "[%6u ms] %s\n", ( unsigned )dwElapsed, szFormat );
			fflush( g_pLogFile );
		}
	}
}
