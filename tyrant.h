#ifndef TYRANT_H_INCLUDED
#define TYRANT_H_INCLUDED

#ifndef TYRANT_OPTIMIZER_VERSION
#define TYRANT_OPTIMIZER_VERSION "NO VERSION"
#endif

#include <string>
#include <sstream>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <tuple>
#include <boost/algorithm/string.hpp>


enum Fix
{
    no_fix,
    enhance_early,
    revenge_on_death,
    death_from_bge,
    legion_under_mega,
    barrier_each_turn,
    dont_evade_mimic_selection,
    leech_increase_max_hp,
    counter_without_damage,
    subdue_before_attack,
    corrosive_protect_armor,
    poison_after_attacked,
    num_fixes
};

class Card;

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

namespace Skill {
enum Skill
{
    // Placeholder for no-skill:
    no_skill,

    // Activation (harmful):
    enfeeble, jam, mortar, siege, strike, sunder, weaken,

    // Activation (helpful):
    evolve, heal, mend, overload, protect, rally, fortify,
    enrage, entrap, rush,

    // Activation (unclassified/polymorphic):
    mimic,

    // Defensive:
    armor, avenge, scavenge, corrosive, counter, evade, subdue, absorb, flying,
    payback, revenge, tribute, refresh, wall, barrier,

    // Combat-Modifier:
    coalition, legion, pierce, rupture, swipe, drain, venom, hunt,mark,

    // Damage-Dependent:
    berserk,  leech, poison,

    // Instant-Debuff:
    inhibit, sabotage, disease,

    // Triggered:
    allegiance, flurry, valor, stasis, summon, bravery, enhance,

    // End of skills
    num_skills
};
enum Trigger
{
    activate,
    play,
    attacked,
    death,
    num_triggers
};
}
extern const std::string skill_names[Skill::num_skills];
extern const std::string skill_trigger_names[Skill::num_triggers];


namespace PassiveBGE {
enum PassiveBGE
{
    // Placeholder for no-bge:
    no_bge,

    // Passive BGEs
    bloodlust, brigade, counterflux, divert, enduringrage, fortification, heroism,
    zealotspreservation, metamorphosis, megamorphosis, revenge, turningtides, virulence,
    haltedorders, devour, criticalreach, temporalbacklash, furiosity, oath_of_loyalty,
    bloodvengeance, coldsleep,ironwill,unity,devotion,crackdown,superheroism,

    // End of BGEs
    num_passive_bges
};
}
extern const std::string passive_bge_names[PassiveBGE::num_passive_bges];


inline bool is_activation_harmful_skill(Skill::Skill skill_id)
{
    switch(skill_id)
    {
    case Skill::enfeeble:
    case Skill::jam:
    case Skill::mortar:
    case Skill::siege:
    case Skill::strike:
    case Skill::sunder:
    case Skill::weaken:
        return true;
    default:
        return false;
    }
}

inline bool is_activation_hostile_skill(Skill::Skill skill_id)
{
    switch(skill_id)
    {
    case Skill::mimic:
        return true;
    default:
        return is_activation_harmful_skill(skill_id);
    }
}

inline bool is_activation_helpful_skill(Skill::Skill skill_id)
{
    switch(skill_id)
    {
    //case Skill::enhance:
    case Skill::evolve:
    case Skill::heal:
    case Skill::mend:
    case Skill::fortify:
    case Skill::overload:
    case Skill::protect:
    case Skill::rally:
    case Skill::enrage:
    case Skill::entrap:
    case Skill::rush:
        return true;
    default:
        return false;
    }
}

inline bool is_activation_allied_skill(Skill::Skill skill_id)
{
    return is_activation_helpful_skill(skill_id);
}

inline bool is_activation_skill(Skill::Skill skill_id)
{
    return is_activation_hostile_skill(skill_id)
        || is_activation_allied_skill(skill_id)
    ;
}

inline bool is_activation_skill_with_x(Skill::Skill skill_id)
{
    switch(skill_id)
    {
    case Skill::enfeeble:
    case Skill::mortar:
    case Skill::siege:
    case Skill::strike:
    case Skill::sunder:
    case Skill::weaken:
    //case Skill::enhance:
    case Skill::mimic:
    case Skill::heal:
    case Skill::mend:
    case Skill::fortify:
    case Skill::protect:
    case Skill::rally:
    case Skill::enrage:
    case Skill::entrap:
    case Skill::rush:
        return true;
    default:
        return false;
    }
}

inline bool is_defensive_skill(Skill::Skill skill_id)
{
    switch(skill_id)
    {
    case Skill::armor:
    case Skill::avenge:
    case Skill::scavenge:
    case Skill::corrosive:
    case Skill::counter:
    case Skill::evade:
    case Skill::subdue:
    case Skill::absorb:
    case Skill::flying:
    case Skill::payback:
    case Skill::revenge:
    case Skill::tribute:
    case Skill::refresh:
    case Skill::wall:
        return true;
    default:
        return false;
    }
}

inline bool is_combat_modifier_skill(Skill::Skill skill_id)
{
    switch(skill_id)
    {
    case Skill::coalition:
    case Skill::legion:
    case Skill::pierce:
    case Skill::rupture:
    case Skill::swipe:
    case Skill::drain:
    case Skill::venom:
    case Skill::hunt:
    case Skill::mark:
        return true;
    default:
        return false;
    }
}

inline bool is_instant_debuff_skill(Skill::Skill skill_id)
{
    switch(skill_id)
    {
    case Skill::inhibit:
    case Skill::sabotage:
    case Skill::disease:
        return true;
    default:
        return false;
    }
}

inline bool is_damage_dependent_skill(Skill::Skill skill_id)
{
    switch(skill_id)
    {
    case Skill::berserk:
    case Skill::leech:
    case Skill::poison:
        return true;
    default:
        return false;
    }
}

inline bool is_triggered_skill(Skill::Skill skill_id)
{
    switch(skill_id)
    {
    case Skill::allegiance:
    case Skill::flurry:
    case Skill::valor:
    case Skill::stasis:
    case Skill::summon:
    case Skill::bravery:
        return true;
    default:
        return false;
    }
}

inline PassiveBGE::PassiveBGE passive_bge_name_to_id(const std::string& name_)
{
    std::string name(name_);
    name.erase(std::remove_if(name.begin(), name.end(), boost::is_any_of("-")), name.end());
    for (unsigned i(PassiveBGE::no_bge); i < PassiveBGE::num_passive_bges; ++i)
    {
        if (boost::iequals(passive_bge_names[i], name))
        {
            return static_cast<PassiveBGE::PassiveBGE>(i);
        }
    }
    return PassiveBGE::no_bge;
}

inline Faction faction_name_to_id(const std::string& name_)
{
    std::string name(name_);
    name.erase(std::remove_if(name.begin(), name.end(), boost::is_any_of("-")), name.end()); //Mostly useless
    for (unsigned i(allfactions); i < num_factions; ++i)
    {
        if (boost::iequals(faction_names[i], name))
        {
            return static_cast<Faction>(i);
        }
    }
    return allfactions;
}

inline void map_keys_to_set(const std::unordered_map<unsigned, unsigned>& m, std::unordered_set<unsigned>& s)
{
    for (auto it = m.begin(); it != m.end(); ++it)
    {
        s.insert(it->first);
    }
}

inline unsigned safe_minus(unsigned x, unsigned y)
{
    return (x > y) ? (x - y) : 0;
}

namespace CardType {
enum CardType {
    commander,
    assault,
    structure,
    num_cardtypes
};
}

namespace CardCategory {
enum CardCategory {
    normal,
    special,
    fortress_defense,
    fortress_siege,
    fortress_conquest,
    dominion_alpha,
    dominion_material,
    num_cardcategories
};
}

extern const std::string cardtype_names[CardType::num_cardtypes];

extern const std::string rarity_names[];

extern unsigned const upgrade_cost[];
extern std::map<const Card*, unsigned> dominion_cost[3][7];
extern std::map<const Card*, unsigned> dominion_refund[3][7];

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

#ifndef NQUEST
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
#endif

enum OptimizationMode
{
    notset,
    winrate,
    defense,
    war,
    war_defense,
    brawl,
    brawl_defense,
    raid,
    campaign,
#ifndef NQUEST
    quest,
#endif
    num_optimization_mode
};


extern unsigned min_possible_score[(size_t)OptimizationMode::num_optimization_mode];
extern unsigned max_possible_score[(size_t)OptimizationMode::num_optimization_mode];

template <typename x_type>
struct _SkillSpec
{
    Skill::Skill id;
    x_type x;
    Faction y;
    unsigned n;
    unsigned c;
    Skill::Skill s;
    Skill::Skill s2;
    bool all;
    unsigned card_id;
};

using SkillSpec = _SkillSpec<unsigned>;
using SkillSpecXMult = _SkillSpec<double>;

// --------------------------------------------------------------------------------
// Common functions
namespace tuo {
template<typename T>
std::string to_string(const T val)
{
    std::stringstream s;
    s << val;
    return s.str();
}
}
inline uint8_t byte_bits_count(uint8_t i)
{
    i = i - ((i >> 1) & 0x55);
    i = (i & 0x33) + ((i >> 2) & 0x33);
    return (i + (i >> 4)) & 0x0F;
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
            if(debug_cached) {                                     \
                char buf[4096];                                         \
                if(debug_line){ snprintf(buf, sizeof(buf), "%i - " format, __LINE__, ##args);}  \
                  else { snprintf(buf, sizeof(buf), format, ##args); }            \
                debug_str += buf;                                       \
            }                                                           \
            else { if(debug_line){ printf("%i - " format, __LINE__ , ##args);} \
                   else {printf(format, ##args);}                             \
            std::cout << std::flush;  }                                  \
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
#define _DEBUG_ASSERT(expr) { assert(expr); }
#else
#define _DEBUG_MSG(v, format, args...)
#define _DEBUG_SELECTION(format, args...)
#define _DEBUG_ASSERT(expr)
#endif


#endif
