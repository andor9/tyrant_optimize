#ifndef TYRANT_H_INCLUDED
#define TYRANT_H_INCLUDED

#define TYRANT_OPTIMIZER_VERSION "1.1.5"

#include <string>
#include <set>
#include <tuple>

enum Faction
{
    allfactions,
    bloodthirsty,
    imperial,
    raider,
    righteous,
    xeno,
    num_factions
};
extern const std::string faction_names[num_factions];

enum Skill
{
    // Attack:
    attack,
    // Activation (including Destroyed):
    augment, backfire, chaos, cleanse, enfeeble, freeze, heal, infuse, jam,
    mimic, protect, rally, recharge, repair, rush, shock, siege, split, strike, summon, supply,
    trigger_regen, // not actually a skill; handles regeneration after strike/siege
    weaken, 
    // Combat-Modifier:
    antiair, burst, fear, flurry, pierce, swipe, valor,
    // Damage-Dependant:
    berserk, crush, disease, immobilize, leech, phase, poison, siphon, sunder,
    // Defensive:
    armored, counter, emulate, evade, flying, intercept, payback, refresh, regenerate, stun, tribute, wall,
    // Triggered:
    blitz, legion,
    // Static, ignored:
    /* blizzard, fusion, mist, */
    // Placeholder for new gained skill from battleground effect:
    new_skill,
    num_skills
};
extern std::string skill_names[num_skills];
extern std::set<Skill> helpful_skills;

namespace SkillMod {
enum SkillMod
{
    on_activate,
    on_play,
    on_attacked,
    on_kill,
    on_death,
    num_skill_activation_modifiers
};
}
extern std::string skill_activation_modifier_names[SkillMod::num_skill_activation_modifiers];

namespace CardType {
enum CardType {
    commander,
    assault,
    structure,
    action,
    num_cardtypes
};
}

extern std::string cardtype_names[CardType::num_cardtypes];

extern std::string rarity_names[5];

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
    num_effects
};

extern std::string effect_names[Effect::num_effects];

enum AchievementMiscReq
{
    unit_with_flying_killed,  // 104 Sky Control
    skill_activated,  // 105 Brute Strength
    turns,  // all "Speedy" and "Slow"
    damage,  // 168 SMASH!; 183 Rally Free Zone
    com_total,  // 169 Overkill; 170 EXTREME Overkill!!!
    num_achievement_misc_reqs
};

extern std::string achievement_misc_req_names[num_achievement_misc_reqs];

enum gamemode_t
{
    fight,
    surge,
    tournament
};

enum class OptimizationMode
{
    winrate,
    achievement,
    raid,
    defense
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

typedef std::tuple<Skill, unsigned, Faction, bool /* all */, SkillMod::SkillMod> SkillSpec;

#endif
