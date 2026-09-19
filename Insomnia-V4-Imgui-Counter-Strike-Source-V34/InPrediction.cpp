#include "Main.h"

typedef bool( __thiscall* InPrediction_t )( void* );
bool __fastcall Hooked_InPrediction( void* ptr, int edx )
{
	bool ret = PredictionVMT->Function< InPrediction_t >( 8 )( ptr );
	return ret;
}

extern "C" void __stdcall Gate_SetViewAngles( PFLOAT pfAngles, BaseEntity* ent, DWORD dwReturnAddress )
{
	PFLOAT pfPunchAngle;
	
	if( dwReturnAddress == 0x240192BE )
	{
		if( ent && ent->entindex( ) == g_pEngineClient->GetLocalPlayer( ) && pfAngles )
		{
			pfPunchAngle = ( PFLOAT )( ( DWORD ) ent + 0xBB0 );
			
			if( g_CVars.Visuals.NoVisualRecoil )
			{
				pfAngles[ 0 ] -= pfPunchAngle[ 0 ];
				pfAngles[ 1 ] -= pfPunchAngle[ 1 ];
				pfAngles[ 2 ] -= pfPunchAngle[ 2 ];
			}
		}
	}

	
}

extern "C" { BOOL g_bSetViewAngles; }

#ifdef _MSC_VER
__declspec ( naked ) void Hooked_SetViewAngles( void )
{
	__asm
	{
		XOR EAX, EAX
		
		MOV AL, BYTE PTR [ ECX + 0x8 ]
		MOV [ g_bSetViewAngles ], EAX
		
		MOV EAX, DWORD PTR [ ESP ]
		PUSH EAX
		PUSH ESI
		PUSH EDI
		CALL Gate_SetViewAngles
		
		MOV EAX, [ g_bSetViewAngles ]
		RETN
	}
}
#else
// mingw: replicate the MSVC naked thunk 1:1 (file-scope asm).
__asm__(
	".text\n"
	".globl _Hooked_SetViewAngles\n"
	"_Hooked_SetViewAngles:\n\t"
	"xorl %eax, %eax\n\t"
	"movb 8(%ecx), %al\n\t"
	"movl %eax, _g_bSetViewAngles\n\t"
	"movl (%esp), %eax\n\t"
	"pushl %eax\n\t"
	"pushl %esi\n\t"
	"pushl %edi\n\t"
	"call _Gate_SetViewAngles@12\n\t"
	"movl _g_bSetViewAngles, %eax\n\t"
	"ret\n"
);
#endif