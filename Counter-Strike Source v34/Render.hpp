#pragma once

#include "Valve.hpp"

namespace Feature
{
	using Direct3D9::Color;

	class Render
	{
	public:
		Render();


		void		OnDrawModel( void* ecx, ModelRenderInfo_t* info );

		IMaterial*	CreateMaterial( bool bVertexLit, bool bIgnoreZ, bool bWireframe = false, bool bTranslucent = false, bool bAdditive = false );
		void		ForceMaterial( const Color& color, IMaterial* mat, bool mod = true );

	private:
		IMaterial*	m_pVertexIn;
		IMaterial*	m_pVertexOut;

		IMaterial*	m_pMatIn;
		IMaterial*	m_pMatOut;

		IMaterial*	m_pWireIn;
		IMaterial*	m_pWireOut;
		IMaterial*	m_pGlassIn;
		IMaterial*	m_pGlassOut;
		IMaterial*	m_pGlowIn;
		IMaterial*	m_pGlowOut;
		IMaterial*	m_pGhostIn;
		IMaterial*	m_pGhostOut;

		IMaterial*	m_pOut;
		IMaterial*	m_pIn;
	};
}