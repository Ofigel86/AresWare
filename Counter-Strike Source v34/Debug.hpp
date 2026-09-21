#pragma once

// ============================================================================
// Диагностика чита.
//
//   DPRINT(...) — только Debug-сборка (как и раньше, печатает в отладчик).
//   LOG(...)    — ВСЕГДА: пишет строку в <рабочая папка>\aresware.log и в
//                 OutputDebugString. Нужен, чтобы Release-сборка не выгружалась
//                 «молча» (раньше любая ошибка старта была невидимой — именно
//                 поэтому невозможно было понять, почему нет меню).
//
// Формат файла: обычный текстовый лог, в начале каждой строки — время в мс.
// ============================================================================

namespace Debug
{
	// Печать в отладчик (OutputDebugString). Безопасна к длинным строкам.
	extern void Print( const char* fmt, ... );

	// Открыть/закрыть файл лога. LogOpen можно звать один раз на старте.
	extern void LogOpen( const char* szPath );
	extern void LogClose();

	// Запись в лог: файл + отладчик. Работает без LogOpen (тогда только отладчик).
	extern void Log( const char* fmt, ... );
}

#ifdef _DEBUG
#define DPRINT( fmt, ... ) Debug::Print( fmt, __VA_ARGS__ )
#else
#define DPRINT( fmt, ... )
#endif

#define LOG( fmt, ... ) Debug::Log( fmt, __VA_ARGS__ )
