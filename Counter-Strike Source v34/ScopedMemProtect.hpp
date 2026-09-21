#pragma once

#include "Crypt.hpp"
#include "Debug.hpp"

#include <windows.h>

#include <cstdint>
#include <memory>

namespace Memory
{
	// RAII-обёртка над VirtualProtect: восстанавливает старую защиту страницы
	// только если изменение действительно удалось. Если VirtualProtect не
	// сработал — просто ничего не делаем (раньше в m_dwProtect попадал мусор,
	// и деструктор мог снять защиту с чужой страницы).
	class ScopedMemProtect
	{
	public:
		template< typename T >
		ScopedMemProtect( T tAddress, std::size_t nSize, DWORD dwProtect )
			:	m_pAddress( ( void* )tAddress ),
				m_nSize( nSize ),
				m_dwProtect( 0 ),
				m_bActive( false )
		{
			if( !tAddress || !nSize )
				return;

			DWORD dwOld = 0;

			if( VirtualProtect( ( LPVOID )tAddress, nSize, dwProtect, &dwOld ) )
			{
				m_dwProtect = dwOld;
				m_bActive = true;
			}
			else
			{
				DPRINT( XorStr( "[ScopedMemProtect] VirtualProtect failed at 0x%X (size %u)" ), ( unsigned )tAddress, ( unsigned )nSize );
			}
		}

		~ScopedMemProtect()
		{
			if( !m_bActive )
				return;

			DWORD dwDummy = 0;
			VirtualProtect( m_pAddress, m_nSize, m_dwProtect, &dwDummy );
		}

		ScopedMemProtect( const ScopedMemProtect& ) = delete;
		ScopedMemProtect& operator = ( const ScopedMemProtect& ) = delete;

	private:
		void*		m_pAddress;
		std::size_t	m_nSize;
		DWORD		m_dwProtect;
		bool		m_bActive;
	};
}
