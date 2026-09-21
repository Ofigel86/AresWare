#include "VmtSwap.hpp"
#include "Debug.hpp"
#include "Crypt.hpp"

#include <cstring>

namespace Memory
{
	// Предохранитель от бесконечного обхода битой таблицы.
	static const std::size_t kMaxVmtSize = 2048;

	VmtSwap::VmtSwap()
		:	m_ppInstance( nullptr ),
			m_pBackupVmt( nullptr ),
			m_pCustomVmt( nullptr ),
			m_nSize( 0 ),
			m_bApplied( false )
	{

	}

	bool VmtSwap::Apply( void* pInstance )
	{
		if( !pInstance )
		{
			LOG( XorStr( "[VmtSwap::Apply] Invalid instance (nullptr)." ) );
			return false;
		}

		if( m_bApplied )
		{
			LOG( XorStr( "[VmtSwap::Apply] Already applied, call Release() first." ) );
			return false;
		}

		auto ppVmt = ( std::uintptr_t** )pInstance;
		auto pBackupVmt = *ppVmt;

		if( !pBackupVmt )
		{
			LOG( XorStr( "[VmtSwap::Apply] Instance has no vtable (0x%X)." ), ( unsigned )pInstance );
			return false;
		}

		std::size_t nSize = 0;

		while( nSize < kMaxVmtSize && pBackupVmt[ nSize ] )
			nSize++;

		if( nSize == 0 )
		{
			LOG( XorStr( "[VmtSwap::Apply] Vtable is empty (0x%X)." ), ( unsigned )pInstance );
			return false;
		}

		auto pCustomVmt = std::make_unique< std::uintptr_t[ ] >( nSize );

		if( !pCustomVmt )
		{
			LOG( XorStr( "[VmtSwap::Apply] Allocation failed (%u entries)." ), ( unsigned )nSize );
			return false;
		}

		memcpy( pCustomVmt.get(), pBackupVmt, nSize * sizeof( std::uintptr_t ) );

		m_ppInstance = ppVmt;
		m_pBackupVmt = pBackupVmt;
		m_pCustomVmt = std::move( pCustomVmt );
		m_nSize = nSize;
		m_bApplied = true;

		Replace();

		return true;
	}

	void VmtSwap::Release()
	{
		if( !m_bApplied )
			return;

		Restore();

		m_ppInstance = nullptr;
		m_pBackupVmt = nullptr;
		m_pCustomVmt.reset();
		m_nSize = 0;
		m_bApplied = false;
	}

	void VmtSwap::Replace()
	{
		if( !m_bApplied || !m_ppInstance || !m_pCustomVmt )
			return;

		*m_ppInstance = m_pCustomVmt.get();
	}

	void VmtSwap::Restore()
	{
		if( !m_bApplied || !m_ppInstance || !m_pBackupVmt )
			return;

		*m_ppInstance = m_pBackupVmt;
	}

	void VmtSwap::Hook( void* pHooked, std::size_t nIndex )
	{
		if( !m_bApplied || !m_pCustomVmt )
		{
			LOG( XorStr( "[VmtSwap::Hook] Not applied, hook at index %u ignored." ), ( unsigned )nIndex );
			return;
		}

		if( nIndex >= m_nSize )
		{
			LOG( XorStr( "[VmtSwap::Hook] Index %u is out of range (size %u)." ), ( unsigned )nIndex, ( unsigned )m_nSize );
			return;
		}

		if( !pHooked )
		{
			LOG( XorStr( "[VmtSwap::Hook] Invalid hook address at index %u." ), ( unsigned )nIndex );
			return;
		}

		m_pCustomVmt[ nIndex ] = ( std::uintptr_t )pHooked;
	}

	bool VmtSwap::IsApplied() const
	{
		return m_bApplied;
	}
}
