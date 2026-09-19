#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <string>
#include <vector>

// config system: named .ini configs + lua scripts under C:/Awesware
class CConfig
{
public:
	void SetModule( HMODULE hModule );

	// legacy single-config API (default.ini)
	void Load( void );
	void Save( void );

	// named config manager
	static void EnsureDirs( void );                    // creates C:\Awesware\{configs,scripts} + migrates old config.cfg
	static std::string RootDir( void );                // C:/Awesware
	static std::string ConfigDir( void );              // C:/Awesware/configs
	static std::string ScriptDir( void );              // C:/Awesware/scripts
	static std::string SanitizeName( const char* name );

	bool SaveAs( const char* name );
	bool LoadFrom( const char* name );
	bool Delete( const char* name );
	void List( std::vector<std::string>& out );        // config names without extension

	// core: read/write a full profile to an arbitrary ini path
	bool ReadProfile( const std::string& path );
	bool WriteProfile( const std::string& path );

private:
	static HMODULE m_hModule;
};

extern CConfig g_Config;

#endif // __CONFIG_H__
