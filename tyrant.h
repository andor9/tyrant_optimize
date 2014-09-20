#ifndef TYRANT_H_INCLUDED
#define TYRANT_H_INCLUDED

#define TYRANT_OPTIMIZER_VERSION "2.1.6"

#include <string>
#include <sstream>
#include <unordered_set>
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
    BEGIN_ACTIVATION_HARMFUL, // TODO skill traits
    enfeeble, jam, siege, strike, weaken,
    END_ACTIVATION_HARMFUL,
    BEGIN_ACTIVATION_HELPFUL,
    enhance, heal, overload, protect, rally,
    END_ACTIVATION_HELPFUL,
    // Defensive:
    BEGIN_DEFENSIVE,
    armor, corrosive, counter, evade, wall,
    END_DEFENSIVE,
    // Combat-Modifier:
    flurry, pierce, valor,
    // Damage-Dependant:
    berserk, inhibit, leech, poison,
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
    quest,
    custom_deck,
    num_decktypes
};
}

extern std::string decktype_names[DeckType::num_decktypes];

enum Effect {
    none,
    armored_1,
    armored_2,
    armored_3,
    berserk_1,
    berserk_2,
    berserk_3,
    corrosive_1,
    corrosive_2,
    corrosive_3,
    counter_1,
    counter_2,
    counter_3,
    enfeeble_1,
    enfeeble_2,
    enfeeble_3,
    evade_1,
    evade_2,
    evade_3,
    heal_1,
    heal_2,
    heal_3,
    leech_1,
    leech_2,
    leech_3,
    overload_1,
    overload_2,
    overload_3,
    poison_1,
    poison_2,
    poison_3,
    bg_progenitor,
    rally_1,
    rally_2,
    rally_3,
    strike_1,
    strike_2,
    strike_3,
    time_surge,
    copycat,
    quicksilver,
    decay,
    high_skies,
    impenetrable,
    invigorate,
    clone_project,
    friendly_fire,
    genesis,
    artillery_strike,
    photon_shield,
    decrepit,
    forcefield,
    chilling_touch,
    clone_experiment,
    toxic,
    haunt,
    united_front,
    harsh_conditions,
    metamorphosis,
    num_effects
};

extern std::string effect_names[Effect::num_effects];

enum gamemode_t
{
    fight,
    surge,
};

enum class OptimizationMode
{
    notset,
    winrate,
    defense,
    war,
    brawl,
    raid,
};

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
