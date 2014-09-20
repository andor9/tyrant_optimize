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
    "<Error>",
    "Enfeeble", "Jam", "Siege", "Strike", "Weaken",
    "<Error>",
    "<Error>",
    "Enhance", "Heal", "Overload", "Protect", "Rally",
    "<Error>",
    // Defensive:
    "<Error>",
    "Armor", "Corrosive", "Counter", "Evade", "Wall",
    "<Error>",
    // Combat-Modifier:
    "Flurry", "Pierce", "Valor",
    // Damage-Dependant:
    "Berserk", "Inhibit", "Leech", "Poison",
};

std::string cardtype_names[CardType::num_cardtypes]{"Commander", "Assault", "Structure", };

std::string rarity_names[6]{"", "common", "rare", "epic", "legendary", "vindicator", };

unsigned upgrade_cost[]{0, 5, 15, 30, 75, 150};
unsigned salvaging_income[][7]{{}, {0, 1, 2, 5}, {0, 5, 10, 15, 20}, {0, 20, 25, 30, 40, 50, 65}, {0, 40, 45, 60, 75, 100, 125}, {0, 80, 85, 100, 125, 175, 250}};

std::string decktype_names[DeckType::num_decktypes]{"Deck", "Mission", "Raid", "Quest", "Custom Deck", };

std::string effect_names[Effect::num_effects] = {
    "None",
    "Armor 1",
    "Armor 2",
    "Armor 3",
    "Berserk 1",
    "Berserk 2",
    "Berserk 3",
    "Corrosive 1",
    "Corrosive 2",
    "Corrosive 3",
    "Counter 1",
    "Counter 2",
    "Counter 3",
    "Enfeeble 1",
    "Enfeeble 2",
    "Enfeeble 3",
    "Evade 1",
    "Evade 2",
    "Evade 3",
    "Heal 1",
    "Heal 2",
    "Heal 3",
    "Leech 1",
    "Leech 2",
    "Leech 3",
    "Overload 1",
    "Overload 2",
    "Overload 3",
    "Poison 1",
    "Poison 2",
    "Poison 3",
    "Progenitor",
    "Rally 1",
    "Rally 2",
    "Rally 3",
    "Strike 1",
    "Strike 2",
    "Strike 3",
    "Time Surge",
    "Copycat",
    "Quicksilver",
    "Decay",
    "High Skies",
    "Impenetrable",
    "Invigorate",
    "Clone Project",
    "Friendly Fire",
    "Genesis",
    "Artillery Strike",
    "Photon Shield",
    "Decrepit",
    "Forcefield",
    "Chilling Touch",
    "Clone Experiment",
    "Toxic",
    "Haunt",
    "United Front",
    "Harsh Conditions",
    "Metamorphosis",
};

signed debug_print(0);
unsigned debug_cached(0);
bool debug_line(false);
std::string debug_str("");
