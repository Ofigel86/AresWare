# Weapon reference (from official CS:S sources, `Other Source/hl2sdk-ep1c-game-cstrike-1`)

Cross-checked against this cheat's `NoSpread.cpp` / `SDK/CSWeapon.h::GetSpread`:
the cheat's values are an exact decompilation of the v34 `client.dll` and match
the SDK formulas everywhere they can be compared. Where the SDK era differs from
the v34 binary (e.g. Glock burst standing spread), the cheat follows the v34 binary.

## Spread formulas (PrimaryAttack, ep1c SDK)

Format: air | run | duck | stand (accuracy = m_flAccuracy)

| Weapon | Air | Run (>140 / >5) | Duck | Stand |
|---|---|---|---|---|
| AK47 (27) | 0.04+0.4*a | 0.04+0.07*a | 0.0275*a | 0.0275*a |
| AUG (8) | 0.035+0.4*a | 0.035+0.07*a | 0.02*a | 0.02*a |
| M4A1 (21) | 0.035+0.4*a | 0.035+0.07*a | 0.025*a silenced / 0.02*a unsil | same |
| SG552 (26) | 0.035+0.45*a | 0.035+0.075*a | 0.02*a | 0.02*a |
| Galil (14) | 0.04+0.3*a | 0.04+0.07*a | 0.0375*a | 0.0375*a |
| FAMAS (15) | 0.03+0.3*a | 0.03+0.07*a | 0.02*a | 0.02*a (+0.01 non-burst) |
| MAC10 (7) | 0.375*a | 0.03*a | 0.03*a | 0.03*a |
| MP5 (18) | 0.2*a | 0.04*a | 0.04*a | 0.04*a |
| UMP45 (12) | 0.24*a | 0.04*a | 0.04*a | 0.04*a |
| TMP (22) | 0.2*a (approx) | 0.04*a | 0.04*a | 0.04*a |
| P90 (29) | 0.3*a | 0.115*a | 0.045*a | 0.045*a |
| M249 (19) | 0.045*a | 0.08*a | 0.0375*a | 0.0375*a |
| AWP (17) | 0.85 | 0.25 | 0.0 | 0.001 (+0.08 unscoped) |
| Scout (3) | 0.2 | 0.075 | 0.0 | 0.005 (+0.025 unscoped) |
| SG550 (13) | 0.45*(1-a) | 0.15 | 0.04*(1-a) | 0.05*(1-a) (+0.025 unscoped) |
| G3SG1 (23) | 0.45*(1-a) | 0.15 | 0.035*(1-a) | 0.055*(1-a) (+0.025 unscoped) |
| Deagle (25) | 1.5*(1-a) | 0.25*(1-a) | 0.115*(1-a) | 0.13*(1-a) |
| USP (16) sil | 1.3*(1-a) | 0.25*(1-a) | 0.125*(1-a) | 0.15*(1-a) |
| USP (16) unsil | 1.2*(1-a) | 0.225*(1-a) | 0.08*(1-a) | 0.1*(1-a) |
| Glock (2) semi | 1.0*(1-a) | 0.165*(1-a) | 0.075*(1-a) | 0.1*(1-a) |
| Glock (2) burst | 1.2*(1-a) | v34: 0.3*(1-a) | 0.095*(1-a) | v34: 0.185*(1-a), burst follow-ups 0.05 |
| P228 (1) | 1.5*(1-a) | 0.255*(1-a) | 0.075*(1-a) | 0.15*(1-a) |
| FiveSeven (11) | 1.5*(1-a) | 0.255*(1-a) | 0.075*(1-a) | 0.15*(1-a) |
| Elite (10) | 1.3*(1-a) | 0.175*(1-a) | 0.08*(1-a) | 0.1*(1-a) |

## KickBack recoil parameters (up_base, lateral_base, up_mod, lat_mod, up_max, lat_max, dir_change)

Order of conditions: run (vel2d>5) | air | duck | stand

| Weapon | Run | Air | Duck | Stand |
|---|---|---|---|---|
| AK47 | (1.5,0.45,0.225,0.05,6.5,2.5,7) | (2,1,0.5,0.35,9,6,5) | (0.9,0.35,0.15,0.025,5.5,1.5,9) | (1,0.375,0.175,0.0375,5.75,1.75,8) |
| M4A1/Galil | (1,0.45,0.28,0.045,3.75,3,7) | (1.2,0.5,0.23,0.15,5.5,3.5,6) | (0.6,0.3,0.2,0.0125,3.25,2,7) | (0.65,0.35,0.25,0.015,3.5,2.25,7) |
| AUG/FAMAS | (1,0.45,0.275,0.05,4,2.5,7) | (1.25,0.45,0.22,0.18,5.5,4,5) | (0.575,0.325,0.2,0.011,3.25,2,8) | (0.625,0.375,0.25,0.0125,3.5,2.25,8) |
| SG552 | (1,0.45,0.28,0.04,4.25,2.5,7) | (1.25,0.45,0.22,0.18,6,4,5) | (0.6,0.35,0.2,0.0125,3.7,2,10) | (0.625,0.375,0.25,0.0125,4,2.25,9) |
| M249 | (1.8,0.65,0.45,0.125,5,3.5,8) | (1.1,0.5,0.3,0.06,4,3,8) | (0.75,0.325,0.25,0.025,3.5,2.5,9) | (0.8,0.35,0.3,0.03,3.75,3,9) |
| MAC10 | (1.3,0.55,0.4,0.05,4.75,3.75,5) | (0.9,0.45,0.25,0.035,3.5,2.75,7) | (0.75,0.4,0.175,0.03,2.75,2.5,10) | (0.775,0.425,0.2,0.03,3,2.75,9) |
| MP5 | (0.9,0.475,0.35,0.0425,5,3,6) | (0.5,0.275,0.2,0.03,3,2,10) | (0.225,0.15,0.1,0.015,2,1,10) | (0.25,0.175,0.125,0.02,2.25,1.25,10) |
| UMP45 | (0.125,0.65,0.55,0.0475,5.5,4,10) | (0.55,0.3,0.225,0.03,3.5,2.5,10) | (0.25,0.175,0.125,0.02,2.25,1.25,10) | (0.275,0.2,0.15,0.0225,2.5,1.5,10) |
| TMP | (1.1,0.5,0.35,0.045,4.5,3.5,6) | (0.8,0.4,0.2,0.03,3,2.5,7) | (0.7,0.35,0.125,0.025,2.5,2,10) | (0.725,0.375,0.15,0.025,2.75,2.25,9) |
| P90 | (0.9,0.45,0.35,0.04,5.25,3.5,4) | (0.45,0.3,0.2,0.0275,4,2.25,7) | (0.275,0.2,0.125,0.02,3,1,9) | (0.3,0.225,0.125,0.02,3.25,1.25,8) |

KickBack math (CCSPlayer::KickBack):
first shot uses (up_base, lateral_base); subsequent shots add
shotsFired * (up_modifier, lateral_modifier), clamped by (up_max, lateral_max);
every direction_change shots the lateral direction flips sign.

## Bullet penetration (CCSPlayer::GetBulletTypeParameters)

| Ammo | Power | Max distance |
|---|---|---|
| .50 AE | 30 | 1000 |
| 7.62mm | 39 | 5000 |
| 5.56mm / box | 35 | 4000 |
| .338 Magnum | 45 | 8000 |
| 9mm | 21 | 800 |
| Buckshot | 0 | 0 |
| .45 ACP | 15 | 500 |
| .357 SIG | 25 | 800 |
| 5.7mm | 30 | 2000 |

Material modifiers (penetration / damage): metal 0.5/0.3, dirt 0.5/0.3,
concrete 0.4/0.25, grate 1.0/0.99, vent 0.5/0.45, tile 0.65/0.3,
computer 0.4/0.45, wood 1.0/0.6, default 1.0/0.5.
Grates (CONTENTS_GRATE) force 1.0/0.99 regardless of texture.
Same-material wood/metal entry+exit doubles the penetration modifier.
