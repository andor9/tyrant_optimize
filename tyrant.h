#ifndef TYRANT_H_INCLUDED
#define TYRANT_H_INCLUDED

#define TYRANT_OPTIMIZER_VERSION "2.20.1"

#include <string>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <tuple>

enum Faction
{
    allfactions,
    imperial,
    raider,
    bloodthirsty,
    xeno,
    righteous,
    progenitor,
    num_factions
};
extern const std::string faction_names[num_factions];

enum Skill
{
    // Placeholder for no-skill:
    no_skill,
    // Attack:
    attack,
    // Activation:
    BEGIN_ACTIVATION, BEGIN_ACTIVATION_HARMFUL, // TODO skill traits
    enfeeble, jam, mortar, siege, strike, sunder, weaken,
    END_ACTIVATION_HARMFUL,
    BEGIN_ACTIVATION_HELPFUL,
    enhance, evolve, heal, mend, overload, protect, rally, rush,
    END_ACTIVATION_HELPFUL, END_ACTIVATION,
    // Defensive:
    BEGIN_DEFENSIVE,
    armor, avenge, corrosive, counter, evade, payback, refresh, wall,
    END_DEFENSIVE,
    // Combat-Modifier:
    legion, pierce, rupture, swipe, venom,
    // Damage-Dependent:
    berserk, inhibit, leech, poison,
    // Triggered:
    allegiance, flurry, valor,
    // Pseudo-Skill for BGE:
    BEGIN_BGE_SKILL,
    bloodlust, brigade, counterflux, divert, enduringrage, fortification, heroism,
    zealotspreservation, metamorphosis, revenge, turningtides, virulence,
    END_BGE_SKILL,
    num_skills
};
extern std::string skill_names[num_skills];

namespace CardType {
enum CardType {
    commander,
    assault,
    structure,
    num_cardtypes
};
}

extern std::string cardtype_names[CardType::num_cardtypes];

extern std::string rarity_names[];

extern unsigned upgrade_cost[];
extern unsigned salvaging_income[][7];

namespace DeckType {
enum DeckType {
    deck,
    mission,
    raid,
    campaign,
    custom_deck,
    num_decktypes
};
}

extern std::string decktype_names[DeckType::num_decktypes];

enum gamemode_t
{
    fight,
    surge,
};

namespace QuestType
{
enum QuestType
{
    none,
    skill_use,
    skill_damage,
    faction_assault_card_use,
    type_card_use,
    faction_assault_card_kill,
    type_card_kill,
    card_survival,
    num_objective_types
};
}

enum class OptimizationMode
{
    notset,
    winrate,
    defense,
    war,
    brawl,
    raid,
    campaign,
    quest,
    num_optimization_mode
};

extern signed min_possible_score[(size_t)OptimizationMode::num_optimization_mode];
extern signed max_possible_score[(size_t)OptimizationMode::num_optimization_mode];

struct true_ {};

struct false_ {};

template<unsigned>
struct skillTriggersRegen { typedef false_ T; };

template<>
struct skillTriggersRegen<strike> { typedef true_ T; };

template<>
struct skillTriggersRegen<siege> { typedef true_ T; };

enum SkillSourceType
{
    source_hostile,
    source_allied,
    source_global_hostile,
    source_global_allied,
    source_chaos
};

struct SkillSpec
{
    Skill id;
    unsigned x;
    Faction y;
    unsigned n;
    unsigned c;
    Skill s;
    Skill s2;
    bool all;
};

// --------------------------------------------------------------------------------
// Common functions
template<typename T>
std::string to_string(const T val)
{
    std::stringstream s;
    s << val;
    return s.str();
}

//---------------------- Debugging stuff ---------------------------------------
extern signed debug_print;
extern unsigned debug_cached;
extern bool debug_line;
extern std::string debug_str;
#ifndef NDEBUG
#define _DEBUG_MSG(v, format, args...)                                  \
    {                                                                   \
        if(__builtin_expect(debug_print >= v, false))                   \
        {                                                               \
            if(debug_line) { printf("%i - " format, __LINE__ , ##args); }      \
            else if(debug_cached) {                                     \
                char buf[4096];                                         \
                snprintf(buf, sizeof(buf), format, ##args);             \
                debug_str += buf;                                       \
            }                                                           \
            else { printf(format, ##args); }                            \
            std::cout << std::flush;                                    \
        }                                                               \
    }
#define _DEBUG_SELECTION(format, args...)                               \
    {                                                                   \
        if(__builtin_expect(debug_print >= 2, 0))                       \
        {                                                               \
            _DEBUG_MSG(2, "Possible targets of " format ":\n", ##args); \
            fd->print_selection_array();                                \
        }                                                               \
    }
#else
#define _DEBUG_MSG(v, format, args...)
#define _DEBUG_SELECTION(format, args...)
#endif

#endif
