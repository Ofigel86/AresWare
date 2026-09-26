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
	void Init( void );

	// Appends one formatted line to the log (thread-safe).
	void Write( const char* fmt, ... );

	// Closes the log file (called on unload).
	void Shutdown( void );

	// Full path of the log file (valid after first Init/Write).
	const char* Path( void );
}

#endif // _LOGGER_H_
