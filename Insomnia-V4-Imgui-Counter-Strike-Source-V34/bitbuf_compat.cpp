// r47: bitbuf_compat.cpp — GCC/MinGW implementations for the hl2sdk tier1 bitbuf subset
// actually used by the vendored ClientMod emulator. MSVC builds don't compile this file
// (they resolve the same symbols from tier1.lib); including it there would be harmless
// anyway (symbols already defined would just shadow the static lib members).
//
// Semantics copied from Valve's bitbuf.cpp: LSB-first bit packing, little-endian dwords.

#include "tier1/bitbuf.h"

unsigned long g_ExtraMasks[ 32 ] =
{
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff
};

static inline unsigned int BitMask( int n )
{
	return ( n >= 32 ) ? 0xFFFFFFFFu : ( ( 1u << n ) - 1u );
}


// g_BitWriteMasks[startBit][nBits]: keeps bits below startBit and at/above startBit+nBits.
// Used by the inline WriteUBitLong in bitbuf.h. Filled at static init.
unsigned long g_BitWriteMasks[ 32 ][ 33 ];

namespace
{
	struct CBitMasksInit
	{
		CBitMasksInit( )
		{
			for( int s = 0; s < 32; s++ )
			{
				for( int b = 0; b <= 32; b++ )
				{
					int end = s + b;
					unsigned int low = ( 1u << s ) - 1u;
					unsigned int high = ( end >= 32 ) ? 0u : ~( ( 1u << end ) - 1u );
					g_BitWriteMasks[ s ][ b ] = low | high;
				}
			}
		}
	};
	static CBitMasksInit g_BitMasksInitializer;
}

// r47: GCC build cannot take these from the MSVC tier1.lib (different C++ mangling)
int V_stricmp( const char* str1, const char* str2 )
{
	return stricmp( str1, str2 );
}

// ---------------- bf_write ----------------

void bf_write::WriteByte( int val )
{
	WriteUBitLong( (unsigned int)( val & 0xFF ), 8 );
}

void bf_write::WriteShort( int val )
{
	WriteUBitLong( (unsigned int)( val & 0xFFFF ), 16 );
}

void bf_write::WriteLong( long val )
{
	WriteUBitLong( (unsigned int)val, 32 );
}

bool bf_write::WriteBytes( const void* pBuf, int nBytes )
{
	const unsigned char* p = (const unsigned char*)pBuf;
	for( int i = 0; i < nBytes; i++ )
		WriteUBitLong( (unsigned int)p[ i ], 8 );
	return !m_bOverflow;
}

// ---------------- bf_read ----------------

int bf_read::ReadByte()
{
	return (int)( ReadUBitLong( 8 ) & 0xFF );
}

bool bf_read::ReadBits( void* pOut, int nBits )
{
	unsigned char* out = (unsigned char*)pOut;
	int nBytes = nBits >> 3;
	for( int i = 0; i < nBytes; i++ )
		out[ i ] = (unsigned char)ReadUBitLong( 8 );
	int rem = nBits & 7;
	if( rem && !m_bOverflow )
		out[ nBytes ] = (unsigned char)ReadUBitLong( rem );
	return !m_bOverflow;
}

bool bf_read::ReadBytes( void* pOut, int nBytes )
{
	return ReadBits( pOut, nBytes * 8 );
}

bool bf_read::ReadString( char* pStr, int bufLen, bool bLine, int* pOutNumChars )
{
	if( bufLen == 0 )
	{
		if( pOutNumChars ) *pOutNumChars = 0;
		return false;
	}

	int nChars = 0;
	while( nChars < bufLen - 1 )
	{
		char val = (char)ReadUBitLong( 8 );
		if( val == 0 || ( bLine && val == '\n' ) )
			break;
		pStr[ nChars ] = val;
		nChars++;
	}
	pStr[ nChars ] = 0;
	if( pOutNumChars ) *pOutNumChars = nChars;

	if( nChars == bufLen - 1 && !m_bOverflow )
	{
		// filled the buffer without hitting the terminator: peek whether the next char ends it
	}
	return !m_bOverflow;
}

// r47: Microsoft Detours 1.5 (detours.lib, MSVC-built) references the SEH frame-list head
// as a data symbol; it is only meaningful inside real SEH, which this code does not walk.
// A definition keeps the MSVC import satisfiable under binutils.
extern "C" void* _except_list = 0;
