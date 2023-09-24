/*
 * itemtypes.cpp
 * This whole file is suboptimal.
 * Use this tool with "hash" algorithm selected : https://casualhacks.net/hashtool.html
 */

#include "common.hpp"

unsigned long Hash::String(const char *pszOrg)
{
    unsigned long iHash = 5381;

    while (int iStr = static_cast<unsigned char>(*pszOrg++))
        iHash = iHash * 33 ^ iStr;

    return iHash;
}

bool Hash::IsHealth(const char *pszName)
{
    switch (String(pszName))
    {
    case MedKitSmall:
    case MedKitMedium:
    case MedKitLarge:
    case MedKitSmallHalloween:
    case MedKitMediumHalloween:
    case MedKitLargeHalloween:
    case MedKitSmallBday:
    case MedKitMediumBday:
    case MedKitLargeBday:
    case MedKitLargeMushroom:
    case MedKitMediumMedieval:
    case MedKitSandwich:
    case MedKitSandwichXmas:
    case MedKitSandwichRobo:
    case MedKitBanana:
    case MedKitSteak:
    case MedKitFishcake:
    case MedKitChoco:
        return true;
    default:
        return false;
    }
}

bool Hash::IsAmmo(const char *pszName)
{
    switch (String(pszName))
    {
    case AmmoSmall:
    case AmmoMedium:
    case AmmoLarge:
    case AmmoMediumBday:
    case AmmoLargeBday:
        return true;
    default:
        return false;
    }
}

bool Hash::IsCrumpkin(const char *pszName)
{
    switch (String(pszName))
    {
    case Crumpkin:
        return true;
    default:
        return false;
    }
}

bool Hash::IsPowerup(const char *pszName)
{
    switch (String(pszName))
    {
    case PowerupHaste:
    case PowerupVampire:
    case PowerupPrecision:
    case PowerupRegen:
    case PowerupSupernova:
    case PowerupStrength:
    case PowerupKnockout:
    case PowerupResistance:
    case PowerupCrits:
    case PowerupAgility:
    case PowerupKing:
    case PowerupPlague:
    case PowerupReflect:
        return true;
    default:
        return false;
    }
}

/*bool Hash::IsPowerupHaste(const char *pszName)
{
   switch (String(pszName))
   {
   case PowerupHaste:
       return true;
   default:
       return false;
   }
}

bool Hash::IsPowerupVampire(const char *pszName)
{
   switch (String(pszName))
   {
   case PowerupVampire:
       return true;
   default:
       return false;
   }
}

bool Hash::IsPowerupPrecision(const char *pszName)
{
   switch (String(pszName))
   {
   case PowerupPrecision:
       return true;
   default:
       return false;
   }
}

bool Hash::IsPowerupRegen(const char *pszName)
{
   switch (String(pszName))
   {
   case PowerupRegen:
       return true;
   default:
       return false;
   }
}

bool Hash::IsPowerupSupernova(const char *pszName)
{
   switch (String(pszName))
   {
   case PowerupSupernova:
       return true;
   default:
       return false;
   }
}

bool Hash::IsPowerupStrength(const char *pszName)
{
   switch (String(pszName))
   {
   case PowerupStrength:
       return true;
   default:
       return false;
   }
}

bool Hash::IsPowerupKnockout(const char *pszName)
{
   switch (String(pszName))
   {
   case PowerupKnockout:
       return true;
   default:
       return false;
   }
}

bool Hash::IsPowerupResistance(const char *pszName)
{
   switch (String(pszName))
   {
   case PowerupResistance:
       return true;
   default:
       return false;
   }
}

bool Hash::IsPowerupCrits(const char *pszName)
{
   switch (String(pszName))
   {
   case PowerupCrits:
       return true;
   default:
       return false;
   }
}

bool Hash::IsPowerupAgility(const char *pszName)
{
   switch (String(pszName))
   {
   case PowerupAgility:
       return true;
   default:
       return false;
   }
}

bool Hash::IsPowerupKing(const char *pszName)
{
   switch (String(pszName))
   {
   case PowerupKing:
       return true;
   default:
       return false;
   }
}

bool Hash::IsPowerupPlague(const char *pszName)
{
   switch (String(pszName))
   {
   case PowerupPlague:
       return true;
   default:
       return false;
   }
}

bool Hash::IsPowerupReflect(const char *pszName)
{
   switch (String(pszName))
   {
   case PowerupReflect:
       return true;
   default:
       return false;
   }
}*/

bool Hash::IsSpellbook(const char *pszName)
{
    switch (String(pszName))
    {
    case Spell:
    case Spell1:
    case Spell2:
        return true;
    default:
        return false;
    }
}

bool Hash::IsSpellbookRare(const char *pszName)
{
    switch (String(pszName))
    {
    case RareSpell:
    case RareSpell1:
    case RareSpell2:
        return true;
    default:
        return false;
    }
}

bool Hash::IsHazard(const char *pszName)
{
    switch (String(pszName))
    {
    case PumpkinBomb:
    case PumpkinBombTeamcolor:
    case BalloonBomb:
    case WoodenBarrel:
    case WalkerExplode:
    case SnowmanBomb:
    case BreadFatty:
        return true;
    default:
        return false;
    }
}

bool Hash::IsFlag(const char *pszName)
{
    switch (String(pszName))
    {
    case AtomBomb:
    case SkullPickup:
    case SkullPickup1:
    case GibBucket:
    case BottlePickup:
    case Gift:
    case AussieContainer:
    case TicketCase:
    case Briefcase:
    case Briefcase2:
    case Flag:
    case Flag2:
        return true;
    default:
        return false;
    }
}

bool Hash::IsBombCart(const char *pszName)
{
    switch (String(pszName))
    {
    case BombCart:
    case BombCart1:
    case BombCart2:
    case BombCart3:
    case BombCart4:
    case BombCart5:
        return true;
    default:
        return false;
    }
}

bool Hash::IsBombCartRed(const char *pszName)
{
    switch (String(pszName))
    {
    case BombCartRed:
    case BombCartRed1:
        return true;
    default:
        return false;
    }
}
