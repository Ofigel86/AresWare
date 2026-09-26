#include "Main.h"
#include <algorithm>

bool bInGame = false;

std::vector< IMaterial* > vecSkyTextures;
std::vector< IMaterial* > vecPlayerTextures;
std::vector< IMaterial* > vecWeaponTextures;
std::vector< IMaterial* > vecMapTextures;

void SetAsusWalls( float flAlpha )
{
	for( unsigned int i = 0; i < vecMapTextures.size( ); i++ )
	{
		vecMapTextures[ i ]->ColorModulate( .5f, .5f, .5f );
		vecMapTextures[ i ]->AlphaModulate( flAlpha );
	}
}

void SetSkyChams( bool bDraw )
{
	for( unsigned int i = 0; i < vecSkyTextures.size( ); i++ )
	{
		if( bDraw ) vecSkyTextures[ i ]->ColorModulate( 0.0001f, 0.0001f, 0.0001f );
	}
}

void SetPlayerChams( bool bWallHack, bool bWireFrame )
{
	for( unsigned int i = 0; i < vecPlayerTextures.size( ); i++ )
	{
		vecPlayerTextures[ i ]->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, bWallHack );
		vecPlayerTextures[ i ]->SetMaterialVarFlag( MATERIAL_VAR_ZNEARER, bWallHack );
		vecPlayerTextures[ i ]->SetMaterialVarFlag( MATERIAL_VAR_WIREFRAME, bWireFrame ); 
	}
}

void SetWeaponChams( bool bWireFrame, bool bNoHand )
{
	for( unsigned int i = 0; i < vecWeaponTextures.size( ); i++ )
	{
		if( strstr( vecWeaponTextures[ i ]->GetName( ), /*hand*/XorStr<0x10,5,0x3D1540CD>("\x78\x70\x7C\x77"+0x3D1540CD).s ) )
		{
			vecWeaponTextures[ i ]->SetMaterialVarFlag( MATERIAL_VAR_NO_DRAW, bNoHand );
			vecWeaponTextures[ i ]->SetMaterialVarFlag( MATERIAL_VAR_WIREFRAME, bWireFrame );
		}
	}
}

typedef IMaterial* ( __stdcall *FindMaterial_t )( const char*, const char*, bool, const char* );
IMaterial* __stdcall Hooked_FindMaterial( const char* pMaterialName, const char* pTextureGroupName, bool complain, const char* pComplainPrefix )
{
	IMaterial* pTemp = MaterialSystemVMT->Function< FindMaterial_t >( 27 )( pMaterialName, pTextureGroupName, complain, pComplainPrefix );

	if( pMaterialName && pTextureGroupName )
	{
		if( g_pEngineClient->IsInGame( ) )
		{
			if( g_CVars.Visuals.ASUS > 0.8 ) g_CVars.Visuals.ASUS = 1.0;
			SetAsusWalls( g_CVars.Visuals.ASUS );
			if( g_CVars.Visuals.ASUS < 0.9 ) g_CVars.Visuals.NoSky = true;
			SetSkyChams( g_CVars.Visuals.NoSky );
			SetWeaponChams( false, g_CVars.Visuals.NoHands );
			bInGame = true;
		}
		else
		{
			if( bInGame )
			{
				vecSkyTextures.clear( );
				SetAsusWalls( 1.0f );
				SetSkyChams( false );
				SetPlayerChams( false, false );
				SetWeaponChams( false, false );
				vecMapTextures.clear( );
				bInGame = false;
			}

			//Dedupe: while sitting in the menu FindMaterial fires repeatedly
			//for the same materials and unbounded push_backs leaked memory.
			auto pushUnique = []( std::vector< IMaterial* > &vec, IMaterial* pMat )
			{
				if( pMat && std::find( vec.begin( ), vec.end( ), pMat ) == vec.end( ) )
					vec.push_back( pMat );
			};

			if( strstr( pTextureGroupName, "SkyBox textures" ) ) pushUnique( vecSkyTextures, pTemp );
			if( strstr( pTextureGroupName, "World" ) && !strstr( pMaterialName, "models\\player" ) ) pushUnique( vecMapTextures, pTemp );
			if( strstr( pTextureGroupName, "Model texture" ) && strstr( pMaterialName, "models\\player" ) ) pushUnique( vecPlayerTextures, pTemp );
			if( strstr( pTextureGroupName, "Model textures" ) && strstr( pMaterialName, "models\\weapons" ) ) pushUnique( vecWeaponTextures, pTemp );
		}
	}

	return pTemp;
}