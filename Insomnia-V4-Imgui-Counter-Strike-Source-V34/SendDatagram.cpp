// BUILD MARKER r9 (2026-09-18): NATIVE CHOKE - CanPacket(slot 52) gate; engine skips CL_SendMove itself (SetChoked) so fakelag/DT backlog never loses CLC_Move.
#include "Main.h"
#include "VMTHook.h"

// Real packet choke for v34: the old CS:GO-style bSendPacket-in-BL trick does
// nothing on this engine (CreateMove has no such channel), so every command
// was always sent - fakelag/AA-desync/DT-burst-hold never actually choked.
// Dropping the datagram here is the choke: the engine keeps all unacked
// commands and releases them in one packet on the next send.

// v34 INetChannel VMT: 22 INetChannelInfo slots + dtor + N methods.
// VERIFIED against engine.dll (md5 dc55cf8d9fe078f076205cdb24ad5eb2): the CNetChan
// vtable (RTTI .?AVCNetChan@@, 60 entries) has SendDatagram at slot 42 - slot 41
// is SetChoked (argless counter bump), 43 is Transmit. CL_Move@0x42510 calls it
// as "push 0; call [netchan+0xA8]". Hooking 41 never choked anything.
// The original pointer is validated against engine.dll before hooking.
#define NETCHAN_SENDDATAGRAM_INDEX 42

// NATIVE CHOKE (r9): in this engine CL_Move gates the send with CanPacket():
//   if ( netchan->CanPacket() ) CL_SendMove(); else netchan->SetChoked();
// When CanPacket returns false the engine ITSELF skips CL_SendMove - the CLC_Move
// blob is never queued and the usercmds wait in cl.commands; on the next send
// CL_SendMove packs the whole backlog (LastOutgoingCommand+1..current) itself.
// Dropping SendDatagram instead could orphan the already-queued CLC_Move, which
// is why DT held shots and fakelag never reached the server.
// Slot from SDK header layout (rom4s/hl2sdk-ep1c public/inetchannel.h): the same
// numbering that puts SetChoked at 41 and SendDatagram at 42 (md5-verified) puts
// CanPacket at 52 and ends the vtable exactly at entry 59 - layout self-consistent.
#define NETCHAN_CANPACKET_INDEX 52

typedef int( __thiscall* SendDatagram_t )( void*, void* );
static SendDatagram_t oSendDatagram = nullptr;
typedef bool( __thiscall* CanPacket_t )( void* );
static CanPacket_t oCanPacket = nullptr;
bool g_CanPacketHooked = false;
static void* g_NetchanTried = nullptr; // last netchannel we attempted (no retry spam)
bool g_NetchanHooked = false;

static bool PtrInEngine( void* p )
{
	HMODULE eng = GetModuleHandleA( "engine.dll" );
	if( !eng || !p ) return false;
	PIMAGE_DOS_HEADER dos = ( PIMAGE_DOS_HEADER ) eng;
	if( dos->e_magic != IMAGE_DOS_SIGNATURE ) return false;
	PIMAGE_NT_HEADERS nt = ( PIMAGE_NT_HEADERS )( ( BYTE* ) eng + dos->e_lfanew );
	if( nt->Signature != IMAGE_NT_SIGNATURE ) return false;
	DWORD base = ( DWORD ) eng, size = nt->OptionalHeader.SizeOfImage;
	return ( ( DWORD ) p >= base && ( DWORD ) p < base + size );
}

int g_iChokedTicks = 0; // live choke counter for indicators

int __fastcall Hooked_SendDatagram( void* netchan, void* edx, void* datagram )
{
	static bool s_alive = false;
	if( !s_alive ) { s_alive = true; printconsole( "[Awesware] choke hook alive (datagram=%p)\n", datagram ); }
	if( g_CanPacketHooked ) return oSendDatagram( netchan, datagram ); // native gate owns deferral - never mix mechanisms
	if( datagram ) return oSendDatagram( netchan, datagram ); // demo path: never touch
	if( !g_pEngineClient->IsInGame( ) ) return oSendDatagram( netchan, datagram );
	if( !g_bSendPacket ) { g_iChokedTicks++; return 0; } // FALLBACK choke (only if CanPacket slot was rejected)
	g_iChokedTicks = 0; // real send: backlog flushed
	return oSendDatagram( netchan, datagram );
}

bool __fastcall Hooked_CanPacket( void* netchan, void* edx )
{
	if( g_pEngineClient && g_pEngineClient->IsInGame( ) && !g_bSendPacket )
	{
		g_iChokedTicks++; // native choke: engine calls SetChoked() and skips CL_SendMove itself
		return false;
	}
	g_iChokedTicks = 0;
	return oCanPacket( netchan );
}

void Netchan_UpdateHook( void )
{
	void* nc = nullptr;
	if( g_pEngineClient && g_pEngineClient->IsInGame( ) )
		nc = ( void* ) g_pEngineClient->GetNetChannelInfo( );
	if( nc == g_NetchanTried ) return; // already handled (hooked or rejected)
	g_NetchanTried = nc;
	g_NetchanHooked = false;
	g_CanPacketHooked = false;
	if( !nc ) return;

	// NOTE: the previous CVMTHook is intentionally leaked, never unhooked:
	// its netchannel object is already dead after a disconnect, and touching
	// freed memory would crash. ~200 bytes per server join, harmless.
	CVMTHook* vmt = new CVMTHook( nc );
	void* orig = vmt->GetOriginalFunction( NETCHAN_SENDDATAGRAM_INDEX );
	if( !orig || !PtrInEngine( orig ) )
	{
		printconsole( "[Awesware] netchan hook rejected (bad vtable)\n" );
		return;
	}
	oSendDatagram = ( SendDatagram_t ) vmt->HookFunction( NETCHAN_SENDDATAGRAM_INDEX, ( void* ) Hooked_SendDatagram );
	g_NetchanHooked = true;
	printconsole( "[Awesware] netchan choke hook installed\n" );

	// r9: primary choke = native CanPacket gate (slot 52)
	void* origCP = vmt->GetOriginalFunction( NETCHAN_CANPACKET_INDEX );
	if( origCP && PtrInEngine( origCP ) )
	{
		oCanPacket = ( CanPacket_t ) vmt->HookFunction( NETCHAN_CANPACKET_INDEX, ( void* ) Hooked_CanPacket );
		g_CanPacketHooked = true;
		printconsole( "[Awesware] native choke gate hooked (CanPacket=52)\n" );
	}
	else
	{
		printconsole( "[Awesware] CanPacket slot rejected - fallback to datagram drop\n" );
	}
}
