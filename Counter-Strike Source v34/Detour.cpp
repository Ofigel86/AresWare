#include "Detour.hpp"
#include "Debug.hpp"
#include "Crypt.hpp"

#include <cstring>

namespace Memory
{
	// Минимальный размер патча: 1 байт (E9) + 4 байта относительного смещения.
	static const std::size_t kMinDetourSize = 5;

	Detour::Detour()
		:	m_uTarget( 0 ),
			m_uJumpBack( 0 ),
			m_uHooked( 0 ),
			m_nSize( 0 ),
			m_bApplied( false ),
			m_pBackupCode( nullptr )
	{

	}

	Detour::~Detour()
	{
		Release();
	}

	bool Detour::Apply( std::uintptr_t uTarget, std::uintptr_t uHooked, std::size_t nSize )
	{
		if( !uTarget || !uHooked )
		{
			LOG( XorStr( "[Detour::Apply] Parameters are invalid (target 0x%X, hooked 0x%X)." ), ( unsigned )uTarget, ( unsigned )uHooked );
			return false;
		}

		if( nSize < kMinDetourSize )
		{
			LOG( XorStr( "[Detour::Apply] Patch size %u is too small (min %u)." ), ( unsigned )nSize, ( unsigned )kMinDetourSize );
			return false;
		}

		if( m_bApplied )
		{
			LOG( XorStr( "[Detour::Apply] Already applied at 0x%X." ), ( unsigned )m_uTarget );
			return false;
		}

		auto pBackupCode = std::make_unique< std::uint8_t[ ] >( nSize );

		if( !pBackupCode )
		{
			LOG( XorStr( "[Detour::Apply] Allocation failed (%u bytes)." ), ( unsigned )nSize );
			return false;
		}

		memcpy( pBackupCode.get(), ( const void* )uTarget, nSize );

		ScopedMemProtect ProtectGuard( uTarget, nSize, PAGE_EXECUTE_READWRITE );

		auto uRelative = ( uHooked - uTarget ) - kMinDetourSize;

		*( std::uint8_t* )( uTarget ) = OPCODE_JMP;
		*( std::uint32_t* )( uTarget + 1 ) = ( std::uint32_t )uRelative;

		for( std::size_t i = kMinDetourSize; i < nSize; i++ )
			*( std::uint8_t* )( uTarget + i ) = OPCODE_NOP;

		m_uTarget = uTarget;
		m_uHooked = uHooked;
		m_nSize = nSize;
		m_pBackupCode = std::move( pBackupCode );
		m_uJumpBack = uTarget + nSize;
		m_bApplied = true;

		return true;
	}

	void Detour::Release()
	{
		if( !m_bApplied )
			return;

		ScopedMemProtect ProtectGuard( m_uTarget, m_nSize, PAGE_EXECUTE_READWRITE );

		memcpy( ( void* )m_uTarget, m_pBackupCode.get(), m_nSize );

		m_uTarget = 0;
		m_uJumpBack = 0;
		m_uHooked = 0;
		m_nSize = 0;
		m_pBackupCode.reset();
		m_bApplied = false;
	}

	bool Detour::IsApplied() const
	{
		return m_bApplied;
	}

	const std::uintptr_t Detour::GetReturnLocation() const
	{
		return m_uJumpBack;
	}
}
