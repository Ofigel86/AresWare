#include "Main.h"
#include "Logger.h"

namespace
{
	HANDLE			g_hLogFile = INVALID_HANDLE_VALUE;
	CRITICAL_SECTION	g_LogCs;
	bool			g_bCsInit = false;
	char			g_LogPath[ MAX_PATH ] = { 0 };
	const char*		g_Stage = "";
	PVOID			g_VehHandle = NULL;

	void BuildPath( void )
	{
		if( g_LogPath[ 0 ] ) return;

		// next to the injected DLL (m_pszDllPath is set in DllMain)
		if( m_pszDllPath && m_pszDllPath[ 0 ] )
		{
			lstrcpynA( g_LogPath, m_pszDllPath, MAX_PATH );
			char* pSlash = strrchr( g_LogPath, '\\' );
			if( pSlash ) pSlash[ 1 ] = '\0';
			else         g_LogPath[ 0 ] = '\0';
			lstrcatA( g_LogPath, "AresWare.log" );
		}
		else
		{
			lstrcpynA( g_LogPath, "AresWare.log", MAX_PATH ); // fallback: game CWD
		}
	}

	void EnsureOpen( void )
	{
		if( g_hLogFile == INVALID_HANDLE_VALUE )
		{
			BuildPath( );
			g_hLogFile = CreateFileA( g_LogPath, GENERIC_WRITE, FILE_SHARE_READ,
				NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

			if( g_hLogFile != INVALID_HANDLE_VALUE )
				SetFilePointer( g_hLogFile, 0, NULL, FILE_END ); // append
		}
	}

	void EnsureCs( void )
	{
		if( !g_bCsInit )
		{
			InitializeCriticalSection( &g_LogCs );
			g_bCsInit = true;
		}
	}
}

void Logger::SetStage( const char* stage )
{
	g_Stage = stage ? stage : "";
}

static LONG WINAPI AresWareVeh( EXCEPTION_POINTERS* pEp )
{
	if( pEp && pEp->ExceptionRecord )
	{
		EXCEPTION_RECORD* pRec = pEp->ExceptionRecord;

		// NTSTATUS-style severity bits set: real faults (AV, int3, DLL unload...)
		if( pRec->ExceptionCode & 0x80000000 )
		{
			// CRITICAL_SECTION is per-thread recursive, so logging here is fine
			// even if the fault happened inside another Write() on this thread.
			Logger::Write( "FATAL: exception 0x%08X at address 0x%08X (stage: %s)",
				pRec->ExceptionCode, ( DWORD )pRec->ExceptionAddress, g_Stage );
			Logger::Shutdown( ); // flush via handle close; process is about to die
		}
	}
	return EXCEPTION_CONTINUE_SEARCH; // let the default handling proceed
}

void Logger::InstallCrashHandler( void )
{
	if( !g_VehHandle )
		g_VehHandle = AddVectoredExceptionHandler( 1, AresWareVeh );
}

void Logger::RemoveCrashHandler( void )
{
	if( g_VehHandle )
	{
		RemoveVectoredExceptionHandler( g_VehHandle );
		g_VehHandle = NULL;
	}
}

void Logger::Init( void )
{
	EnsureCs( );
	EnterCriticalSection( &g_LogCs );
	EnsureOpen( );
	LeaveCriticalSection( &g_LogCs );
	InstallCrashHandler( );
}

const char* Logger::Path( void )
{
	BuildPath( );
	return g_LogPath;
}

void Logger::Write( const char* fmt, ... )
{
	if( !fmt ) return;

	char szMsg[ 2048 ] = { 0 };

	va_list ap;
	va_start( ap, fmt );
	int len = _vsnprintf_s( szMsg, sizeof( szMsg ) - 1, _TRUNCATE, fmt, ap );
	va_end( ap );

	if( len < 0 ) len = ( int )sizeof( szMsg ) - 1; // truncated
	szMsg[ len ] = '\0';

	// strip trailing CRLF (printconsole already terminates its lines)
	while( len > 0 && ( szMsg[ len - 1 ] == '\n' || szMsg[ len - 1 ] == '\r' ) )
		szMsg[ --len ] = '\0';

	SYSTEMTIME st;
	GetLocalTime( &st );

	char szLine[ 2304 ];
	int n = wsprintfA( szLine, "[%02d:%02d:%02d.%03d] %s\r\n",
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, szMsg );

	if( n <= 0 ) return;

	EnsureCs( );
	EnterCriticalSection( &g_LogCs );
	EnsureOpen( );
	if( g_hLogFile != INVALID_HANDLE_VALUE )
	{
		DWORD dwWritten = 0;
		WriteFile( g_hLogFile, szLine, ( DWORD )n, &dwWritten, NULL );
	}
	LeaveCriticalSection( &g_LogCs );

	OutputDebugStringA( szLine );
}

void Logger::Shutdown( void )
{
	RemoveCrashHandler( ); // handler lives inside this DLL - must go away first

	if( !g_bCsInit ) return;

	EnterCriticalSection( &g_LogCs );
	if( g_hLogFile != INVALID_HANDLE_VALUE )
	{
		CloseHandle( g_hLogFile );
		g_hLogFile = INVALID_HANDLE_VALUE;
	}
	LeaveCriticalSection( &g_LogCs );
}
