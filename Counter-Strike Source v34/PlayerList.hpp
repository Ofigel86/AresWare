#pragma once

#include "Valve.hpp"

namespace Feature
{
	// Максимальное число игровых слотов (индексы 1..kMaxPlayers).
	static const int kMaxPlayers = 64;

	class Player
	{
	public:
		Player();

		void Update( int index );

	public:
		C_CSPlayer* m_player;

		int m_index;
		int m_pitch;
		int friends;
		int m_yaw;
		int m_spawn_time;
		
		char m_name[ 32 ];
	};

	class PlayerList
	{
	public:
		void OnCreateMove();

		Player* GetPlayer( int index );
		Player* GetPlayer( const char* name );

	private:
		Player m_players[ kMaxPlayers + 1 ];
	};
}