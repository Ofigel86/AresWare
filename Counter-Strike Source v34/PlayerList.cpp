#include "PlayerList.hpp"
#include "Source.hpp"
#include "Player.hpp"

namespace Feature
{
	Player::Player()
		: m_player( nullptr )
		, m_index( 0 )
		, m_pitch( 0 )
		, friends( 0 )
		, m_yaw( 0 )
		, m_spawn_time( 0 )
	{
		// m_name раньше не инициализировалось вообще: до первого успешного
		// GetPlayerInfo (и после смены карты, когда слот переиспользуется)
		// strcmp/ESP читали мусор из памяти.
		m_name[ 0 ] = '\0';
	}

	void Player::Update( int index )
	{
		m_index = index;

		m_player = ToCSPlayer( Source::m_pEntList->GetBaseEntity( m_index ) );

		if( !m_player )
		{
			m_spawn_time = 0;
			m_name[ 0 ] = '\0';
			return;
		}

		player_info_t data;
		if( Source::m_pEngine->GetPlayerInfo( m_index, &data ) )
			strcpy_s( m_name, sizeof( m_name ), data.name );
		else
			m_name[ 0 ] = '\0';	// иначе в ESP остаётся имя игрока с прошлой карты

		if( m_player->m_lifeState() != LIFE_ALIVE )
		{
			m_spawn_time = 0;
			return;
		}

		m_spawn_time++;
	}

	void PlayerList::OnCreateMove()
	{
		// Индексы игроков в движке Source начинаются с 1: слот 0 — это
		// worldspawn, обновлять его не нужно (раньше цикл шёл с нуля).
		int size = Source::m_pEngine->GetMaxClients();

		if( size < 1 )
			return;

		if( size > kMaxPlayers )
			size = kMaxPlayers;

		for( int i = 1; i <= size; i++ )
			m_players[ i ].Update( i );
	}

	Player* PlayerList::GetPlayer( int index )
	{
		// Без проверки индекса любой вызов с отрицательным индексом или
		// индексом > 64 выходил за границы m_players[65].
		if( index < 0 || index > kMaxPlayers )
			return nullptr;

		return &m_players[ index ];
	}

	Player* PlayerList::GetPlayer( const char* name )
	{
		if( !name || name[ 0 ] == '\0' )
			return nullptr;

		for( int i = 1; i <= kMaxPlayers; i++ )
		{
			if( m_players[ i ].m_name[ 0 ] == '\0' )
				continue;

			if( strcmp( m_players[ i ].m_name, name ) == 0 )
				return &m_players[ i ];
		}

		return nullptr;
	}
}
