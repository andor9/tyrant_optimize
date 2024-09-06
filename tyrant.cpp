#include "tyrant.h"
#include "card.h"

#include <string>

const std::string faction_names[Faction::num_factions] =
{ "", "imperial", "raider", "bloodthirsty", "xeno", "righteous", "progenitor" };

const std::string skill_names[Skill::num_skills] =
{
    // Placeholder for no-skill:
    "<Error>",

    // Activation (harmful):
    "Enfeeble", "Jam", "Mortar", "Siege", "Strike", "Sunder", "Weaken",

    // Activation (helpful):
    "Evolve", "Heal", "Mend", "Overload", "Protect", "Rally", "Fortify",
    "Enrage", "Entrap", "Rush",

    // Activation (unclassified/polymorphic):
    "Mimic",

    // Defensive:
    "Armor", "Avenge", "Scavenge", "Corrosive", "Counter", "Evade", "Subdue", "Absorb", "Flying",
    "Payback", "Revenge", "Tribute", "Refresh", "Wall", "Barrier",

    // Combat-Modifier:
    "Coalition", "Legion", "Pierce", "Rupture", "Swipe", "Drain", "Venom", "Hunt","Mark",

    // Damage-Dependant:
    "Berserk", "Leech", "Poison",

    // Instant-Debuff:
    "Inhibit", "Sabotage", "Disease",
    // Triggered:
    "Allegiance", "Flurry", "Valor", "Stasis", "Summon", "Bravery", "Enhance",
};

const std::string skill_trigger_names[Skill::num_triggers] =
{
    "activate", "play", "attacked","death",
};

const std::string passive_bge_names[PassiveBGE::num_passive_bges] =
{
    // Placeholder for no-bge:
    "<Error>",

    // Passive BGEs
    "Bloodlust", "Brigade", "Counterflux", "Divert", "EnduringRage", "Fortification", "Heroism",
    "ZealotsPreservation", "Metamorphosis", "Megamorphosis", "Revenge", "TurningTides", "Virulence",
    "HaltedOrders", "Devour", "CriticalReach", "TemporalBacklash", "Furiosity", "OathOfLoyalty",
    "BloodVengeance", "ColdSleep", "IronWill", "Unity", "Devotion", "Crackdown", "SuperHeroism",
};

const std::string cardtype_names[CardType::num_cardtypes]{"Commander", "Assault", "Structure", };

const std::string rarity_names[]{"", "common", "rare", "epic", "legend", "vindi", "mythic", };

unsigned const upgrade_cost[]{0, 5, 15, 30, 75, 150};
std::map<const Card*, unsigned> dominion_cost[3][7];
std::map<const Card*, unsigned> dominion_refund[3][7];

unsigned min_possible_score[]{0, 0, 0, 20, 20, 5, 5, 5, 0
#ifndef NQUEST
, 0
#endif
};
unsigned max_possible_score[]{100, 100, 100, 208, 208, 100, 100, 100, 100
#ifndef NQUEST
, 100
#endif
};

std::string decktype_names[DeckType::num_decktypes]{"Deck", "Mission", "Raid", "Campaign", "Custom Deck", };

signed debug_print(0);
unsigned debug_cached(0);
bool debug_line(false);
std::string debug_str("");
