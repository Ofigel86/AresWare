#pragma once
#ifndef _LOGGER_H_
#define _LOGGER_H_

// Simple file logger. Writes "AresWare.log" next to the injected DLL.
// Thread-safe, adds a [HH:MM:SS.mmm] timestamp to every line,
// mirrors everything to OutputDebugStringA (visible in DbgView/DebugView).

namespace Logger
{
	// Opens (creates) the log file. Lazy: also happens on first Write().
	// Call once DllMain stored m_pszDllPath, otherwise falls back to CWD.
	// Also installs the crash handler (see below).
	void Init( void );

	// Appends one formatted line to the log (thread-safe).
	void Write( const char* fmt, ... );

	// Closes the log file and removes the crash handler (called on unload).
	void Shutdown( void );

	// Full path of the log file (valid after first Init/Write).
	const char* Path( void );

	// Marks the current init stage - reported by the crash handler.
	void SetStage( const char* stage );

	// Vectored exception handler: logs any fatal exception (code + fault
	// address + current stage) before the process dies, so a crash always
	// leaves a trace in AresWare.log. No __try/__except needed anywhere.
	void InstallCrashHandler( void );
	void RemoveCrashHandler( void );
}

#endif // _LOGGER_H_
