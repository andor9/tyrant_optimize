#include "tyrant.h"

#include <string>

const std::string faction_names[Faction::num_factions] =
{ "", "imperial", "raider", "bloodthirsty", "xeno", "righteous", "progenitor" };

std::string skill_names[Skill::num_skills] =
{
    // Placeholder for no-skill:
    "<Error>",
    // Attack:
    "0",
    // Activation:
    "<Error>", "<Error>",
    "Enfeeble", "Jam", "Mortar", "Siege", "Strike", "Sunder", "Weaken",
    "<Error>",
    "<Error>",
    "Enhance", "Evolve", "Heal", "Mend", "Overload", "Protect", "Rally", "Rush",
    "<Error>", "<Error>",
    // Defensive:
    "<Error>",
    "Armor", "Avenge", "Corrosive", "Counter", "Evade", "Payback", "Refresh", "Wall",
    "<Error>",
    // Combat-Modifier:
    "Legion", "Pierce", "Rupture", "Swipe", "Venom",
    // Damage-Dependant:
    "Berserk", "Inhibit", "Leech", "Poison",
    // Triggered:
    "Allegiance", "Flurry", "Valor",
    // Pseudo-skill for passive BGEs:
    "<Error>",
    "Bloodlust", "Brigade", "Counterflux", "Divert", "EnduringRage", "Fortification", "Heroism",
    "ZealotsPreservation", "Metamorphosis", "Revenge", "TurningTides", "Virulence",
    "<Error>",
};

std::string cardtype_names[CardType::num_cardtypes]{"Commander", "Assault", "Structure", };

std::string rarity_names[6]{"", "common", "rare", "epic", "legend", "vindi", };

unsigned upgrade_cost[]{0, 5, 15, 30, 75, 150};
unsigned salvaging_income[][7]{{}, {0, 1, 2, 5}, {0, 5, 10, 15, 20}, {0, 20, 25, 30, 40, 50, 65}, {0, 40, 45, 60, 75, 100, 125}, {0, 80, 85, 100, 125, 175, 250}};

signed min_possible_score[]{0, 0, 0, 10, 5, 5, 0, 0};
signed max_possible_score[]{100, 100, 100, 100, 67, 100, 100, 100};

std::string decktype_names[DeckType::num_decktypes]{"Deck", "Mission", "Raid", "Campaign", "Custom Deck", };

signed debug_print(0);
unsigned debug_cached(0);
bool debug_line(false);
std::string debug_str("");
