#pragma once

#include "Valve.hpp"

namespace Feature
{
	namespace Resolver
	{
		// Returns the corrected eye angle. Must be called from the
		// DT_CSPlayer::m_angEyeAngles recv proxies (player list "Auto" modes).
		// The value written back drives UpdateClientSideAnimation() ->
		// pose parameters -> bones -> hitboxes, so correcting it here makes
		// our hitboxes match the ones the server hit-tests against.
		auto ResolvePitch( C_CSPlayer* player, float raw_pitch ) -> float;
		auto ResolveYaw( C_CSPlayer* player, float raw_yaw ) -> float;

		// Shot / hit / death / round feedback. Forward every game event here.
		auto OnGameEvent( IGameEvent* game_event ) -> void;

		auto Reset( int index ) -> void;
		auto ResetAll() -> void;

		// Short AA-mode text for ESP ("" when there is nothing to show).
		auto GetText( int index ) -> const char*;

		// m_flPoseParameter element proxy: captures the server-computed
		// pose parameters before our client re-animates over them.
		void DT_CSPlayer_m_flPoseParameter( const CRecvProxyData* pData, void* pStruct, void* pOut );
	}
}
