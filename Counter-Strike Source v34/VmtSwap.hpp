#pragma once

#include "ScopedMemProtect.hpp"

namespace Memory
{
	// Подмена таблицы виртуальных функций объекта.
	//
	// Гарантии (раньше их не было и это роняло игру при неудачной инжекции):
	//   * Apply() можно звать только один раз до Release();
	//   * Restore()/Replace()/Hook() ничего не делают, если Apply() не прошёл;
	//   * Hook() проверяет индекс — запись вне скопированной таблицы невозможна;
	//   * Release() идемпотентен.
	class VmtSwap
	{
	public:
		VmtSwap();

		bool Apply( void* pInstance );
		void Release();

		void Replace();
		void Restore();

		void Hook( void* pHooked, std::size_t nIndex );

		bool IsApplied() const;

		template< typename T >
		inline T VCall( const std::size_t nIndex )
		{
			return ( T )( m_pBackupVmt[ nIndex ] );
		}

	private:
		std::uintptr_t**					m_ppInstance;
		std::uintptr_t*						m_pBackupVmt;
		std::unique_ptr< std::uintptr_t[ ] >	m_pCustomVmt;
		std::size_t							m_nSize;
		bool								m_bApplied;
	};
}
