/*
 * itemtypes.hpp
 * This whole file is suboptimal.
 * Use this tool with "hash" algorithm selected : https://casualhacks.net/hashtool.html
 */

#pragma once
#include <string>

typedef unsigned long size_owned;

// Main hash function
// And prehashed strings allowing for fast comparison.
namespace Hash
{
//=== Health packs ===
// Normal
constexpr size_owned MedKitSmall  = 0x39a595ee;
constexpr size_owned MedKitMedium = 0x33115aec;
constexpr size_owned MedKitLarge  = 0x6330b20c;

// Halloween
constexpr size_owned MedKitSmallHalloween  = 0xe9bcc846;
constexpr size_owned MedKitMediumHalloween = 0xacfc7a04;
constexpr size_owned MedKitLargeHalloween  = 0xe0d93ae4;

// Holiday
constexpr size_owned MedKitSmallBday     = 0xc237906f;
constexpr size_owned MedKitMediumBday    = 0x3e17252d;
constexpr size_owned MedKitLargeBday     = 0x8730d1cd;
constexpr size_owned MedKitLargeMushroom = 0xb4cff15c;

// Medieval
constexpr size_owned MedKitMediumMedieval = 0x7587b6e1;

// Lunchboxes
constexpr size_owned MedKitSandwich     = 0xde7af718;
constexpr size_owned MedKitSandwichXmas = 0x26aa9052;
constexpr size_owned MedKitSandwichRobo = 0xa754f845;
constexpr size_owned MedKitBanana       = 0x05146988;
constexpr size_owned MedKitSteak        = 0xda7d352f;
constexpr size_owned MedKitFishcake     = 0xab5934e6;
constexpr size_owned MedKitChoco        = 0xa5caa8e6;

//=== Ammo packs ===
// Normal
constexpr size_owned AmmoSmall  = 0x11ef1a43;
constexpr size_owned AmmoMedium = 0x4dd0cc21;
constexpr size_owned AmmoLarge  = 0xb538c321;
// Holiday
constexpr size_owned AmmoMediumBday = 0x28b17300;
constexpr size_owned AmmoLargeBday  = 0x72abca00;

//=== Halloween ===
constexpr size_owned Crumpkin = 0xaa60f7cf;

//=== Powerups ===
constexpr size_owned PowerupHaste      = 0x113cb550;
constexpr size_owned PowerupVampire    = 0xb434f08f;
constexpr size_owned PowerupPrecision  = 0xd6e7e2cd;
constexpr size_owned PowerupRegen      = 0xf6fbc3a0;
constexpr size_owned PowerupSupernova  = 0xf8dad5ec;
constexpr size_owned PowerupStrength   = 0xf89aa23e;
constexpr size_owned PowerupKnockout   = 0xa4ea7097;
constexpr size_owned PowerupResistance = 0x11193801;
constexpr size_owned PowerupCrits      = 0xe1cf4257;
constexpr size_owned PowerupAgility    = 0x378f855c;
constexpr size_owned PowerupKing       = 0x6d6c8370;
constexpr size_owned PowerupPlague     = 0xffd5ced1;
constexpr size_owned PowerupReflect    = 0xf88a87f4;

//=== Spellbooks ===
constexpr size_owned Spell      = 0x7dfe1327;
constexpr size_owned Spell1     = 0x9de70281;
constexpr size_owned Spell2     = 0x8fd658ed;
constexpr size_owned RareSpell  = 0x6c0f00e3;
constexpr size_owned RareSpell1 = 0x28177e05;
constexpr size_owned RareSpell2 = 0x2a1a7798;

//=== Environmental Hazards ===
constexpr size_owned PumpkinBomb          = 0xa093e718;
constexpr size_owned PumpkinBombTeamcolor = 0xb2a68387;
constexpr size_owned BalloonBomb          = 0xda3eb058;
constexpr size_owned WoodenBarrel         = 0x23c01644;
constexpr size_owned WalkerExplode        = 0xd530882f;
constexpr size_owned SnowmanBomb          = 0xddf06d35;
constexpr size_owned BreadFatty           = 0xa32d5466;

//=== Flags ===
constexpr size_owned AtomBomb        = 0x4df65bd9;
constexpr size_owned SkullPickup     = 0x5eea40f5;
constexpr size_owned SkullPickup1    = 0x2669d100;
constexpr size_owned GibBucket       = 0x4fdc540a;
constexpr size_owned BottlePickup    = 0xacc31aee;
constexpr size_owned Gift            = 0x8d0ca1c0;
constexpr size_owned AussieContainer = 0xec29d3e0;
constexpr size_owned TicketCase      = 0xab52d991;
constexpr size_owned Briefcase       = 0xd81f4570;
constexpr size_owned Briefcase2      = 0x77382e50;
constexpr size_owned Flag            = 0x4e1f4b32;
constexpr size_owned Flag2           = 0xaff31b06;

//=== Bomb Carts ===
constexpr size_owned BombCart     = 0xc8eec394;
constexpr size_owned BombCart1    = 0x10badb35;
constexpr size_owned BombCart2    = 0x83739708;
constexpr size_owned BombCart3    = 0x1b2875eb;
constexpr size_owned BombCart4    = 0x12550947;
constexpr size_owned BombCart5    = 0x0ff76f5e;
constexpr size_owned BombCartRed  = 0x5bcb6c87;
constexpr size_owned BombCartRed1 = 0xdc059338;

bool IsAmmo(const char *szName);
bool IsCrumpkin(const char *szName);
bool IsPowerup(const char *szName);
/*bool IsPowerupHaste(const char *szName);
bool IsPowerupVampire(const char *szName);
bool IsPowerupPrecision(const char *szName);
bool IsPowerupRegen(const char *szName);
bool IsPowerupSupernova(const char *szName);
bool IsPowerupStrength(const char *szName);
bool IsPowerupKnockout(const char *szName);
bool IsPowerupResistance(const char *szName);
bool IsPowerupCrits(const char *szName);
bool IsPowerupAgility(const char *szName);
bool IsPowerupKing(const char *szName);
bool IsPowerupPlague(const char *szName);
bool IsPowerupReflect(const char *szName);*/
bool IsHealth(const char *szName);
bool IsSpellbook(const char *szName);
bool IsSpellbookRare(const char *szName);
bool IsHazard(const char *szName);
bool IsFlag(const char *szName);
bool IsBombCart(const char *szName);
bool IsBombCartRed(const char *szName);
unsigned long String(const char *szOrg);
} // namespace Hash
