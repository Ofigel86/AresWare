#pragma once

#include "Valve.hpp"

namespace Feature
{

class HitMarker
{
public:
	HitMarker() = default;

public:
	static HitMarker& Instance()
	{
		static HitMarker instance = { };
		return instance;
	}

public:
	void FireGameEvent( IGameEvent* game_event );
	void Present();

protected:
	HitMarker( const HitMarker& ) = delete;
	HitMarker& operator = ( const HitMarker& ) = delete;

private:
	bool m_render = false;
	Shared::Timer m_timer = { };
};

}