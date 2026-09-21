#include "Resolver.hpp"
#include "Player.hpp"
#include "Entity.hpp"
#include "PlayerList.hpp"
#include "Config.hpp"
#include "Source.hpp"

#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <algorithm>

// ============================================================================
// Resolver theory of operation (Counter-Strike: Source, engine EP1 / v34)
//
// Server, every tick (CCSPlayer::PostThink, no viewangle clamp on this
// engine generation):
//     m_angEyeAngles = EyeAngles();              // raw usercmd angles
//     m_PlayerAnimState->Update( eyeYaw, eyePitch );
//
// Anim state (CBasePlayerAnimState, LEGANIM_9WAY):
//   - moving: feet yaw converges to eye yaw at mp_feetyawrate
//   - body_yaw pose  = eyeYaw - feetYaw            (torso twist)
//   - body_pitch pose = clamp(eyePitch, -90, 90)   (look up/down)
//   - move_x/move_y   = strafe blend from eye-vs-gait delta
//
// Lag compensation restores origin + local angles (feet!) + sequences/cycles
// + layers from the record, but pose parameters stay CURRENT. Head hitbox
// orientation during the server hit-test is therefore roughly:
//     recordFeetYaw + currentBodyYaw, currentBodyPitch
//
// Client animates other players with networked m_angEyeAngles
// (C_CSPlayer::UpdateClientSideAnimation), so whatever these proxies write
// drives our pose params -> bones -> hitboxes. The resolver's job is to make
// that match the server hit-test state:
//
//   1. Classify the incoming angles (static / spin / jitter / fake pitch /
//      forward / choked) from a short per-player history.
//   2. Pick an ordered correction ladder for that class (raw angle first,
//      most likely real angles next).
//   3. Walk the ladder using real shot/hit feedback (weapon_fire /
//      player_hurt events): misses advance, hits lock the correction in.
//
// Pose monitor: the server computes m_flPoseParameter from the same eye
// angles it networks, so server pose normally mirrors the eye history.
// Capturing it (before our own client re-animates over it) gives an
// independent AA/desync readout: flapping server pose = active antiaim,
// and a server-pitch-vs-eye split (body_pitch != clamp(eye pitch)) is hard
// evidence that the server animated with different angles than networked.
// ============================================================================

namespace Feature
{
	namespace Resolver
	{
		namespace
		{
			constexpr int kMaxPlayers = 64;
			constexpr int kHistory = 8;
			constexpr int kPoseCount = 24;

			// studiohdr_t layout (Source 2007 SDK, public/studio.h): the
			// pose-parameter directory sits at fixed offsets. Verified at
			// runtime ('IDST' magic + sane count + resolvable names).
			constexpr int kStudioId = 0x54534449; // 'IDST'
			constexpr int kStudioPoseCountOffset = 300; // numlocalposeparameters
			constexpr int kStudioPoseIndexOffset = 304; // localposeparamindex
			constexpr int kPoseDescSize = 20; // mstudioposeparamdesc_t

			enum EPitchAA : int
			{
				Pitch_None,
				Pitch_Down,   // networked pitch looks down (+60..+140)
				Pitch_Up,     // networked pitch looks up (-140..-60)
				Pitch_Zero,   // networked pitch ~level (|p| < 25)
				Pitch_Fake,   // impossible pitch, near +/-180
				Pitch_Jitter  // pitch flips between updates
			};

			enum EYawAA : int
			{
				Yaw_None,     // normal mouse movement -> trust networked
				Yaw_Static,   // no movement -> desync/small-AA ladder
				Yaw_Spin,     // consistent rotation -> quadrant ladder
				Yaw_Jitter,   // direction flips -> median + small ladder
				Yaw_Forward,  // static and facing us -> backward first
				Yaw_Choke     // static with choked packets -> desync ladder
			};

			struct ResolveState
			{
				bool active = false;
				int userID = -1;
				float infoTime = -1.0f;
				char name[ 32 ] = {};

				float pitchHist[ kHistory ] = {};
				float yawHist[ kHistory ] = {};
				int pitchCount = 0;
				int yawCount = 0;
				float lastSim = 0.0f;
				float lastChokeTime = -10.0f;

				EPitchAA pitchAA = Pitch_None;
				EYawAA yawAA = Yaw_None;
				bool moving = false;
				bool poseMismatch = false;

				int ladderKey = -1;
				int step = 0;
				int shotsAtStep = 0;
				int hitsAtStep = 0;
				int shotsTotal = 0;
				bool hasBest = false;

				float lastYawCorr = 0.0f;
				float lastPitchOut = 0.0f;

				char text[ 64 ] = {};
			};

			ResolveState s_state[ kMaxPlayers + 1 ];

			// Server pose parameters captured from the m_flPoseParameter
			// recv proxies, before our client re-animates over them.
			float s_serverPose[ kMaxPlayers + 1 ][ kPoseCount ] = {};
			bool s_poseSeen[ kMaxPlayers + 1 ] = {};

			struct PoseIndices
			{
				int yaw = -2;   // -2 = unresolved, -1 = failed
				int pitch = -2;
				const model_t* model = nullptr;
			};

			PoseIndices s_poseIdx[ kMaxPlayers + 1 ];

			// Yaw correction ladders: offsets applied to the networked
			// (or median, for jitter) yaw. Index 0 is always the most
			// likely candidate for the detected class.
			static const float kYawSpin[]    = { 0.0f, 180.0f, 90.0f, -90.0f, 45.0f, -45.0f, 135.0f, -135.0f };
			static const float kYawJitter[]  = { 0.0f, 25.0f, -25.0f, 50.0f, -50.0f, 90.0f, -90.0f, 180.0f };
			static const float kYawForward[] = { 180.0f, 90.0f, -90.0f, 135.0f, -135.0f, 45.0f, -45.0f, 0.0f };
			static const float kYawStatic[]  = { 0.0f, 35.0f, -35.0f, 60.0f, -60.0f, 90.0f, -90.0f, 125.0f, -125.0f, 180.0f };
			static const float kYawChoke[]   = { 0.0f, 20.0f, -20.0f, 40.0f, -40.0f, 60.0f, -60.0f, 90.0f, -90.0f, 180.0f };

#define LADDER_LEN( ladder ) ( sizeof( ladder ) / sizeof( ( ladder )[ 0 ] ) )

			using Tier0MsgFn = void( * )( const char* fmt, ... );

			auto GetTier0Msg() -> Tier0MsgFn
			{
				static Tier0MsgFn fn = nullptr;
				static bool tried = false;

				if( !tried )
				{
					tried = true;

					HMODULE tier0 = ModuleD( "tier0.dll" );

					if( tier0 )
						fn = ( Tier0MsgFn )GetProcedure( tier0, "Msg" );
				}

				return fn;
			}

			auto Log( const char* fmt, ... ) -> void
			{
				if( !Config::Misc->ResolverLog )
					return;

				Tier0MsgFn msg = GetTier0Msg();

				if( !msg )
					return;

				char buffer[ 512 ] = {};

				va_list args;
				va_start( args, fmt );
				vsnprintf( buffer, sizeof( buffer ) - 2, fmt, args );
				va_end( args );

				std::strcat( buffer, "\n" );
				msg( "%s", buffer );
			}

			auto YawAAName( EYawAA aa ) -> const char*
			{
				switch( aa )
				{
				case Yaw_Static: return "STATIC";
				case Yaw_Spin: return "SPIN";
				case Yaw_Jitter: return "JITTER";
				case Yaw_Forward: return "FORWARD";
				case Yaw_Choke: return "CHOKE";
				default: return "--";
				}
			}

			auto PitchAAName( EPitchAA aa ) -> const char*
			{
				switch( aa )
				{
				case Pitch_Down: return "DOWN";
				case Pitch_Up: return "UP";
				case Pitch_Zero: return "ZERO";
				case Pitch_Fake: return "FAKE";
				case Pitch_Jitter: return "JITTER";
				default: return "--";
				}
			}

			auto NormalizeYaw( float yaw ) -> float
			{
				while( yaw > 180.0f )
					yaw -= 360.0f;

				while( yaw <= -180.0f )
					yaw += 360.0f;

				return yaw;
			}

			auto NormalizePitch( float pitch ) -> float
			{
				while( pitch > 180.0f )
					pitch -= 360.0f;

				while( pitch < -180.0f )
					pitch += 360.0f;

				return pitch;
			}

			auto ClampPitchPose( float pitch ) -> float
			{
				// Mirrors ComputePoseParam_BodyPitch: wrap, then clamp.
				float wrapped = NormalizePitch( pitch );
				return std::max( -90.0f, std::min( 90.0f, wrapped ) );
			}

			auto YawDiff( float a, float b ) -> float
			{
				return NormalizeYaw( a - b );
			}

			auto PushValue( float* hist, int& count, float value ) -> void
			{
				if( count < kHistory )
				{
					hist[ count++ ] = value;
				}
				else
				{
					std::memmove( hist, hist + 1, sizeof( float ) * ( kHistory - 1 ) );
					hist[ kHistory - 1 ] = value;
				}
			}

			struct SwingAnalysis
			{
				int count = 0;
				float avgAbsDelta = 0.0f;
				int signFlips = 0;
				int posCount = 0;
				int negCount = 0;
				float range = 0.0f;
				float median = 0.0f;
			};

			// Analyze a short angle history. Deltas are wrap-aware and the
			// range/median are computed on the unwrapped series so spin and
			// jitter are detected correctly across the +/-180 seam.
			auto AnalyzeSwing( const float* hist, int count ) -> SwingAnalysis
			{
				SwingAnalysis result;
				result.count = count;

				if( count <= 0 )
					return result;

				float unwrapped[ kHistory ] = {};
				unwrapped[ 0 ] = hist[ 0 ];

				float mn = hist[ 0 ];
				float mx = hist[ 0 ];
				float prevSign = 0.0f;

				for( int i = 1; i < count; i++ )
				{
					float delta = NormalizeYaw( hist[ i ] - hist[ i - 1 ] );
					unwrapped[ i ] = unwrapped[ i - 1 ] + delta;

					result.avgAbsDelta += std::fabs( delta );

					if( delta > 0.5f )
					{
						result.posCount++;

						if( prevSign < 0.0f )
							result.signFlips++;

						prevSign = 1.0f;
					}
					else if( delta < -0.5f )
					{
						result.negCount++;

						if( prevSign > 0.0f )
							result.signFlips++;

						prevSign = -1.0f;
					}

					if( unwrapped[ i ] < mn )
						mn = unwrapped[ i ];

					if( unwrapped[ i ] > mx )
						mx = unwrapped[ i ];
				}

				if( count > 1 )
					result.avgAbsDelta /= ( float )( count - 1 );

				result.range = mx - mn;

				float sorted[ kHistory ] = {};
				std::memcpy( sorted, unwrapped, sizeof( float ) * count );

				for( int i = 1; i < count; i++ )
				{
					float value = sorted[ i ];
					int j = i - 1;

					while( j >= 0 && sorted[ j ] > value )
					{
						sorted[ j + 1 ] = sorted[ j ];
						j--;
					}

					sorted[ j + 1 ] = value;
				}

				if( count & 1 )
					result.median = sorted[ count / 2 ];
				else
					result.median = ( sorted[ count / 2 - 1 ] + sorted[ count / 2 ] ) * 0.5f;

				return result;
			}

			auto ClassifyYaw( const SwingAnalysis& analysis, bool facing_us, bool choking ) -> EYawAA
			{
				if( analysis.count < 3 )
					return Yaw_Static;

				bool one_direction = ( analysis.posCount == 0 || analysis.negCount == 0 );

				// Consistent rotation (fast spin or slow drift in one direction).
				if( analysis.count >= 4 && one_direction &&
					( analysis.avgAbsDelta > 8.0f || analysis.range > 25.0f ) )
					return Yaw_Spin;

				// Rapid direction flips.
				if( analysis.signFlips >= 3 && analysis.avgAbsDelta > 12.0f )
					return Yaw_Jitter;

				// Effectively frozen angles.
				if( analysis.range < 3.0f && analysis.avgAbsDelta < 2.0f )
				{
					if( facing_us )
						return Yaw_Forward;

					if( choking )
						return Yaw_Choke;

					return Yaw_Static;
				}

				// Anything else looks like normal mouse movement.
				return Yaw_None;
			}

			auto ClassifyPitch( float raw_pitch, const SwingAnalysis& analysis ) -> EPitchAA
			{
				if( analysis.count >= 4 && analysis.signFlips >= 3 && analysis.avgAbsDelta > 15.0f )
					return Pitch_Jitter;

				float pitch = NormalizePitch( raw_pitch );

				if( pitch >= 60.0f && pitch <= 140.0f )
					return Pitch_Down;

				if( pitch <= -60.0f && pitch >= -140.0f )
					return Pitch_Up;

				if( std::fabs( pitch ) < 25.0f )
					return Pitch_Zero;

				if( std::fabs( pitch ) > 140.0f )
					return Pitch_Fake;

				return Pitch_None;
			}

			auto PitchLadderValue( EPitchAA aa, int step, float raw_pitch, float median ) -> float
			{
				static const float kDown[] = { -89.0f, 0.0f, 89.0f, -45.0f, 45.0f };
				static const float kUp[] = { 89.0f, 0.0f, -89.0f, 45.0f, -45.0f };
				static const float kZero[] = { 0.0f, -45.0f, 45.0f, -89.0f, 89.0f };
				static const float kAlt[] = { -89.0f, 89.0f, 0.0f };

				switch( aa )
				{
				case Pitch_Down:
					return kDown[ step % LADDER_LEN( kDown ) ];

				case Pitch_Up:
					return kUp[ step % LADDER_LEN( kUp ) ];

				case Pitch_Zero:
					return kZero[ step % LADDER_LEN( kZero ) ];

				case Pitch_Fake:
					return ( step % 4 == 0 ) ? raw_pitch : kAlt[ ( step - 1 ) % LADDER_LEN( kAlt ) ];

				case Pitch_Jitter:
					return ( step % 4 == 0 ) ? NormalizePitch( median ) : kAlt[ ( step - 1 ) % LADDER_LEN( kAlt ) ];

				default:
					return raw_pitch;
				}
			}

			auto EnsureState( int index ) -> ResolveState&
			{
				ResolveState& state = s_state[ index ];

				float curtime = Source::m_pGlobalVars->curtime;

				// userID check (once per frame): a new player in this slot
				// gets a fresh state instead of inheriting the old one.
				if( state.infoTime != curtime )
				{
					state.infoTime = curtime;

					player_info_t info = {};

					if( Source::m_pEngine->GetPlayerInfo( index, &info ) )
					{
						if( !state.active || state.userID != info.userID )
						{
							state = ResolveState{};
							state.active = true;
							state.userID = info.userID;
							state.infoTime = curtime;
						}

						std::memcpy( state.name, info.name, sizeof( state.name ) - 1 );
						state.name[ sizeof( state.name ) - 1 ] = '\0';
					}
				}

				if( !state.active )
					state.active = true;

				return state;
			}

			// Map body_yaw / body_pitch pose names to m_flPoseParameter
			// indices through the model's studio header. Cached per model,
			// SEH-guarded and validated; failures degrade gracefully.
			auto ResolvePoseIndices( int index, C_CSPlayer* player ) -> void
			{
				PoseIndices& indices = s_poseIdx[ index ];

				const model_t* model = player->GetModel();

				if( !model )
					return;

				if( indices.model == model && indices.yaw != -2 )
					return;

				indices.model = model;
				indices.yaw = -1;
				indices.pitch = -1;

				studiohdr_t* studio = Source::m_pModelInfoClient->GetStudioModel( model );

				if( !studio )
					return;

				__try
				{
					if( studio->id != kStudioId )
						return;

					int count = *( int* )( ( byte* )studio + kStudioPoseCountOffset );
					int offset = *( int* )( ( byte* )studio + kStudioPoseIndexOffset );

					if( count < 0 || count > 32 )
						return;

					if( offset <= 0 || offset > studio->length )
						return;

					for( int i = 0; i < count && i < kPoseCount; i++ )
					{
						byte* desc = ( byte* )studio + offset + i * kPoseDescSize;
						int nameOffset = *( int* )desc;

						if( nameOffset <= 0 || nameOffset > 4096 )
							continue;

						const char* name = ( const char* )desc + nameOffset;

						if( std::strlen( name ) > 32 )
							continue;

						if( !std::strcmp( name, "body_yaw" ) )
							indices.yaw = i;
						else if( !std::strcmp( name, "body_pitch" ) )
							indices.pitch = i;
					}
				}
				__except( EXCEPTION_EXECUTE_HANDLER )
				{
					indices.yaw = -1;
					indices.pitch = -1;
				}
			}

			// Track simulation-time gaps (choked packets). Called from both
			// proxies; drops the given history on dormancy/teleport gaps.
			auto UpdateSimTime( ResolveState& state, float sim_time, int* history_count ) -> void
			{
				float curtime = Source::m_pGlobalVars->curtime;
				float interval = Source::m_pGlobalVars->interval_per_tick;

				if( state.lastSim > 0.0f && sim_time > state.lastSim )
				{
					float delta = sim_time - state.lastSim;

					if( delta > interval * 2.5f )
						state.lastChokeTime = curtime;

					if( delta > 1.0f && history_count )
						*history_count = 0;
				}

				state.lastSim = sim_time;
			}

			auto IsChoking( const ResolveState& state ) -> bool
			{
				return ( Source::m_pGlobalVars->curtime - state.lastChokeTime ) < 1.0f;
			}

			auto MaybeResetLadder( ResolveState& state ) -> bool
			{
				int key = ( ( int )state.yawAA << 4 ) | ( int )state.pitchAA;

				if( key != state.ladderKey )
				{
					bool initial = ( state.ladderKey == -1 );
					state.ladderKey = key;
					state.step = 0;
					state.shotsAtStep = 0;
					state.hitsAtStep = 0;
					return !initial;
				}

				return false;
			}

			auto UseRawAngles( const ResolveState& state ) -> bool
			{
				// Grace shots: shoot the networked angles first so legit
				// players are unaffected. Skipped once a working correction
				// was found (hasBest).
				int grace = Config::Current->Aimbot->ResvolerBulletsDelay;

				if( grace < 0 )
					grace = 0;

				return !state.hasBest && state.shotsTotal <= grace;
			}

			auto FormatPoseValue( int index, int pose_index, char* out, int out_size ) -> void
			{
				if( pose_index < 0 || !s_poseSeen[ index ] )
				{
					std::snprintf( out, out_size, "?" );
					return;
				}

				float value = s_serverPose[ index ][ pose_index ];

				if( value != value || value < -999.0f || value > 999.0f )
				{
					std::snprintf( out, out_size, "?" );
					return;
				}

				std::snprintf( out, out_size, "%+.0f", value );
			}

			auto BuildText( ResolveState& state, int index ) -> void
			{
				const char* yaw = "";

				switch( state.yawAA )
				{
				case Yaw_Static: yaw = "ST"; break;
				case Yaw_Spin: yaw = "SPIN"; break;
				case Yaw_Jitter: yaw = "JIT"; break;
				case Yaw_Forward: yaw = "FWD"; break;
				case Yaw_Choke: yaw = "CH"; break;
				default: break;
				}

				const char* pitch = "";

				switch( state.pitchAA )
				{
				case Pitch_Down: pitch = "DN"; break;
				case Pitch_Up: pitch = "UP"; break;
				case Pitch_Zero: pitch = "ZR"; break;
				case Pitch_Fake: pitch = "FK"; break;
				case Pitch_Jitter: pitch = "JT"; break;
				default: break;
				}

				if( !yaw[ 0 ] && !pitch[ 0 ] )
				{
					state.text[ 0 ] = '\0';
					return;
				}

				char pose[ 24 ] = {};

				if( s_poseSeen[ index ] )
				{
					char yaw_value[ 8 ] = {};
					char pitch_value[ 8 ] = {};

					FormatPoseValue( index, s_poseIdx[ index ].yaw, yaw_value, sizeof( yaw_value ) );
					FormatPoseValue( index, s_poseIdx[ index ].pitch, pitch_value, sizeof( pitch_value ) );

					std::snprintf( pose, sizeof( pose ), " [%s/%s]%s",
						yaw_value, pitch_value, state.poseMismatch ? "!" : "" );
				}

				if( state.moving )
					std::snprintf( state.text, sizeof( state.text ), ">%s s%d %s%s", yaw, state.step, pitch, pose );
				else
					std::snprintf( state.text, sizeof( state.text ), "%s s%d %s%s", yaw, state.step, pitch, pose );
			}
		}

		auto ResolvePitch( C_CSPlayer* player, float raw_pitch ) -> float
		{
			if( !player )
				return raw_pitch;

			int index = player->GetIndex();

			if( index < 1 || index > kMaxPlayers )
				return raw_pitch;

			auto local = C_CSPlayer::GetLocalPlayer();

			if( !local || local == player )
				return raw_pitch;

			if( !Config::Current->Aimbot->Resolver || !Config::Current->Aimbot->ResolverPitch )
				return raw_pitch;

			ResolveState& state = EnsureState( index );

			ResolvePoseIndices( index, player );

			UpdateSimTime( state, player->m_flSimulationTime(), &state.pitchCount );
			PushValue( state.pitchHist, state.pitchCount, raw_pitch );

			SwingAnalysis analysis = AnalyzeSwing( state.pitchHist, state.pitchCount );
			state.pitchAA = ClassifyPitch( raw_pitch, analysis );

			// Server-pitch-vs-eye invariant: body_pitch must equal the
			// clamped networked pitch. A split means the server animated
			// with different angles than it networked (hard desync tell).
			if( s_poseIdx[ index ].pitch >= 0 && s_poseSeen[ index ] )
			{
				float server = s_serverPose[ index ][ s_poseIdx[ index ].pitch ];
				bool valid = ( server == server ) && server > -999.0f && server < 999.0f;
				bool mismatch = valid && std::fabs( server - ClampPitchPose( raw_pitch ) ) > 12.0f;

				if( mismatch && !state.poseMismatch )
					Log( "[Resolver] %.24s | POSE/EYE SPLIT: server pitch %+.0f vs eye %+.0f",
						state.name, server, raw_pitch );

				state.poseMismatch = mismatch;
			}
			else
			{
				state.poseMismatch = false;
			}

			if( MaybeResetLadder( state ) )
				Log( "[Resolver] %.24s | mode %s/%s", state.name,
					YawAAName( state.yawAA ), PitchAAName( state.pitchAA ) );

			BuildText( state, index );

			if( UseRawAngles( state ) || state.pitchAA == Pitch_None )
			{
				state.lastPitchOut = raw_pitch;
				return raw_pitch;
			}

			state.lastPitchOut = PitchLadderValue( state.pitchAA, state.step, raw_pitch, analysis.median );
			return state.lastPitchOut;
		}

		auto ResolveYaw( C_CSPlayer* player, float raw_yaw ) -> float
		{
			if( !player )
				return raw_yaw;

			int index = player->GetIndex();

			if( index < 1 || index > kMaxPlayers )
				return raw_yaw;

			auto local = C_CSPlayer::GetLocalPlayer();

			if( !local || local == player )
				return raw_yaw;

			if( !Config::Current->Aimbot->Resolver )
				return raw_yaw;

			ResolveState& state = EnsureState( index );

			ResolvePoseIndices( index, player );

			UpdateSimTime( state, player->m_flSimulationTime(), &state.yawCount );
			PushValue( state.yawHist, state.yawCount, raw_yaw );

			Vector3 velocity = player->m_vecVelocity();
			state.moving = ( velocity.x * velocity.x + velocity.y * velocity.y ) > 25.0f;

			// Is he statically staring at us? (classic forward-AA tell)
			Vector3 direction = local->EyePosition() - player->m_vecOrigin();
			VectorNormalize( direction );

			Vector3 aim = {};
			VectorAngles( direction, aim );

			bool facing_us = std::fabs( YawDiff( raw_yaw, aim.y ) ) < 12.0f;

			SwingAnalysis analysis = AnalyzeSwing( state.yawHist, state.yawCount );
			state.yawAA = ClassifyYaw( analysis, facing_us, IsChoking( state ) );

			if( MaybeResetLadder( state ) )
				Log( "[Resolver] %.24s | mode %s/%s", state.name,
					YawAAName( state.yawAA ), PitchAAName( state.pitchAA ) );

			BuildText( state, index );

			if( UseRawAngles( state ) || state.yawAA == Yaw_None )
			{
				state.lastYawCorr = 0.0f;
				return raw_yaw;
			}

			// ----------------------------------------------------------
			// ПРИОРИТЕТ 1: pose parameter "body_yaw".
			//
			// Сервер сам присылает нам поворот корпуса относительно ног
			// (это и есть десинк) — его не надо угадывать лесенкой.
			// В CS:S v34 body_yaw хранится в m_flPoseParameter уже
			// нормализованным в 0..1 для диапазона [-60; 60] градусов.
			// Пока значение валидно, это КРАТНО точнее брутфорса, поэтому
			// лесенка ниже остаётся только как запасной вариант.
			if( s_poseIdx[ index ].yaw >= 0 && s_poseSeen[ index ] )
			{
				float pose = s_serverPose[ index ][ s_poseIdx[ index ].yaw ];

				if( pose == pose && pose >= -0.01f && pose <= 1.01f )
				{
					// 0..1 -> -60..+60 градусов поворота корпуса.
					const float body_yaw = ( pose * 120.0f ) - 60.0f;

					// Корпус повёрнут заметно = игрок в десинке. Реальные
					// углы = то, что networked, плюс этот поворот.
					if( std::fabs( body_yaw ) > 5.0f )
					{
						state.lastYawCorr = body_yaw;

						return NormalizeYaw( raw_yaw + body_yaw + Config::Misc->ResolverAng );
					}
				}
			}

			const float* ladder = kYawStatic;
			int length = LADDER_LEN( kYawStatic );
			float base = raw_yaw;

			switch( state.yawAA )
			{
			case Yaw_Spin:
				ladder = kYawSpin;
				length = LADDER_LEN( kYawSpin );
				break;

			case Yaw_Jitter:
				ladder = kYawJitter;
				length = LADDER_LEN( kYawJitter );
				base = NormalizeYaw( analysis.median );
				break;

			case Yaw_Forward:
				ladder = kYawForward;
				length = LADDER_LEN( kYawForward );
				break;

			case Yaw_Choke:
				ladder = kYawChoke;
				length = LADDER_LEN( kYawChoke );
				break;

			default:
				break;
			}

			state.lastYawCorr = ladder[ state.step % length ];

			// Manual fine-tune on top of the ladder ("Resolver Add Y").
			return NormalizeYaw( base + state.lastYawCorr + Config::Misc->ResolverAng );
		}

		auto RegisterShot( int index ) -> void
		{
			if( index < 1 || index > kMaxPlayers )
				return;

			ResolveState& state = s_state[ index ];

			if( !state.active )
				state.active = true;

			state.shotsAtStep++;
			state.shotsTotal++;

			int bullets = Config::Current->Aimbot->ResvolerBullets;

			if( bullets < 1 )
				bullets = 1;

			int grace = Config::Current->Aimbot->ResvolerBulletsDelay;

			if( grace < 0 )
				grace = 0;

			// Hits offset misses, so a working correction sticks longer
			// the more it hits instead of being stepped away from.
			int misses = state.shotsAtStep - state.hitsAtStep;

			if( state.shotsTotal > grace && misses >= bullets )
			{
				state.step++;
				state.shotsAtStep = 0;
				state.hitsAtStep = 0;

				Log( "[Resolver] %.24s | %s/%s -> step %d (yaw %+.0f pitch %+.0f) after %d miss",
					state.name, YawAAName( state.yawAA ), PitchAAName( state.pitchAA ),
					state.step, state.lastYawCorr, state.lastPitchOut, misses );
			}
		}

		auto RegisterHit( int index ) -> void
		{
			if( index < 1 || index > kMaxPlayers )
				return;

			ResolveState& state = s_state[ index ];

			// Ignore collateral damage on players we never shot at: it must
			// not "confirm" a correction we never tested.
			if( !state.active || state.shotsTotal == 0 )
				return;

			state.hitsAtStep++;
			state.hasBest = true;

			// First hit on a step locks it in: worth one log line.
			if( state.hitsAtStep == 1 )
				Log( "[Resolver] %.24s | HIT @ step %d (yaw %+.0f pitch %+.0f) %dh/%ds",
					state.name, state.step, state.lastYawCorr, state.lastPitchOut,
					state.hitsAtStep, state.shotsTotal );
		}

		auto OnGameEvent( IGameEvent* game_event ) -> void
		{
			if( !game_event )
				return;

			const char* name = game_event->GetName();

			if( !name )
				return;

			if( !std::strcmp( name, "round_start" ) )
			{
				ResetAll();
				return;
			}

			auto local = C_CSPlayer::GetLocalPlayer();

			if( !local )
				return;

			if( !std::strcmp( name, "weapon_fire" ) )
			{
				int shooter = Source::m_pEngine->GetPlayerForUserID( game_event->GetInt( "userid" ) );

				if( shooter != local->GetIndex() )
					return;

				// Knives, grenades and C4 are not resolver feedback.
				const char* weapon = game_event->GetString( "weapon" );

				if( weapon && ( std::strstr( weapon, "knife" ) || std::strstr( weapon, "grenade" ) ||
					std::strstr( weapon, "flashbang" ) || std::strstr( weapon, "c4" ) ) )
					return;

				if( !Config::Current->Aimbot->Resolver )
					return;

				// Attribute the shot to the aimbot's current target.
				int target = Config::Misc->target;

				if( target < 1 || target > kMaxPlayers )
					return;

				// Only track targets that actually use the auto resolver.
				auto player_from_list = Source::m_pPlayerList->GetPlayer( target );

				if( !player_from_list )
					return;

				if( player_from_list->m_pitch != 4 && player_from_list->m_yaw != 7 && player_from_list->m_yaw != 8 )
					return;

				RegisterShot( target );
			}
			else if( !std::strcmp( name, "player_hurt" ) )
			{
				int attacker = Source::m_pEngine->GetPlayerForUserID( game_event->GetInt( "attacker" ) );

				if( attacker != local->GetIndex() )
					return;

				RegisterHit( Source::m_pEngine->GetPlayerForUserID( game_event->GetInt( "userid" ) ) );
			}
			else if( !std::strcmp( name, "player_death" ) )
			{
				Reset( Source::m_pEngine->GetPlayerForUserID( game_event->GetInt( "userid" ) ) );
			}
		}

		auto Reset( int index ) -> void
		{
			if( index < 1 || index > kMaxPlayers )
				return;

			s_state[ index ] = ResolveState{};
			s_poseSeen[ index ] = false;
			s_poseIdx[ index ] = PoseIndices{};
		}

		auto ResetAll() -> void
		{
			for( int i = 1; i <= kMaxPlayers; i++ )
			{
				s_state[ i ] = ResolveState{};
				s_poseSeen[ i ] = false;
				s_poseIdx[ i ] = PoseIndices{};
			}
		}

		auto GetText( int index ) -> const char*
		{
			if( index < 1 || index > kMaxPlayers )
				return "";

			if( !Config::Current->Aimbot->Resolver )
				return "";

			return s_state[ index ].text;
		}

		void DT_CSPlayer_m_flPoseParameter( const CRecvProxyData* pData, void* pStruct, void* pOut )
		{
			float value = pData ? pData->m_Value.m_Float : 0.0f;

			// Always write through: this proxy only observes, it must
			// never break the network chain.
			*( float* )( pOut ) = value;

			if( !pData || !pStruct )
				return;

			int element = pData->m_iElement;

			if( element < 0 || element >= kPoseCount )
				return;

			auto entity = ( C_BaseEntity* )pStruct;
			int index = entity->GetIndex();

			if( index < 1 || index > kMaxPlayers )
				return;

			auto local = C_CSPlayer::GetLocalPlayer();

			if( local && index == local->GetIndex() )
				return;

			s_serverPose[ index ][ element ] = value;
			s_poseSeen[ index ] = true;
		}
	}
}
