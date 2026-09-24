#include "Main.h"

CUserCmd* __stdcall Hooked_GetUserCmd( int sequence_number )
{
	if( !g_pInput ) return NULL;

	//m_nVerifiedNewCommands / command buffer at +0xC4, 90 slots.
	//Guard against negative or out-of-range sequence numbers so the
	//pointer arithmetic can never index outside the array.
	int index = sequence_number % 90;
	if( index < 0 ) index += 90;

	CUserCmd* pBuf = *( CUserCmd** )( ( DWORD ) g_pInput + 0xC4 );
	if( !pBuf ) return NULL;

	return &pBuf[ index ];
}
