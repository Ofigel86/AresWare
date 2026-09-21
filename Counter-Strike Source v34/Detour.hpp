#pragma once

#include "ScopedMemProtect.hpp"

namespace Memory
{
	enum
	{
		OPCODE_NOP = 0x90,
		OPCODE_JMP = 0xE9,
	};

	// Код-патч: 5-байтовый относительный jmp (E9) + NOP до nSize.
	//
	// Внимание: это x86-патч (32-битный target). Сборка проекта — Win32/x86,
	// где sizeof( std::uintptr_t ) == 4, поэтому запись 4-байтового смещения
	// корректа. Для x64 нужна другая реализация.
	//
	// Гарантии (добавлено): Apply() проверяет параметры и повторный вызов,
	// Release() идемпотентен, деструктор безопасен.
	class Detour
	{
	public:
		Detour();
		~Detour();

		bool Apply( std::uintptr_t uTarget, std::uintptr_t uHooked, std::size_t nSize );
		void Release();

		bool IsApplied() const;
		const std::uintptr_t GetReturnLocation() const;

	private:
		std::uintptr_t						m_uTarget;
		std::uintptr_t						m_uJumpBack;
		std::uintptr_t						m_uHooked;
		std::size_t							m_nSize;
		bool								m_bApplied;
		std::unique_ptr< std::uint8_t[ ] >	m_pBackupCode;
	};
}
