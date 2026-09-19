#include "Main.h"
#include "detours.h"

typedef void ( *CL_Move_t )( float, bool );
CL_Move_t _CL_Move;

void Hooked_CL_Move( float accumulated_extra_samples, bool bFinalTick )
{
	// doubletap: shift FIRST, normal move LAST.
	// The held shot (CreateMove chokes it via g_bSendPacket) sits in the netchannel
	// backlog. Re-running CL_Move with g_DTInBurst re-appends the shifted attack
	// commands (all choked too), then the normal move below flushes EVERYTHING as
	// ONE datagram - that single packet is what makes the server fire twice.
	// The burst must run BEFORE the normal move: the move re-arms CreateMove which
	// resets g_DTAttack and releases the hold, so a burst checked after it never ran.
	if( !g_CVars.Miscellaneous.Speedhack && g_CVars.Miscellaneous.DoubleTap && g_DTAttack && !g_DTInBurst )
	{
		int ticks = g_DTTicks; // per-shot shift, computed at fire time in CreateMove
		if( ticks < 2 ) ticks = 2; if( ticks > 16 ) ticks = 16;
		if( g_DTCharge >= ticks )
		{
			g_bCL_Move = true;
			g_DTInBurst = true;
			g_bSendPacket = false; // every burst re-call chokes into the backlog
			g_DTForceLeft = ticks;
			g_DTForceIdx = 1;
			for( int i = 0; i < ticks; i++ ) _CL_Move( accumulated_extra_samples, bFinalTick );
			g_DTInBurst = false;
			g_DTForceLeft = 0;
			g_bSendPacket = true; // release: the normal move below flushes backlog in one packet
			g_DTAttack = false;
			g_DTCharge = 0;
			printconsole( "[Awesware] DT burst x%d\n", ticks );
		}
		else if( g_DTCharge < 16 ) g_DTCharge++;
	}
	else if( g_CVars.Miscellaneous.Speedhack && GetAsyncKeyState( SpeedhackKeyVK( g_CVars.Miscellaneous.SpeedhackKey ) ) )
	{
		g_bCL_Move = true;
		for( int i = 0; i <= g_CVars.Miscellaneous.SpeedhackValue; i++ ) _CL_Move( accumulated_extra_samples, bFinalTick );
	}
	else if( g_CVars.Miscellaneous.DoubleTap )
	{
		if( g_DTCharge < 16 ) g_DTCharge++;
	}
	else g_DTCharge = 0;

	// normal move: re-arms g_bSendPacket via CreateMove, and its SendDatagram
	// releases the held shot + shift commands together (the actual double tap)
	_CL_Move( accumulated_extra_samples, bFinalTick );

	g_bCL_Move = false;

	if( !g_DTInBurst ) LuaAPI::Tick( ); // lua on_tick (skipped during burst re-sends)
}

void CL_Move( void )
{
	_CL_Move = ( CL_Move_t ) DetourFunction( ( PBYTE ) ( ( DWORD ) BASE_ENGINE + 0x42510 ), ( PBYTE ) Hooked_CL_Move );
}
