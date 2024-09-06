#include "sim.h"

#include <boost/range/adaptors.hpp>
#include <boost/range/join.hpp>
#include <iostream>
#include <random>
#include <string>
#include <sstream>
#include <vector>
#include <cmath>

#include "card.h"
#include "cards.h"
#include "deck.h"

bool check_and_perform_valor(Field* fd, CardStatus* src);
bool check_and_perform_bravery(Field* fd, CardStatus* src);
bool check_and_perform_early_enhance(Field* fd, CardStatus* src);
bool check_and_perform_later_enhance(Field* fd, CardStatus* src);
CardStatus* check_and_perform_summon(Field* fd, CardStatus* src);
//------------------------------------------------------------------------------
inline unsigned remove_absorption(Field* fd, CardStatus* status, unsigned dmg);
inline unsigned remove_absorption(CardStatus* status, unsigned dmg);
inline unsigned remove_disease(CardStatus* status, unsigned heal);
//------------------------------------------------------------------------------
inline bool has_attacked(CardStatus* c) { return (c->m_step == CardStep::attacked); }
inline bool is_alive(CardStatus* c) { return (c->m_hp > 0); }
inline bool can_act(CardStatus* c) { return is_alive(c) && !c->m_jammed; }
inline bool is_active(CardStatus* c) { return can_act(c) && (c->m_delay == 0); }
inline bool is_active_next_turn(CardStatus* c) { return can_act(c) && (c->m_delay <= 1); }
inline bool will_activate_this_turn(CardStatus* c) { return is_active(c) && ((c->m_step == CardStep::none) || (c->m_step == CardStep::attacking && c->has_skill(Skill::flurry) && c->m_action_index < c->skill_base_value(Skill::flurry)));}
// Can be healed / repaired
inline bool can_be_healed(CardStatus* c) { return is_alive(c) && (c->m_hp < c->max_hp()); }
// Strange Transmission [Gilians] features
#ifdef TUO_GILIAN
inline bool is_gilian(CardStatus* c) { return(
        (c->m_card->m_id >= 25054 && c->m_card->m_id <= 25063) // Gilian Commander
        ||  (c->m_card->m_id >= 38348 && c->m_card->m_id <= 38388) // Gilian assaults plus the Gil's Shard
        ); }
inline bool is_alive_gilian(CardStatus* c) { return(is_alive(c) && is_gilian(c)); }
#else
# define is_gilian(c) (false)
# define is_alive_gilian(c) (false)
#endif

//------------------------------------------------------------------------------
inline std::string status_description(const CardStatus* status)
{
    return status->description();
}
//------------------------------------------------------------------------------
template <typename CardsIter, typename Functor>
inline unsigned Field::make_selection_array(CardsIter first, CardsIter last, Functor f)
{
    this->selection_array.clear();
    for(auto c = first; c != last; ++c)
    {
        if (f(*c))
        {
            this->selection_array.push_back(*c);
        }
    }
    return(this->selection_array.size());
}
inline CardStatus * Field::left_assault(const CardStatus * status)
{
    return left_assault(status, 1);
}
inline CardStatus * Field::left_assault(const CardStatus * status, const unsigned n)
{
    auto & assaults = this->players[status->m_player]->assaults;
    if (status->m_index >= n)
    {
        auto left_status = &assaults[status->m_index - n];
        if (is_alive(left_status))
        {
            return left_status;
        }
    }
    return nullptr;
}
inline CardStatus * Field::right_assault(const CardStatus * status)
{
    return right_assault(status, 1);
}
inline CardStatus * Field::right_assault(const CardStatus * status, const unsigned n)
{
    auto & assaults = this->players[status->m_player]->assaults;
    if ((status->m_index + n) < assaults.size())
    {
        auto right_status = &assaults[status->m_index + n];
        if (is_alive(right_status))
        {
            return right_status;
        }
    }
    return nullptr;
}
inline void Field::print_selection_array()
{
#ifndef NDEBUG
    for(auto c: this->selection_array)
    {
        _DEBUG_MSG(2, "+ %s\n", status_description(c).c_str());
    }
#endif
}

//------------------------------------------------------------------------------
inline void Field::prepare_action()
{
    damaged_units_to_times.clear();
}

//------------------------------------------------------------------------------
inline void Field::finalize_action()
{
    for (auto unit_it = damaged_units_to_times.begin(); unit_it != damaged_units_to_times.end(); ++ unit_it)
    {
        if (__builtin_expect(!unit_it->second, false))
        { continue; }
        CardStatus * dmg_status = unit_it->first;
        if (__builtin_expect(!is_alive(dmg_status), false))
        { continue; }
        unsigned barrier_base = dmg_status->skill(Skill::barrier);
        if (barrier_base)
        {
            unsigned protect_value = barrier_base * unit_it->second;
            _DEBUG_MSG(1, "%s protects itself for %u (barrier %u x %u damage taken)\n",
                    status_description(dmg_status).c_str(), protect_value, barrier_base, unit_it->second);
            dmg_status->m_protected += protect_value;
        }
    }
}

//------------------------------------------------------------------------------
inline unsigned CardStatus::skill_base_value(Skill::Skill skill_id) const
{
    return m_card->m_skill_value[skill_id + m_primary_skill_offset[skill_id]]
        + (skill_id == Skill::berserk ? m_enraged : 0)
        + (skill_id == Skill::counter ? m_entrapped : 0)
        ;
}
//------------------------------------------------------------------------------
inline unsigned CardStatus::skill(Skill::Skill skill_id) const
{
    return (is_activation_skill_with_x(skill_id)
            ? safe_minus(skill_base_value(skill_id), m_sabotaged)
            : skill_base_value(skill_id))
        + enhanced(skill_id);
}
//------------------------------------------------------------------------------
inline bool CardStatus::has_skill(Skill::Skill skill_id) const
{
    return skill_base_value(skill_id);
}
//------------------------------------------------------------------------------
inline unsigned CardStatus::enhanced(Skill::Skill skill_id) const
{
    return m_enhanced_value[skill_id + m_primary_skill_offset[skill_id]];
}
//------------------------------------------------------------------------------
inline unsigned CardStatus::protected_value() const
{
    return m_protected + m_protected_stasis;
}
//------------------------------------------------------------------------------
inline unsigned CardStatus::max_hp() const
{
    return (m_card->m_health + safe_minus(m_perm_health_buff, m_subdued));
}
/** @brief Increase of current health.
 *  This takes disease into account and removes as needed.
 *
 *  @param [in] value increase of health.
 *  @return applied increase of health.
 */
inline unsigned CardStatus::add_hp(unsigned value)
{
    value = remove_disease(this,value);
    return (m_hp = std::min(m_hp + value, max_hp()));
}
/** @brief Permanent increase of maximum health.
 *  This takes disease into account and removes as needed.
 *  The increase of the maximum health entails an increase of current health by the same amount.
 *
 *  @param [in] value increase of maximum health.
 *  @return applied increase of maximum health.
 */ 
inline unsigned CardStatus::ext_hp(unsigned value)
{
    value = remove_disease(this,value);
    m_perm_health_buff += value;
    // we can safely call add_hp without worring about the second call to remove_disease because
    // the first call will have already removed the disease or the value will be 0
    return add_hp(value);
}
//------------------------------------------------------------------------------
inline void CardStatus::set(const Card* card)
{
    this->set(*card);
}
//------------------------------------------------------------------------------
inline void CardStatus::set(const Card& card)
{
    m_card = &card;
    m_index = 0;
    m_action_index=0;
    m_player = 0;
    m_delay = card.m_delay;
    m_hp = card.m_health;
    m_absorption = 0;
    m_step = CardStep::none;
    m_perm_health_buff = 0;
    m_perm_attack_buff = 0;
    m_temp_attack_buff = 0;
    m_corroded_rate = 0;
    m_corroded_weakened = 0;
    m_subdued = 0;
    m_enfeebled = 0;
    m_evaded = 0;
    m_inhibited = 0;
    m_sabotaged = 0;
    m_jammed = false;
    m_overloaded = false;
    m_paybacked = 0;
    m_tributed = 0;
    m_poisoned = 0;
    m_protected = 0;
    m_protected_stasis = 0;
    m_enraged = 0;
    m_entrapped = 0;
    m_marked = 0;
    m_diseased = 0;
    m_rush_attempted = false;
    m_sundered = false;
    //APN
    m_summoned = false;

    std::memset(m_primary_skill_offset, 0, sizeof m_primary_skill_offset);
    std::memset(m_evolved_skill_offset, 0, sizeof m_evolved_skill_offset);
    std::memset(m_enhanced_value, 0, sizeof m_enhanced_value);
    std::memset(m_skill_cd, 0, sizeof m_skill_cd);
}
//------------------------------------------------------------------------------
inline unsigned CardStatus::attack_power() const
{
    signed attack = calc_attack_power();
    if(__builtin_expect(attack <0,false))
    {
        std::cout << m_card->m_name << " " << m_card->m_attack  << " " << attack << " " << m_temp_attack_buff << " " << m_corroded_weakened << std::endl;
    }
    _DEBUG_ASSERT(attack >= 0);
    return (unsigned)attack;
}

inline signed CardStatus::calc_attack_power() const
{
    return
        (signed)safe_minus(
                m_card->m_attack + safe_minus(m_perm_attack_buff, m_subdued),
                m_corroded_weakened
                )
        + m_temp_attack_buff;
}
//------------------------------------------------------------------------------
const Card* card_by_id_safe(const Cards& cards, const unsigned card_id)
{
    const auto cardIter = cards.cards_by_id.find(card_id);
    if (cardIter == cards.cards_by_id.end()) assert(false);//"UnknownCard.id[" + to_string(card_id) + "]"); }
    return cardIter->second;
}
std::string card_name_by_id_safe(const Cards& cards, const unsigned card_id)
{
    const auto cardIter = cards.cards_by_id.find(card_id);
    if (cardIter == cards.cards_by_id.end()) { return "UnknownCard.id[" + tuo::to_string(card_id) + "]"; }
    return cardIter->second->m_name;
}
//------------------------------------------------------------------------------
std::string card_description(const Cards& cards, const Card* c)
{
    std::string desc;
    desc = c->m_name;
    switch(c->m_type)
    {
        case CardType::assault:
            desc += ": " + tuo::to_string(c->m_attack) + "/" + tuo::to_string(c->m_health) + "/" + tuo::to_string(c->m_delay);
            break;
        case CardType::structure:
            desc += ": " + tuo::to_string(c->m_health) + "/" + tuo::to_string(c->m_delay);
            break;
        case CardType::commander:
            desc += ": hp:" + tuo::to_string(c->m_health);
            break;
        case CardType::num_cardtypes:
            assert(false);
            break;
    }
    if (c->m_rarity >= 4) { desc += " " + rarity_names[c->m_rarity]; }
    if (c->m_faction != allfactions) { desc += " " + faction_names[c->m_faction]; }
    for (auto& skill: c->m_skills_on_play) { desc += ", " + skill_description(cards, skill, Skill::Trigger::play); }
    for (auto& skill: c->m_skills) { desc += ", " + skill_description(cards, skill, Skill::Trigger::activate); }
    //APN
    for (auto& skill: c->m_skills_on_attacked) { desc += ", " + skill_description(cards, skill, Skill::Trigger::attacked); }
    for (auto& skill: c->m_skills_on_death) { desc += ", " + skill_description(cards, skill, Skill::Trigger::death); }
    return desc;
}
//------------------------------------------------------------------------------
std::string CardStatus::description() const
{
    std::string desc = "P" + tuo::to_string(m_player) + " ";
    switch(m_card->m_type)
    {
        case CardType::commander: desc += "Commander "; break;
        case CardType::assault: desc += "Assault " + tuo::to_string(m_index) + " "; break;
        case CardType::structure: desc += "Structure " + tuo::to_string(m_index) + " "; break;
        case CardType::num_cardtypes: assert(false); break;
    }
    desc += "[" + m_card->m_name;
    switch (m_card->m_type)
    {
        case CardType::assault:
            desc += " att:[[" + tuo::to_string(m_card->m_attack) + "(base)";
            if (m_perm_attack_buff)
            {
                desc += "+[" + tuo::to_string(m_perm_attack_buff) + "(perm)";
                if (m_subdued) { desc += "-" + tuo::to_string(m_subdued) + "(subd)"; }
                desc += "]";
            }
            if (m_corroded_weakened) { desc += "-" + tuo::to_string(m_corroded_weakened) + "(corr)"; }
            desc += "]";
            if (m_temp_attack_buff) { desc += (m_temp_attack_buff > 0 ? "+" : "") + tuo::to_string(m_temp_attack_buff) + "(temp)"; }
            desc += "]=" + tuo::to_string(attack_power());
        case CardType::structure:
        case CardType::commander:
            desc += " hp:" + tuo::to_string(m_hp);
            if(m_absorption)desc += " absorb:" + tuo::to_string(m_absorption);
            break;
        case CardType::num_cardtypes:
            assert(false);
            break;
    }
    if (m_delay) { desc += " cd:" + tuo::to_string(m_delay); }
    // Status w/o value
    if (m_jammed) { desc += ", jammed"; }
    if (m_overloaded) { desc += ", overloaded"; }
    if (m_sundered) { desc += ", sundered"; }
    if (m_summoned) { desc+= ", summoned"; }
    // Status w/ value
    if (m_corroded_weakened || m_corroded_rate) { desc += ", corroded " + tuo::to_string(m_corroded_weakened) + " (rate: " + tuo::to_string(m_corroded_rate) + ")"; }
    if (m_subdued) { desc += ", subdued " + tuo::to_string(m_subdued); }
    if (m_enfeebled) { desc += ", enfeebled " + tuo::to_string(m_enfeebled); }
    if (m_inhibited) { desc += ", inhibited " + tuo::to_string(m_inhibited); }
    if (m_sabotaged) { desc += ", sabotaged " + tuo::to_string(m_sabotaged); }
    if (m_poisoned) { desc += ", poisoned " + tuo::to_string(m_poisoned); }
    if (m_protected) { desc += ", protected " + tuo::to_string(m_protected); }
    if (m_protected_stasis) { desc += ", stasis " + tuo::to_string(m_protected_stasis); }
    if (m_enraged) { desc += ", enraged " + tuo::to_string(m_enraged); }
    if (m_entrapped) { desc += ", entrapped " + tuo::to_string(m_entrapped); }
    if (m_marked) { desc += ", marked " + tuo::to_string(m_marked); }
    if (m_diseased) { desc += ", diseased " + tuo::to_string(m_diseased); }
    //    if(m_step != CardStep::none) { desc += ", Step " + to_string(static_cast<int>(m_step)); }
    //APN
    const Skill::Trigger s_triggers[] = { Skill::Trigger::play, Skill::Trigger::activate, Skill::Trigger::death , Skill::Trigger::attacked};
    for (const Skill::Trigger& trig: s_triggers)
    {
        std::vector<SkillSpec> card_skills(
                (trig == Skill::Trigger::play) ? m_card->m_skills_on_play :
                (trig == Skill::Trigger::activate) ? m_card->m_skills :
                //APN
                (trig == Skill::Trigger::attacked) ? m_card->m_skills_on_attacked :
                (trig == Skill::Trigger::death) ? m_card->m_skills_on_death :
                std::vector<SkillSpec>());

        // emulate Berserk/Counter by status Enraged/Entrapped unless such skills exist (only for normal skill triggering)
        if (trig == Skill::Trigger::activate)
        {
            if (m_enraged && !std::count_if(card_skills.begin(), card_skills.end(), [](const SkillSpec ss) { return (ss.id == Skill::berserk); }))
            {
                SkillSpec ss{Skill::berserk, m_enraged, allfactions, 0, 0, Skill::no_skill, Skill::no_skill, false, 0,};
                card_skills.emplace_back(ss);
            }
            if (m_entrapped && !std::count_if(card_skills.begin(), card_skills.end(), [](const SkillSpec ss) { return (ss.id == Skill::counter); }))
            {
                SkillSpec ss{Skill::counter, m_entrapped, allfactions, 0, 0, Skill::no_skill, Skill::no_skill, false, 0,};
                card_skills.emplace_back(ss);
            }
        }
        for (const auto& ss : card_skills)
        {
            std::string skill_desc;
            if (m_evolved_skill_offset[ss.id]) { skill_desc += "->" + skill_names[ss.id + m_evolved_skill_offset[ss.id]]; }
            if (m_enhanced_value[ss.id]) { skill_desc += " +" + tuo::to_string(m_enhanced_value[ss.id]); }
            if (!skill_desc.empty())
            {
                desc += ", " + (
                        (trig == Skill::Trigger::play) ? "(On Play)" :
                        (trig == Skill::Trigger::attacked) ? "(On Attacked)" :
                        (trig == Skill::Trigger::death) ? "(On Death)" :
                        std::string("")) + skill_names[ss.id] + skill_desc;
            }
        }
    }
    return desc + "]";
}
//------------------------------------------------------------------------------
void Hand::reset(std::mt19937& re)
{
    assaults.reset();
    structures.reset();
    deck->shuffle(re);
    commander.set(deck->shuffled_commander);
    total_cards_destroyed = 0;
    total_nonsummon_cards_destroyed = 0;
    if (commander.skill(Skill::stasis))
    {
        stasis_faction_bitmap |= (1u << commander.m_card->m_faction);
    }
}

//---------------------- $40 Game rules implementation -------------------------
// Everything about how a battle plays out, except the following:
// the implementation of the attack by an assault card is in the next section;
// the implementation of the active skills is in the section after that.
unsigned turn_limit{50};

//------------------------------------------------------------------------------
inline unsigned opponent(unsigned player)
{
    return((player + 1) % 2);
}

//------------------------------------------------------------------------------
SkillSpec apply_evolve(const SkillSpec& s, signed offset)
{
    SkillSpec evolved_s = s;
    evolved_s.id = static_cast<Skill::Skill>(evolved_s.id + offset);
    return(evolved_s);
}

//------------------------------------------------------------------------------
SkillSpec apply_enhance(const SkillSpec& s, unsigned enhanced_value)
{
    SkillSpec enahnced_s = s;
    enahnced_s.x += enhanced_value;
    return(enahnced_s);
}

//------------------------------------------------------------------------------
SkillSpec apply_sabotage(const SkillSpec& s, unsigned sabotaged_value)
{
    SkillSpec sabotaged_s = s;
    sabotaged_s.x -= std::min(sabotaged_s.x, sabotaged_value);
    return(sabotaged_s);
}

//------------------------------------------------------------------------------
inline void resolve_scavenge(Storage<CardStatus>& store)
{
    for(auto status : store.m_indirect)
    {
        if(!is_alive(status))continue;
        unsigned scavenge_value = status->skill(Skill::scavenge);
        if(!scavenge_value)continue;

        _DEBUG_MSG(1, "%s activates Scavenge %u\n",
                status_description(status).c_str(), scavenge_value);
        status->ext_hp(scavenge_value);
    }
}
//------------------------------------------------------------------------------
/**
 * @brief Handle death of a card
 * 
 * @param fd Field pointer 
 * @param paybacked Is the death caused by payback?
 */
void prepend_on_death(Field* fd, bool paybacked=false)
{
    if (fd->killed_units.empty())
        return;
    bool skip_all_except_summon = fd->fixes[Fix::death_from_bge] && (fd->current_phase == Field::bge_phase);
    if (skip_all_except_summon)
    {
        _DEBUG_MSG(2, "Death from BGE Fix (skip all death depended triggers except summon)\n");
    }
    auto& assaults = fd->players[fd->killed_units[0]->m_player]->assaults;
    unsigned stacked_poison_value = 0;
    unsigned last_index = 99999;
    CardStatus* left_virulence_victim = nullptr;
    for (auto status: fd->killed_units)
    {
        // Skill: Scavenge
        // Any unit dies => perm-hp-buff
        if (__builtin_expect(!skip_all_except_summon, true))
        {
            resolve_scavenge(fd->players[0]->assaults);
            resolve_scavenge(fd->players[1]->assaults);
            resolve_scavenge(fd->players[0]->structures);
            resolve_scavenge(fd->players[1]->structures);
        }

        if ((status->m_card->m_type == CardType::assault) && (!skip_all_except_summon))
        {
            // Skill: Avenge
            const unsigned host_idx = status->m_index;
            unsigned from_idx, till_idx;
            //scan all assaults for Avenge
            from_idx = 0;
            till_idx = assaults.size() - 1;
            for (; from_idx <= till_idx; ++ from_idx)
            {
                if (from_idx == host_idx) { continue; }
                CardStatus* adj_status = &assaults[from_idx];
                if (!is_alive(adj_status)) { continue; }
                unsigned avenge_value = adj_status->skill(Skill::avenge);
                if (!avenge_value) { continue; }

                // Use half value rounded up
                // (for distance > 1, i. e. non-standard Avenge triggering)
                if (std::abs((signed)from_idx - (signed)host_idx) > 1)
                {
                    avenge_value = (avenge_value + 1) / 2;
                }
                _DEBUG_MSG(1, "%s activates %sAvenge %u\n",
                        status_description(adj_status).c_str(),
                        (std::abs((signed)from_idx - (signed)host_idx) > 1 ? "Half-" : ""),
                         avenge_value);
                if (!adj_status->m_sundered)
                { adj_status->m_perm_attack_buff += avenge_value; }
                adj_status->ext_hp(avenge_value);
            }

            // Passive BGE: Virulence
            if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::virulence], false))
            {
                if (status->m_index != last_index + 1)
                {
                    stacked_poison_value = 0;
                    left_virulence_victim = nullptr;
                    if (status->m_index > 0)
                    {
                        auto left_status = &assaults[status->m_index - 1];
                        if (is_alive(left_status))
                        {
                            left_virulence_victim = left_status;
                        }
                    }
                }
                if (status->m_poisoned > 0)
                {
                    if (left_virulence_victim != nullptr)
                    {
                        _DEBUG_MSG(1, "Virulence: %s spreads left poison +%u to %s\n",
                                status_description(status).c_str(), status->m_poisoned,
                                status_description(left_virulence_victim).c_str());
                        left_virulence_victim->m_poisoned += status->m_poisoned;
                    }
                    stacked_poison_value += status->m_poisoned;
                    _DEBUG_MSG(1, "Virulence: %s spreads right poison +%u = %u\n",
                            status_description(status).c_str(), status->m_poisoned, stacked_poison_value);
                }
                if (status->m_index + 1 < assaults.size())
                {
                    auto right_status = &assaults[status->m_index + 1];
                    if (is_alive(right_status))
                    {
                        _DEBUG_MSG(1, "Virulence: spreads stacked poison +%u to %s\n",
                                stacked_poison_value, status_description(right_status).c_str());
                        right_status->m_poisoned += stacked_poison_value;
                    }
                }
                last_index = status->m_index;
            }
        }

        // Passive BGE: Revenge
        // Fix::death_from_bge: should not affect passive BGE, keep it as was before
        if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::revenge], false))
        {
            if (fd->bg_effects[fd->tapi][PassiveBGE::revenge] < 0)
                throw std::runtime_error("BGE Revenge: value must be defined & positive");
            SkillSpec ss_heal{Skill::heal, (unsigned)fd->bg_effects[fd->tapi][PassiveBGE::revenge], allfactions, 0, 0, Skill::no_skill, Skill::no_skill, true, 0,};
            SkillSpec ss_rally{Skill::rally, (unsigned)fd->bg_effects[fd->tapi][PassiveBGE::revenge], allfactions, 0, 0, Skill::no_skill, Skill::no_skill, true, 0,};
            CardStatus* commander = &fd->players[status->m_player]->commander;
            _DEBUG_MSG(2, "Revenge: Preparing (head) skills  %s and %s\n",
                    skill_description(fd->cards, ss_heal).c_str(),
                    skill_description(fd->cards, ss_rally).c_str());
            fd->skill_queue.emplace(fd->skill_queue.begin()+0, commander, ss_heal);
            fd->skill_queue.emplace(fd->skill_queue.begin()+1, commander, ss_rally); // +1: keep ss_heal at first place
        }

        // resolve On-Death skills
        for (auto& ss: status->m_card->m_skills_on_death)
        {
            if (__builtin_expect(skip_all_except_summon && (ss.id != Skill::summon), false))
            { continue; }
        	SkillSpec tss = ss;
            _DEBUG_MSG(2, "On Death %s: Preparing (tail) skill %s\n",
                    status_description(status).c_str(), skill_description(fd->cards, ss).c_str());
            if(fd->fixes[Fix::revenge_on_death] && is_activation_harmful_skill(ss.id) && paybacked)
            {
            	_DEBUG_MSG(2, "On Death Revenge Fix\n");
            	tss.s2 = Skill::revenge;
            }
            fd->skill_queue.emplace_back(status, tss);
        }
    }
    fd->killed_units.clear();
}

//------------------------------------------------------------------------------
void(*skill_table[Skill::num_skills])(Field*, CardStatus* src, const SkillSpec&);
void resolve_skill(Field* fd)
{
    while (!fd->skill_queue.empty())
    {
        auto skill_instance(fd->skill_queue.front());
        auto& status(std::get<0>(skill_instance));
        const auto& ss(std::get<1>(skill_instance));
        fd->skill_queue.pop_front();
        if (__builtin_expect(status->m_card->m_skill_trigger[ss.id] == Skill::Trigger::activate, true))
        {
            if (!is_alive(status))
            {
                _DEBUG_MSG(2, "%s failed to %s because it is dead.\n",
                        status_description(status).c_str(), skill_description(fd->cards, ss).c_str());
                continue;
            }
            if (status->m_jammed)
            {
                _DEBUG_MSG(2, "%s failed to %s because it is Jammed.\n",
                        status_description(status).c_str(), skill_description(fd->cards, ss).c_str());
                continue;
            }
        }

        // is summon? (non-activation skill)
        if (ss.id == Skill::summon)
        {
            check_and_perform_summon(fd, status);
            continue;
        }
        _DEBUG_ASSERT(is_activation_skill(ss.id) || ss.id == Skill::enhance); // enhance is no trigger, but  queues the skill

        SkillSpec modified_s = ss;

        // apply evolve
        signed evolved_offset = status->m_evolved_skill_offset[modified_s.id];
        if (evolved_offset != 0)
        { modified_s = apply_evolve(modified_s, evolved_offset); }

        // apply sabotage (only for X-based activation skills)
        unsigned sabotaged_value = status->m_sabotaged;
        if ((sabotaged_value > 0) && is_activation_skill_with_x(modified_s.id))
        { modified_s = apply_sabotage(modified_s, sabotaged_value); }

        // apply enhance
        unsigned enhanced_value = status->enhanced(modified_s.id);
        if (enhanced_value > 0)
        { modified_s = apply_enhance(modified_s, enhanced_value); }

        // perform skill (if it is still applicable)
        if (is_activation_skill_with_x(modified_s.id) && !modified_s.x)
        {
            _DEBUG_MSG(2, "%s failed to %s because its X value is zeroed (sabotaged).\n",
                    status_description(status).c_str(), skill_description(fd->cards, ss).c_str());
            continue;
        }
        else { skill_table[modified_s.id](fd, status, modified_s); }
    }
}

void apply_corrosion(CardStatus * status)
{
      if (status->m_corroded_rate)
      {
        unsigned v = std::min(status->m_corroded_rate, status->attack_power());
        unsigned corrosion = std::min(v, status->m_card->m_attack
                + status->m_perm_attack_buff - status->m_corroded_weakened);
        _DEBUG_MSG(1, "%s loses Attack by %u (+corrosion %u).\n", status_description(status).c_str(), v, corrosion);
        status->m_corroded_weakened += corrosion;
      }
}
void remove_corrosion(CardStatus * status)
{
       if (status->m_corroded_rate)
       {
           _DEBUG_MSG(1, "%s loses Status corroded.\n", status_description(status).c_str());
           status->m_corroded_rate = 0;
           status->m_corroded_weakened = 0;
       }
}
//------------------------------------------------------------------------------
bool attack_phase(Field* fd);

template<Skill::Skill skill_id>
inline bool check_and_perform_skill(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s, bool is_evadable
#ifndef NQUEST
        , bool & has_counted_quest
#endif
        );

    template <enum CardType::CardType type>
void evaluate_skills(Field* fd, CardStatus* status, const std::vector<SkillSpec>& skills, bool* attacked=nullptr)
{
    _DEBUG_ASSERT(status);
    unsigned num_actions(1);
    for (unsigned action_index(0); action_index < num_actions; ++ action_index)
    {
        status->m_action_index = action_index;
        fd->prepare_action();
        _DEBUG_ASSERT(fd->skill_queue.size() == 0);
        for (auto & ss: skills)
        {
            if (!is_activation_skill(ss.id)) { continue; }
            if (status->m_skill_cd[ss.id] > 0) { continue; }
            _DEBUG_MSG(2, "Evaluating %s skill %s\n",
                    status_description(status).c_str(), skill_description(fd->cards, ss).c_str());
            fd->skill_queue.emplace_back(status, ss);
            resolve_skill(fd);
        }
        if (type == CardType::assault)
        {
            // Attack
            if (can_act(status))
            {
                if (attack_phase(fd))
                {
                    *attacked = true;
                    if (__builtin_expect(fd->end, false)) { break; }
                    //Apply corrosion
                    apply_corrosion(status);
                }
                else
                {
                  // Remove Corrosion
                  remove_corrosion(status);
                }
            }
            else
            {
                _DEBUG_MSG(2, "%s cannot take attack.\n", status_description(status).c_str());
                // Remove Corrosion
                remove_corrosion(status);
            }
        }
        fd->finalize_action();
        // Flurry
        if (can_act(status) && status->has_skill(Skill::flurry) && (status->m_skill_cd[Skill::flurry] == 0))
        {
#ifndef NQUEST
            if (status->m_player == 0)
            {
                fd->inc_counter(QuestType::skill_use, Skill::flurry);
            }
#endif
            _DEBUG_MSG(1, "%s activates Flurry x %d\n",
                    status_description(status).c_str(), status->skill_base_value(Skill::flurry));
            num_actions += status->skill_base_value(Skill::flurry);
            for (const auto & ss : skills)
            {
                Skill::Skill evolved_skill_id = static_cast<Skill::Skill>(ss.id + status->m_evolved_skill_offset[ss.id]);
                if (evolved_skill_id == Skill::flurry)
                {
                    status->m_skill_cd[ss.id] = ss.c;
                }
            }
        }
    }
}

struct PlayCard
{
    const Card* card;
    Field* fd;
    CardStatus* status;
    Storage<CardStatus>* storage;
    const unsigned actor_index;
    const CardStatus* actor_status;

    PlayCard(const Card* card_, Field* fd_, unsigned ai_, CardStatus* as_) :
        card{card_},
        fd{fd_},
        status{nullptr},
        storage{nullptr},
        actor_index{ai_},
        actor_status{as_}
    {}

    template <enum CardType::CardType type>
        CardStatus* op()
        {
            return op<type>(false);
        }

    template <enum CardType::CardType type>
        CardStatus* op(bool summoned)
        {
            setStorage<type>();
            placeCard<type>();
            status->m_summoned = summoned;

            unsigned played_faction_mask(0);
            unsigned same_faction_cards_count(0);
            bool bge_megamorphosis = fd->bg_effects[status->m_player][PassiveBGE::megamorphosis];
            //played_status = status;
            //played_card = card;

            check_and_perform_bravery(fd,status);
            if (status->m_delay == 0)
            {
                check_and_perform_valor(fd, status);
            }

            //refresh/init absorb
            if(status->has_skill(Skill::absorb))
            {
                status->m_absorption = status->skill_base_value(Skill::absorb);
            }


            // 1. Evaluate skill Allegiance & count assaults with same faction (structures will be counted later)
            // 2. Passive BGE Cold Sleep
            for (CardStatus* status_i : fd->players[status->m_player]->assaults.m_indirect)
            {
                if (status_i == status || !is_alive(status_i)) { continue; } // except itself
                //std::cout << status_description(status_i).c_str();
                _DEBUG_ASSERT(is_alive(status_i));
                if (bge_megamorphosis || (status_i->m_card->m_faction == card->m_faction))
                {
                    ++ same_faction_cards_count;
                    unsigned allegiance_value = status_i->skill(Skill::allegiance);
                    if (__builtin_expect(allegiance_value, false) && !status->m_summoned)
                    {
                        _DEBUG_MSG(1, "%s activates Allegiance %u\n", status_description(status_i).c_str(), allegiance_value);
                        if (! status_i->m_sundered)
                        { status_i->m_perm_attack_buff += allegiance_value; }
                        status_i->ext_hp(allegiance_value);
                    }
                }
                if (__builtin_expect(fd->bg_effects[status->m_player][PassiveBGE::coldsleep], false)
                        && status_i->m_protected_stasis && can_be_healed(status_i))
                {
                    unsigned bge_value = (status_i->m_protected_stasis + 1) / 2;
                    _DEBUG_MSG(1, "Cold Sleep: %s heals itself for %u\n", status_description(status_i).c_str(), bge_value);
                    status_i->add_hp(bge_value);
                }
            }

            // Setup faction marks (bitmap) for stasis (skill Stasis / Passive BGE TemporalBacklash)
            // unless Passive BGE Megamorphosis is enabled
            if (__builtin_expect(!bge_megamorphosis, true))
            {
                played_faction_mask = (1u << card->m_faction);
                // do played card have stasis? mark this faction for stasis check
                if (__builtin_expect(status->skill(Skill::stasis), false)
                        || __builtin_expect(fd->bg_effects[status->m_player][PassiveBGE::temporalbacklash] && status->skill(Skill::counter), false))
                {
                    fd->players[status->m_player]->stasis_faction_bitmap |= played_faction_mask;
                }
            }

            // Evaluate Passive BGE Oath-of-Loyalty
            unsigned allegiance_value;
            if (__builtin_expect(fd->bg_effects[status->m_player][PassiveBGE::oath_of_loyalty], false)
                    && ((allegiance_value = status->skill(Skill::allegiance)) > 0))
            {
                // count structures with same faction (except fortresses, dominions and other non-normal structures)
                for (CardStatus * status_i : fd->players[status->m_player]->structures.m_indirect)
                {
                    if ((status_i->m_card->m_category == CardCategory::normal)
                            && (bge_megamorphosis || (status_i->m_card->m_faction == card->m_faction)))
                    {
                        ++ same_faction_cards_count;
                    }
                }

                // apply Passive BGE Oath-of-Loyalty when multiplier isn't zero
                if (same_faction_cards_count)
                {
                    unsigned bge_value = allegiance_value * same_faction_cards_count;
                    _DEBUG_MSG(1, "Oath of Loyalty: %s activates Allegiance %u x %u = %u\n",
                            status_description(status).c_str(), allegiance_value, same_faction_cards_count, bge_value);
                    status->m_perm_attack_buff += bge_value;
                    status->ext_hp(bge_value);
                }
            }

            // summarize stasis when:
            //  1. Passive BGE Megamorphosis is enabled
            //  2. current faction is marked for it
            if ((card->m_delay > 0) && (card->m_type == CardType::assault)
                    && __builtin_expect(bge_megamorphosis || (fd->players[status->m_player]->stasis_faction_bitmap & played_faction_mask), false))
            {
                unsigned stacked_stasis = (bge_megamorphosis || (fd->players[status->m_player]->commander.m_card->m_faction == card->m_faction))
                    ? fd->players[status->m_player]->commander.skill(Skill::stasis)
                    : 0u;
#ifndef NDEBUG
                if (stacked_stasis > 0)
                {
                    _DEBUG_MSG(2, "+ Stasis [%s]: stacks +%u stasis protection from %s (total stacked: %u)\n",
                            faction_names[card->m_faction].c_str(), stacked_stasis,
                            status_description(&fd->players[status->m_player]->commander).c_str(), stacked_stasis);
                }
#endif
                for (CardStatus * status_i : fd->players[status->m_player]->structures.m_indirect)
                {
                    if ((bge_megamorphosis || (status_i->m_card->m_faction == card->m_faction)) && is_alive(status_i))
                    {
                        stacked_stasis += status_i->skill(Skill::stasis);
#ifndef NDEBUG
                        if (status_i->skill(Skill::stasis) > 0)
                        {
                            _DEBUG_MSG(2, "+ Stasis [%s]: stacks +%u stasis protection from %s (total stacked: %u)\n",
                                    faction_names[card->m_faction].c_str(), status_i->skill(Skill::stasis),
                                    status_description(status_i).c_str(), stacked_stasis);
                        }
#endif
                    }
                }
                for (CardStatus * status_i : fd->players[status->m_player]->assaults.m_indirect)
                {
                    if ((bge_megamorphosis || (status_i->m_card->m_faction == card->m_faction)) && is_alive(status_i))
                    {
                        stacked_stasis += status_i->skill(Skill::stasis);
#ifndef NDEBUG
                        if (status_i->skill(Skill::stasis) > 0)
                        {
                            _DEBUG_MSG(2, "+ Stasis [%s]: stacks +%u stasis protection from %s (total stacked: %u)\n",
                                    faction_names[card->m_faction].c_str(), status_i->skill(Skill::stasis),
                                    status_description(status_i).c_str(), stacked_stasis);
                        }
#endif
                        if (__builtin_expect(fd->bg_effects[status->m_player][PassiveBGE::temporalbacklash] && status_i->skill(Skill::counter), false))
                        {
                            stacked_stasis += (status_i->skill(Skill::counter) + 1) / 2;
#ifndef NDEBUG
                            _DEBUG_MSG(2, "Temporal Backlash: + Stasis [%s]: stacks +%u stasis protection from %s (total stacked: %u)\n",
                                    faction_names[card->m_faction].c_str(), (status_i->skill(Skill::counter) + 1) / 2,
                                    status_description(status_i).c_str(), stacked_stasis);
#endif
                        }
                    }
                }
                status->m_protected_stasis = stacked_stasis;
#ifndef NDEBUG
                if (stacked_stasis > 0)
                {
                    _DEBUG_MSG(1, "%s gains %u stasis protection\n",
                            status_description(status).c_str(), stacked_stasis);
                }
#endif
                // no more stasis for current faction: do unmark (if no Passive BGE Megamorphosis)
                if (__builtin_expect(!bge_megamorphosis, true) && __builtin_expect(!stacked_stasis, false))
                {
                    fd->players[status->m_player]->stasis_faction_bitmap &= ~played_faction_mask;
                    _DEBUG_MSG(1, "- Stasis [%s]: no more units with stasis from %s\n",
                            faction_names[card->m_faction].c_str(),status_description(&fd->players[status->m_player]->commander).c_str());
                }

            }
            //Devotion BGE
            if (__builtin_expect(fd->bg_effects[status->m_player][PassiveBGE::devotion], false) && !summoned && card->m_category == CardCategory::normal && fd->players[status->m_player]->commander.m_card->m_faction == card->m_faction)
            {
                _DEBUG_MSG(1, "Devotion %s: Gains %u HP\n",
                        status_description(status).c_str(), (2*card->m_health+4)/5);
                status->ext_hp((2*card->m_health+4)/5); //40% bonus health (rounded up)
            }


            // resolve On-Play skills
            // Fix Death on BGE: [On Play] skills during BGE phase can be invoked only by means of [On Death] trigger
            if (__builtin_expect(fd->fixes[Fix::death_from_bge] && (fd->current_phase != Field::bge_phase), true))
            {
                for (const auto& ss: card->m_skills_on_play)
                {
                    _DEBUG_MSG(2, "On Play %s: Preparing (tail) skill %s\n",
                            status_description(status).c_str(), skill_description(fd->cards, ss).c_str());
                    fd->skill_queue.emplace_back(status, ss);
                }
            }
            else
            {
                _DEBUG_MSG(2, "Death from BGE Fix: suppress [On Play] skills invoked by [On Death] summon\n");
            }

            return status;
        }

    template <enum CardType::CardType>
        void setStorage()
        {
        }

    template <enum CardType::CardType type>
        void placeCard()
        {
            status = &storage->add_back();
            status->set(card);
            status->m_index = storage->size() - 1;
            status->m_player = actor_index;
#ifndef NQUEST
            if (actor_index == 0)
            {
                if (card->m_type == CardType::assault)
                {
                    fd->inc_counter(QuestType::faction_assault_card_use, card->m_faction);
                }
                fd->inc_counter(QuestType::type_card_use, type);
            }
#endif
            _DEBUG_MSG(1, "%s plays %s %u [%s]\n",
                    status_description(actor_status).c_str(), cardtype_names[type].c_str(),
                    static_cast<unsigned>(storage->size() - 1), card_description(fd->cards, card).c_str());
        }
};
// assault
    template <>
void PlayCard::setStorage<CardType::assault>()
{
    storage = &fd->players[actor_index]->assaults;
}
// structure
    template <>
void PlayCard::setStorage<CardType::structure>()
{
    storage = &fd->players[actor_index]->structures;
}

// Check if a skill actually proc'ed.
    template<Skill::Skill skill_id>
inline bool skill_check(Field* fd, CardStatus* c, CardStatus* ref)
{
    return is_defensive_skill(skill_id) || is_alive(c);
}

    template<>
inline bool skill_check<Skill::heal>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return can_be_healed(c);
}

    template<>
inline bool skill_check<Skill::mend>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return can_be_healed(c);
}

    template<>
inline bool skill_check<Skill::rally>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return !c->m_sundered;
}

    template<>
inline bool skill_check<Skill::overload>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return is_active(c) && !c->m_overloaded && !has_attacked(c);
}

    template<>
inline bool skill_check<Skill::jam>(Field* fd, CardStatus* c, CardStatus* ref)
{
    if(__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::ironwill], false) && c->has_skill(Skill::armor))return false;
    // active player performs Jam
    if (fd->tapi == ref->m_player)
    { return is_active_next_turn(c); }


    // inactive player performs Jam
    return will_activate_this_turn(c);
}

    template<>
inline bool skill_check<Skill::leech>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return can_be_healed(c) || (fd->fixes[Fix::leech_increase_max_hp] && is_alive(c));
}

    template<>
inline bool skill_check<Skill::coalition>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return is_active(c);
}

    template<>
inline bool skill_check<Skill::payback>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return (ref->m_card->m_type == CardType::assault);
}

    template<>
inline bool skill_check<Skill::revenge>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return skill_check<Skill::payback>(fd, c, ref);
}

    template<>
inline bool skill_check<Skill::tribute>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return (ref->m_card->m_type == CardType::assault) && (c != ref);
}

    template<>
inline bool skill_check<Skill::refresh>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return can_be_healed(c);
}

    template<>
inline bool skill_check<Skill::drain>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return can_be_healed(c);
}

    template<>
inline bool skill_check<Skill::mark>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return (ref->m_card->m_type == CardType::assault);
}

    template<>
inline bool skill_check<Skill::disease>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return is_alive(c) && (ref->m_card->m_type == CardType::assault);
}
    template<>
inline bool skill_check<Skill::inhibit>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return is_alive(c) && (ref->m_card->m_type == CardType::assault);
}
    template<>
inline bool skill_check<Skill::sabotage>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return is_alive(c) && (ref->m_card->m_type == CardType::assault);
}
inline unsigned remove_disease(CardStatus* status, unsigned heal)
{
    unsigned remaining_heal(heal);
    if(__builtin_expect(status->m_diseased == 0,true))
    {
        //skip
    }
    else if (heal > status->m_diseased)
    {
        _DEBUG_MSG(1, "%s disease-blocked %u heal\n", status_description(status).c_str(), status->m_diseased);
        remaining_heal = heal - status->m_diseased;
        status->m_diseased = 0;
    }
    else
    {
        _DEBUG_MSG(1, "%s disease-blocked %u heal\n", status_description(status).c_str(), heal);
        status->m_diseased -= heal;
        remaining_heal = 0;
    }
    return remaining_heal;
}

// Field is currently not needed for remove_absorption, but is here for similiar structure as remove_hp
inline unsigned remove_absorption(Field* fd, CardStatus* status, unsigned dmg)
{
    return remove_absorption(status,dmg);
}
/**
 * @brief Remove absorption from a CardStatus
 * 
 * @param status CardStatus to remove absorption from
 * @param dmg damage to remove absorption from
 * @return unsigned remaining damage after absorption is removed
 */
inline unsigned remove_absorption(CardStatus* status, unsigned dmg)
{
    unsigned remaining_dmg(dmg);
    if(__builtin_expect(status->m_absorption == 0,true))
    {
        //skip
    }
    else if(dmg > status->m_absorption)
    {
        _DEBUG_MSG(1, "%s absorbs %u damage\n", status_description(status).c_str(), status->m_absorption);
        remaining_dmg = dmg - status->m_absorption;
        status->m_absorption = 0;
    }
    else
    {
        _DEBUG_MSG(1, "%s absorbs %u damage\n", status_description(status).c_str(), dmg);
        status->m_absorption -= dmg;
        remaining_dmg = 0;
    }
    return remaining_dmg;
}

void remove_hp(Field* fd, CardStatus* status, unsigned dmg)
{
    if (__builtin_expect(!dmg, false)) { return; }
    _DEBUG_ASSERT(is_alive(status));
    _DEBUG_MSG(2, "%s takes %u damage\n", status_description(status).c_str(), dmg);
    status->m_hp = safe_minus(status->m_hp, dmg);
    if (fd->current_phase < Field::end_phase && status->has_skill(Skill::barrier))
    {
        ++fd->damaged_units_to_times[status];
        _DEBUG_MSG(2, "%s damaged %u times\n",
                status_description(status).c_str(), fd->damaged_units_to_times[status]);
    }
    if (status->m_hp == 0)
    {
#ifndef NQUEST
        if (status->m_player == 1)
        {
            if (status->m_card->m_type == CardType::assault)
            {
                fd->inc_counter(QuestType::faction_assault_card_kill, status->m_card->m_faction);
            }
            fd->inc_counter(QuestType::type_card_kill, status->m_card->m_type);
        }
#endif
        _DEBUG_MSG(1, "%s dies\n", status_description(status).c_str());
        _DEBUG_ASSERT(status->m_card->m_type != CardType::commander);
        fd->killed_units.push_back(status);
        ++fd->players[status->m_player]->total_cards_destroyed;
        if(!status->m_summoned)++fd->players[status->m_player]->total_nonsummon_cards_destroyed;
        if (__builtin_expect((status->m_player == 0) && (fd->players[0]->deck->vip_cards.count(status->m_card->m_id)), false))
        {
            fd->players[0]->commander.m_hp = 0;
            fd->end = true;
        }
    }
}

inline bool is_it_dead(CardStatus& c)
{
    if (c.m_hp == 0) // yes it is
    {
        _DEBUG_MSG(1, "Dead and removed: %s\n", status_description(&c).c_str());
        return true;
    }
    return false; // nope still kickin'
}

inline bool is_it_dominion(CardStatus* c)
{
    return (c->m_card->m_category == CardCategory::dominion_alpha);
}

inline void remove_dead(Storage<CardStatus>& storage)
{
    storage.remove(is_it_dead);
}

void cooldown_skills(CardStatus * status)
{
    for (const auto & ss : status->m_card->m_skills)
    {
        if (status->m_skill_cd[ss.id] > 0)
        {
            _DEBUG_MSG(2, "%s reduces timer (%u) of skill %s\n",
                    status_description(status).c_str(), status->m_skill_cd[ss.id], skill_names[ss.id].c_str());
            -- status->m_skill_cd[ss.id];
        }
    }
}
/**
 * Handle:
 * Absorb, (Activation)Summon, Bravery, (Initial)Valor, Inhibit, Sabotage, Disease, Enhance, (Cooldown/Every X) Reductions
 **/
void turn_start_phase_update(Field*fd,CardStatus * status)
{
            //apply Absorb + Triggered\{Valor} Enhances
            check_and_perform_early_enhance(fd,status);
            if(fd->fixes[Fix::enhance_early])check_and_perform_later_enhance(fd,status);
            //refresh absorb
            if(status->has_skill(Skill::absorb))
            {
                status->m_absorption = status->skill_base_value(Skill::absorb);
            }
            check_and_perform_bravery(fd,status);
            if(__builtin_expect(fd->fixes[Fix::barrier_each_turn],true)){
                status->m_protected += status->skill(Skill::barrier);
            }
            if (status->m_delay > 0)
            {
                _DEBUG_MSG(1, "%s reduces its timer\n", status_description(status).c_str());
                --status->m_delay;
                if (status->m_delay == 0)
                {
                    check_and_perform_valor(fd, status);
                    if(status->m_card->m_skill_trigger[Skill::summon] == Skill::Trigger::activate)check_and_perform_summon(fd, status);
                }
            }
            else
            {
                cooldown_skills(status);
            }
}

void turn_start_phase(Field* fd)
{
    // Active player's commander card:
    cooldown_skills(&fd->tap->commander);
    //grab assaults before new ones get summoned
    auto& assaults(fd->tap->assaults);
    unsigned end(assaults.size());

    //Perform early enhance for commander
    check_and_perform_early_enhance(fd,&fd->tap->commander);
    if(fd->fixes[Fix::enhance_early])check_and_perform_later_enhance(fd,&fd->tap->commander);

    // Active player's structure cards:
    // update index
    // reduce delay; reduce skill cooldown
    {
        auto& structures(fd->tap->structures);
        for(unsigned index(0); index < structures.size(); ++index)
        {
            CardStatus * status = &structures[index];
            status->m_index = index;
            turn_start_phase_update(fd,status);
        }
    }
    // Active player's assault cards:
    // update index
    // reduce delay; reduce skill cooldown
    {
        for(unsigned index(0); index < end; ++index)
        {
            CardStatus * status = &assaults[index];
            status->m_index = index;
            turn_start_phase_update(fd,status);
        }
    }
    // Defending player's structure cards:
    // update index
    {
        auto& structures(fd->tip->structures);
        for(unsigned index(0), end(structures.size()); index < end; ++index)
        {
            CardStatus& status(structures[index]);
            status.m_index = index;
        }
    }
    // Defending player's assault cards:
    // update index
    {
        auto& assaults(fd->tip->assaults);
        for(unsigned index(0), end(assaults.size()); index < end; ++index)
        {
            CardStatus& status(assaults[index]);
            status.m_index = index;
        }
    }
}

void turn_end_phase(Field* fd)
{
    // Inactive player's assault cards:
    {
        auto& assaults(fd->tip->assaults);
        for(unsigned index(0), end(assaults.size()); index < end; ++ index)
        {
            CardStatus& status(assaults[index]);
            if (status.m_hp <= 0)
            {
                continue;
            }
            status.m_enfeebled = 0;
            status.m_protected = 0;
            std::memset(status.m_primary_skill_offset, 0, sizeof status.m_primary_skill_offset);
            std::memset(status.m_evolved_skill_offset, 0, sizeof status.m_evolved_skill_offset);
            std::memset(status.m_enhanced_value, 0, sizeof status.m_enhanced_value);
            status.m_evaded = 0;  // so far only useful in Inactive turn
            status.m_paybacked = 0;  // ditto
            status.m_entrapped = 0;
        }
    }
    // Inactive player's structure cards:
    {
        auto& structures(fd->tip->structures);
        for(unsigned index(0), end(structures.size()); index < end; ++ index)
        {
            CardStatus& status(structures[index]);
            if (status.m_hp <= 0)
            {
                continue;
            }
            status.m_evaded = 0;  // so far only useful in Inactive turn
        }
    }

    // Active player's assault cards:
    {
        auto& assaults(fd->tap->assaults);
        for(unsigned index(0), end(assaults.size()); index < end; ++ index)
        {
            CardStatus& status(assaults[index]);
            if (status.m_hp <= 0)
            {
                continue;
            }
            unsigned refresh_value = status.skill(Skill::refresh) + (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::crackdown],false)?(status.skill(Skill::subdue)+1)/2:0); //BGE: crackdown refresh+=subdue/2

            if (refresh_value && skill_check<Skill::refresh>(fd, &status, nullptr))
            {
                _DEBUG_MSG(1, "%s refreshes %u health\n", status_description(&status).c_str(), refresh_value);
                status.add_hp(refresh_value);
            }
            if (status.m_poisoned > 0)
            {
                unsigned poison_dmg = remove_absorption(fd,&status,status.m_poisoned + status.m_enfeebled);
                poison_dmg = safe_minus(poison_dmg, status.protected_value());
                if (poison_dmg > 0)
                {
#ifndef NQUEST
                    if (status.m_player == 1)
                    {
                        fd->inc_counter(QuestType::skill_damage, Skill::poison, 0, poison_dmg);
                    }
#endif
                    _DEBUG_MSG(1, "%s takes poison damage %u\n", status_description(&status).c_str(), poison_dmg);
                    remove_hp(fd, &status, poison_dmg);  // simultaneous
                }
            }
            // end of the opponent's next turn for enemy units
            status.m_temp_attack_buff = 0;
            status.m_jammed = false;
            status.m_enraged = 0;
            status.m_sundered = false;
            status.m_inhibited = 0;
            status.m_sabotaged = 0;
            status.m_tributed = 0;
            status.m_overloaded = false;
            status.m_step = CardStep::none;
        }
    }
    // Active player's structure cards:
    // nothing so far

    prepend_on_death(fd);  // poison
    resolve_skill(fd);
    remove_dead(fd->tap->assaults);
    remove_dead(fd->tap->structures);
    remove_dead(fd->tip->assaults);
    remove_dead(fd->tip->structures);
}

//---------------------- $50 attack by assault card implementation -------------
/**
 * @brief Return the damage dealt to the attacker (att) by defender (def) through counter skill
 * The counter is increased by the attacker's enfeebled value, while the attacker's protected value is subtracted from the damage.
 * The absorption is removed from the damage before the protected value is subtracted.
 * 
 * @param fd Field pointer 
 * @param att attacker CardStatus
 * @param def defender CardStatus
 * @return unsigned 
 */
inline unsigned counter_damage(Field* fd, CardStatus* att, CardStatus* def)
{
    _DEBUG_ASSERT(att->m_card->m_type == CardType::assault);
    _DEBUG_ASSERT(def->skill(Skill::counter) > 0); // counter skill must be present otherwise enfeeble is wrongly applied

    return safe_minus(remove_absorption(fd,att,def->skill(Skill::counter) + att->m_enfeebled), att->protected_value());
}

inline CardStatus* select_first_enemy_wall(Field* fd)
{
    for(unsigned i(0); i < fd->tip->structures.size(); ++i)
    {
        CardStatus* c(&fd->tip->structures[i]);
        if (c->has_skill(Skill::wall) && is_alive(c)) { return c; }
    }
    return nullptr;
}

inline CardStatus* select_first_enemy_assault(Field* fd)
{
    for(unsigned i(0); i < fd->tip->assaults.size(); ++i)
    {
        CardStatus* c(&fd->tip->assaults[i]);
        if (is_alive(c)) { return c; }
    }
    return nullptr;
}


inline bool alive_assault(Storage<CardStatus>& assaults, unsigned index)
{
    return(assaults.size() > index && is_alive(&assaults[index]));
}

void remove_commander_hp(Field* fd, CardStatus& status, unsigned dmg)
{
    _DEBUG_ASSERT(is_alive(&status));
    _DEBUG_ASSERT(status.m_card->m_type == CardType::commander);
    _DEBUG_MSG(2, "%s takes %u damage\n", status_description(&status).c_str(), dmg);
    status.m_hp = safe_minus(status.m_hp, dmg);
    if (status.m_hp == 0)
    {
        _DEBUG_MSG(1, "%s dies\n", status_description(&status).c_str());
        fd->end = true;
    }
}

//------------------------------------------------------------------------------
// implementation of one attack by an assault card, against either an enemy
// assault card, the first enemy wall, or the enemy commander.
struct PerformAttack
{
    Field* fd;
    CardStatus* att_status;
    CardStatus* def_status;
    unsigned att_dmg;

    PerformAttack(Field* fd_, CardStatus* att_status_, CardStatus* def_status_) :
        fd(fd_), att_status(att_status_), def_status(def_status_), att_dmg(0)
    {}

    template<enum CardType::CardType def_cardtype>
        unsigned op()
        {
            unsigned pre_modifier_dmg = att_status->attack_power();

            // Evaluation order:
            // check flying
            // modify damage
            // deal damage
            // assaults only: (poison,inihibit, sabotage)
            // on-attacked skills
            // counter, berserk
            // assaults only: (leech if still alive)
            // bge
            // subdue

            if(__builtin_expect(def_status->has_skill(Skill::flying),false) && fd->flip()) {
                _DEBUG_MSG(1, "%s flies away from %s\n",
                        status_description(def_status).c_str(),
                        status_description(att_status).c_str());
                return 0;
            }

            modify_attack_damage<def_cardtype>(pre_modifier_dmg);
            if (!att_dmg) { on_attacked<def_cardtype>();return 0; }

            attack_damage<def_cardtype>();

            if (__builtin_expect(fd->end, false)) { return att_dmg; }
            damage_dependant_pre_oa<def_cardtype>();

            on_attacked<def_cardtype>();
            if (!is_alive(att_status)) { return att_dmg; }


            // Enemy Skill: Counter
            if (def_status->has_skill(Skill::counter))
            {
                // perform_skill_counter
                unsigned counter_dmg(counter_damage(fd, att_status, def_status));
#ifndef NQUEST
                if (def_status->m_player == 0)
                {
                    fd->inc_counter(QuestType::skill_use, Skill::counter);
                    fd->inc_counter(QuestType::skill_damage, Skill::counter, 0, counter_dmg);
                }
#endif
                _DEBUG_MSG(1, "%s takes %u counter damage from %s\n",
                        status_description(att_status).c_str(), counter_dmg,
                        status_description(def_status).c_str());
                remove_hp(fd, att_status, counter_dmg);
                prepend_on_death(fd);
                resolve_skill(fd);

                // Passive BGE: Counterflux
                if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::counterflux], false)
                        && (def_cardtype == CardType::assault) && is_alive(def_status))
                {
                    unsigned flux_denominator = (fd->bg_effects[fd->tapi][PassiveBGE::counterflux] > 0)
                        ? (unsigned)fd->bg_effects[fd->tapi][PassiveBGE::counterflux]
                        : 4;
                    unsigned flux_value = (def_status->skill(Skill::counter) - 1) / flux_denominator + 1;
                    _DEBUG_MSG(1, "Counterflux: %s heals itself and berserks for %u\n",
                            status_description(def_status).c_str(), flux_value);
                    def_status->add_hp(flux_value);
                    if (! def_status->m_sundered)
                    { def_status->m_perm_attack_buff += flux_value; }
                }

                // is attacker dead?
                if (!is_alive(att_status)) { return att_dmg; }
            }

            // Skill: Corrosive
            unsigned corrosive_value = def_status->skill(Skill::corrosive);
            if (corrosive_value > att_status->m_corroded_rate)
            {
                // perform_skill_corrosive
                _DEBUG_MSG(1, "%s corrodes %s by %u\n",
                        status_description(def_status).c_str(),
                        status_description(att_status).c_str(), corrosive_value);
                att_status->m_corroded_rate = corrosive_value;
            }

            // Skill: Berserk
            unsigned berserk_value = att_status->skill(Skill::berserk);
            if (!att_status->m_sundered && (berserk_value > 0))
            {
                // perform_skill_berserk
                att_status->m_perm_attack_buff += berserk_value;
#ifndef NQUEST
                if (att_status->m_player == 0)
                {
                    fd->inc_counter(QuestType::skill_use, Skill::berserk);
                }
#endif

                // Passive BGE: EnduringRage
                if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::enduringrage], false))
                {
                    unsigned bge_denominator = (fd->bg_effects[fd->tapi][PassiveBGE::enduringrage] > 0)
                        ? (unsigned)fd->bg_effects[fd->tapi][PassiveBGE::enduringrage]
                        : 2;
                    unsigned bge_value = (berserk_value - 1) / bge_denominator + 1;
                    _DEBUG_MSG(1, "EnduringRage: %s heals and protects itself for %u\n",
                            status_description(att_status).c_str(), bge_value);
                    att_status->add_hp(bge_value);
                    att_status->m_protected += bge_value;
                }
            }

            // Skill: Leech
            do_leech<def_cardtype>();

            // Passive BGE: Heroism
            unsigned valor_value;
            if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::heroism], false)
                    && ((valor_value = att_status->skill(Skill::valor) + att_status->skill(Skill::bravery)) > 0)
                    && !att_status->m_sundered
                    && (def_cardtype == CardType::assault) && (def_status->m_hp <= 0))
            {
                _DEBUG_MSG(1, "Heroism: %s gain %u attack\n",
                        status_description(att_status).c_str(), valor_value);
                att_status->m_perm_attack_buff += valor_value;
            }

            // Passive BGE: Devour
            unsigned leech_value;
            if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::devour], false)
                    && ((leech_value = att_status->skill(Skill::leech) + att_status->skill(Skill::refresh)) > 0)
                    && (def_cardtype == CardType::assault))
            {
                unsigned bge_denominator = (fd->bg_effects[fd->tapi][PassiveBGE::devour] > 0)
                    ? (unsigned)fd->bg_effects[fd->tapi][PassiveBGE::devour]
                    : 4;
                unsigned bge_value = (leech_value - 1) / bge_denominator + 1;
                if (! att_status->m_sundered)
                {
                    _DEBUG_MSG(1, "Devour: %s gains %u attack\n",
                            status_description(att_status).c_str(), bge_value);
                    att_status->m_perm_attack_buff += bge_value;
                }
                _DEBUG_MSG(1, "Devour: %s extends max hp / heals itself for %u\n",
                        status_description(att_status).c_str(), bge_value);
                att_status->ext_hp(bge_value);
            }

            // Skill: Subdue
            unsigned subdue_value = def_status->skill(Skill::subdue);
            if (__builtin_expect(subdue_value, false))
            {
                _DEBUG_MSG(1, "%s subdues %s by %u\n",
                        status_description(def_status).c_str(),
                        status_description(att_status).c_str(), subdue_value);
                att_status->m_subdued += subdue_value;
                //fix negative attack
                if(att_status->calc_attack_power()<0)
                {
                    att_status->m_temp_attack_buff -= att_status->calc_attack_power();
                }
                if (att_status->m_hp > att_status->max_hp())
                {
                    _DEBUG_MSG(1, "%s loses %u HP due to subdue (max hp: %u)\n",
                            status_description(att_status).c_str(),
                            (att_status->m_hp - att_status->max_hp()),
                            att_status->max_hp());
                    att_status->m_hp = att_status->max_hp();
                }
            }

            return att_dmg;
        }

    template<enum CardType::CardType>
        void modify_attack_damage(unsigned pre_modifier_dmg)
        {
            _DEBUG_ASSERT(att_status->m_card->m_type == CardType::assault);
            att_dmg = pre_modifier_dmg;
            if (att_dmg == 0)
            { return; }
#ifndef NDEBUG
            std::string desc;
#endif
            auto& att_assaults = fd->tap->assaults; // (active) attacker assaults
            auto& def_assaults = fd->tip->assaults; // (inactive) defender assaults
            unsigned legion_value = 0;
            unsigned coalition_value = 0;

            // Enhance damage (if additional damage isn't prevented)
            if (! att_status->m_sundered)
            {
                // Skill: Mark
                unsigned marked_value = def_status->m_marked;
                if(marked_value)
                {
#ifndef NDEBUG
                    if (debug_print > 0) { desc += "+" + tuo::to_string(marked_value) + "(mark)"; }
#endif
                    att_dmg += marked_value;
                }
                // Skill: Legion
                unsigned legion_base = att_status->skill(Skill::legion);
                if (__builtin_expect(legion_base, false))
                {
                    unsigned itr_idx, till_idx;
                    bool bge_megamorphosis = fd->bg_effects[fd->tapi][PassiveBGE::megamorphosis] && !fd->fixes[Fix::legion_under_mega];
                    //scan all assaults for Global Legion
                    itr_idx = 0;
                    till_idx = att_assaults.size() - 1;
                    for (; itr_idx <= till_idx; ++ itr_idx)
                    {
                        if(itr_idx == att_status->m_index)continue; //legion doesn't count itself, unlike coalition
                        legion_value += is_alive(&att_assaults[itr_idx])
                          && (bge_megamorphosis || (att_assaults[itr_idx].m_card->m_faction == att_status->m_card->m_faction));
                    }
                    if (legion_value)
                    {
                        legion_value *= legion_base;
#ifndef NDEBUG
                        if (debug_print > 0) { desc += "+" + tuo::to_string(legion_value) + "(legion)"; }
#endif
                        att_dmg += legion_value;
                    }
                }

                // Skill: Coalition
                unsigned coalition_base = att_status->skill(Skill::coalition);
                if (__builtin_expect(coalition_base, false))
                {
                    uint8_t factions_bitmap = 0;
                    for (CardStatus * status : att_assaults.m_indirect)
                    {
                        if (! is_alive(status)) { continue; }
                        factions_bitmap |= (1 << (status->m_card->m_faction));
                    }
                    _DEBUG_ASSERT(factions_bitmap);
                    unsigned uniq_factions = byte_bits_count(factions_bitmap);
                    coalition_value = coalition_base * uniq_factions;
#ifndef NDEBUG
                    if (debug_print > 0) { desc += "+" + tuo::to_string(coalition_value) + "(coalition/x" + tuo::to_string(uniq_factions) + ")"; }
#endif
                    att_dmg += coalition_value;
                }

                // Skill: Rupture
                unsigned rupture_value = att_status->skill(Skill::rupture);
                if (rupture_value > 0)
                {
#ifndef NDEBUG
                    if (debug_print > 0) { desc += "+" + tuo::to_string(rupture_value) + "(rupture)"; }
#endif
                    att_dmg += rupture_value;
                }

                // Skill: Venom
                unsigned venom_value = att_status->skill(Skill::venom);
                if (venom_value > 0 && def_status->m_poisoned > 0)
                {
#ifndef NDEBUG
                    if (debug_print > 0) { desc += "+" + tuo::to_string(venom_value) + "(venom)"; }
#endif
                    att_dmg += venom_value;
                }

                // Passive BGE: Bloodlust
                if (fd->bloodlust_value > 0)
                {
#ifndef NDEBUG
                    if (debug_print > 0) { desc += "+" + tuo::to_string(fd->bloodlust_value) + "(bloodlust)"; }
#endif
                    att_dmg += fd->bloodlust_value;
                }

                // State: Enfeebled
                if (def_status->m_enfeebled > 0)
                {
#ifndef NDEBUG
                    if (debug_print > 0) { desc += "+" + tuo::to_string(def_status->m_enfeebled) + "(enfeebled)"; }
#endif
                    att_dmg += def_status->m_enfeebled;
                }
            }
            // prevent damage
#ifndef NDEBUG
            std::string reduced_desc;
#endif
            unsigned reduced_dmg(0);
            unsigned armor_value = 0;
            // Armor
            if (def_status->m_card->m_type == CardType::assault) {
                // Passive BGE: Fortification (adj step -> 1 (1 left, host, 1 right)
                unsigned adj_size = (unsigned)(fd->bg_effects[fd->tapi][PassiveBGE::fortification]);
                unsigned host_idx = def_status->m_index;
                unsigned from_idx = safe_minus(host_idx, adj_size);
                unsigned till_idx = std::min(host_idx + adj_size, safe_minus(def_assaults.size(), 1));
                for (; from_idx <= till_idx; ++ from_idx)
                {
                    CardStatus* adj_status = &def_assaults[from_idx];
                    if (!is_alive(adj_status)) { continue; }
                    armor_value = std::max(armor_value, adj_status->skill(Skill::armor));
                }
            }
            if (armor_value > 0)
            {
#ifndef NDEBUG
                if(debug_print > 0) { reduced_desc += tuo::to_string(armor_value) + "(armor)"; }
#endif
                reduced_dmg += armor_value;
            }
            if (def_status->protected_value() > 0)
            {
#ifndef NDEBUG
                if(debug_print > 0) { reduced_desc += (reduced_desc.empty() ? "" : "+") + tuo::to_string(def_status->protected_value()) + "(protected)"; }
#endif
                reduced_dmg += def_status->protected_value();
            }
            unsigned pierce_value = att_status->skill(Skill::pierce) + att_status->skill(Skill::rupture);
            if (reduced_dmg > 0 && pierce_value > 0)
            {
#ifndef NDEBUG
                if (debug_print > 0) { reduced_desc += "-" + tuo::to_string(pierce_value) + "(pierce)"; }
#endif
                reduced_dmg = safe_minus(reduced_dmg, pierce_value);
            }
            att_dmg = safe_minus(att_dmg, reduced_dmg);
#ifndef NDEBUG
            if (debug_print > 0)
            {
                if(!reduced_desc.empty()) { desc += "-[" + reduced_desc + "]"; }
                if(!desc.empty()) { desc += "=" + tuo::to_string(att_dmg); }
                _DEBUG_MSG(1, "%s attacks %s for %u%s damage\n",
                        status_description(att_status).c_str(),
                        status_description(def_status).c_str(), pre_modifier_dmg, desc.c_str());
            }
#endif
            // Passive BGE: Brigade
            if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::brigade] && legion_value, false)
                    && can_be_healed(att_status))
            {
                _DEBUG_MSG(1, "Brigade: %s heals itself for %u\n",
                        status_description(att_status).c_str(), legion_value);
                att_status->add_hp(legion_value);
            }

            // Passive BGE: Unity
            if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::unity] && coalition_value, false)
                    && can_be_healed(att_status))
            {
                _DEBUG_MSG(1, "Unity: %s heals itself for %u\n",
                        status_description(att_status).c_str(), (coalition_value + 1)/2);
                att_status->add_hp((coalition_value + 1)/2);
            }

            // Increase Mark-counter
            unsigned mark_base = att_status->skill(Skill::mark);
            if(mark_base && skill_check<Skill::mark>(fd,att_status,def_status)) {
                _DEBUG_MSG(1, "%s marks %s for %u\n",
                        status_description(att_status).c_str(), status_description(def_status).c_str(), mark_base);
                def_status->m_marked += mark_base;
            }

        }

    template<enum CardType::CardType>
        void attack_damage()
        {

            remove_hp(fd, def_status, att_dmg);
            prepend_on_death(fd);
            resolve_skill(fd);
        }

    template<enum CardType::CardType>
        void on_attacked() {
            //APN
            // resolve On-Attacked skills
            for (const auto& ss: def_status->m_card->m_skills_on_attacked)
            {
                _DEBUG_MSG(1, "On Attacked %s: Preparing (tail) skill %s\n",
                        status_description(def_status).c_str(), skill_description(fd->cards, ss).c_str());
                fd->skill_queue.emplace_back(def_status, ss);
                resolve_skill(fd);
            }
        }

    template<enum CardType::CardType>
        void damage_dependant_pre_oa() {}

    template<enum CardType::CardType>
        void do_leech() {}
};

    template<>
void PerformAttack::attack_damage<CardType::commander>()
{
    remove_commander_hp(fd, *def_status, att_dmg);
}

    template<>
void PerformAttack::damage_dependant_pre_oa<CardType::assault>()
{
    unsigned poison_value = std::max(att_status->skill(Skill::poison), att_status->skill(Skill::venom));
    if (poison_value > def_status->m_poisoned && skill_check<Skill::poison>(fd, att_status, def_status))
    {
        // perform_skill_poison
#ifndef NQUEST
        if (att_status->m_player == 0)
        {
            fd->inc_counter(QuestType::skill_use, Skill::poison);
        }
#endif
        _DEBUG_MSG(1, "%s poisons %s by %u\n",
                status_description(att_status).c_str(),
                status_description(def_status).c_str(), poison_value);
        def_status->m_poisoned = poison_value;
    }

    // Damage-Dependent skill: Inhibit
    /*
    unsigned inhibit_value = att_status->skill(Skill::inhibit);
    if (inhibit_value > def_status->m_inhibited && skill_check<Skill::inhibit>(fd, att_status, def_status))
    {
        _DEBUG_MSG(1, "%s inhibits %s by %u\n",
                status_description(att_status).c_str(),
                status_description(def_status).c_str(), inhibit_value);
        def_status->m_inhibited = inhibit_value;
    }
    */

    // Damage-Dependent skill: Sabotage
    /*
    unsigned sabotage_value = att_status->skill(Skill::sabotage);
    if (sabotage_value > def_status->m_sabotaged && skill_check<Skill::sabotage>(fd, att_status, def_status))
    {
        _DEBUG_MSG(1, "%s sabotages %s by %u\n",
                status_description(att_status).c_str(),
                status_description(def_status).c_str(), sabotage_value);
        def_status->m_sabotaged = sabotage_value;
    }
    */
    // Damage-Dependent skill: Increase Disease-counter
    /*
    unsigned disease_base = att_status->skill(Skill::disease);
    if(disease_base && skill_check<Skill::disease>(fd, att_status, def_status)) {
        _DEBUG_MSG(1, "%s diseases %s for %u\n",
        status_description(att_status).c_str(), status_description(def_status).c_str(), disease_base);
        def_status->m_diseased += disease_base;
    }
    */
}

    template<>
void PerformAttack::do_leech<CardType::assault>()
{
    unsigned leech_value = std::min(att_dmg, att_status->skill(Skill::leech));
    if(leech_value > 0 && skill_check<Skill::leech>(fd, att_status, nullptr))
    {
#ifndef NQUEST
        if (att_status->m_player == 0)
        {
            fd->inc_counter(QuestType::skill_use, Skill::leech);
        }
#endif
        _DEBUG_MSG(1, "%s leeches %u health\n", status_description(att_status).c_str(), leech_value);
        if (__builtin_expect(fd->fixes[Fix::leech_increase_max_hp],true)) {
            att_status->ext_hp(leech_value);
        }
        else {
            att_status->add_hp(leech_value);
        }
    }
}

// General attack phase by the currently evaluated assault, taking into accounts exotic stuff such as flurry, etc.
unsigned attack_commander(Field* fd, CardStatus* att_status)
{
    CardStatus* def_status{select_first_enemy_wall(fd)}; // defending wall
    if (def_status != nullptr)
    {
        return PerformAttack{fd, att_status, def_status}.op<CardType::structure>();
    }
    else
    {
        return PerformAttack{fd, att_status, &fd->tip->commander}.op<CardType::commander>();
    }
}
// Return true if actually attacks
bool attack_phase(Field* fd)
{
    CardStatus* att_status(&fd->tap->assaults[fd->current_ci]); // attacking card
    Storage<CardStatus>& def_assaults(fd->tip->assaults);

    if (!att_status->attack_power())
    {
        _DEBUG_MSG(1, "%s cannot take attack (zeroed)\n", status_description(att_status).c_str());
        return false;
    }

    unsigned att_dmg = 0;
    if (alive_assault(def_assaults, fd->current_ci))
    {
        CardStatus* def_status = &def_assaults[fd->current_ci];
        att_dmg = PerformAttack{fd, att_status, def_status}.op<CardType::assault>();
        // perform swipe/drain
        unsigned swipe_value = att_status->skill(Skill::swipe);
        unsigned drain_value = att_status->skill(Skill::drain);
        if (swipe_value || drain_value)
        {
            bool critical_reach = fd->bg_effects[fd->tapi][PassiveBGE::criticalreach];
            auto drain_total_dmg = att_dmg;
            unsigned adj_size = 1 + (unsigned)(critical_reach);
            unsigned host_idx = def_status->m_index;
            unsigned from_idx = safe_minus(host_idx, adj_size);
            unsigned till_idx = std::min(host_idx + adj_size, safe_minus(def_assaults.size(), 1));
            for (; from_idx <= till_idx; ++ from_idx)
            {
                if (from_idx == host_idx) { continue; }
                CardStatus* adj_status = &def_assaults[from_idx];
                if (!is_alive(adj_status)) { continue; }
                //unsigned swipe_dmg = safe_minus(
                //    swipe_value + drain_value + def_status->m_enfeebled,
                //    def_status->protected_value());
                unsigned remaining_dmg = remove_absorption(fd,adj_status,swipe_value + drain_value + adj_status->m_enfeebled);
                remaining_dmg = safe_minus(remaining_dmg,adj_status->protected_value());
                _DEBUG_MSG(1, "%s swipes %s for %u damage\n",
                        status_description(att_status).c_str(),
                        status_description(adj_status).c_str(), remaining_dmg);

                remove_hp(fd, adj_status, remaining_dmg);
                drain_total_dmg += remaining_dmg;
            }
            if (drain_value && skill_check<Skill::drain>(fd, att_status, nullptr))
            {
                _DEBUG_MSG(1, "%s drains %u hp\n",
                        status_description(att_status).c_str(), drain_total_dmg);
                att_status->add_hp(drain_total_dmg);
            }
            prepend_on_death(fd);
            resolve_skill(fd);
        }
        // perform hunt
        unsigned hunt_value = att_status->skill(Skill::hunt);
        if(hunt_value)
        {
            CardStatus* hunted_status{select_first_enemy_assault(fd)};
            if (hunted_status != nullptr)
            {
                unsigned remaining_dmg = remove_absorption(fd,hunted_status,hunt_value + hunted_status->m_enfeebled);
                remaining_dmg = safe_minus(remaining_dmg,hunted_status->protected_value());
                _DEBUG_MSG(1, "%s hunts %s for %u damage\n",
                        status_description(att_status).c_str(),
                        status_description(hunted_status).c_str(), remaining_dmg);

                remove_hp(fd, hunted_status, remaining_dmg);

                prepend_on_death(fd);
                resolve_skill(fd);
            }
        }
    }
    else
    {
        // might be blocked by walls
        att_dmg = attack_commander(fd, att_status);
    }

    // Passive BGE: Bloodlust
    if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::bloodlust], false)
            && !fd->assault_bloodlusted && (att_dmg > 0))
    {
        fd->bloodlust_value += fd->bg_effects[fd->tapi][PassiveBGE::bloodlust];
        fd->assault_bloodlusted = true;
    }

    return true;
}

//---------------------- $65 active skills implementation ----------------------
template<
bool C
, typename T1
, typename T2
>
struct if_
{
    typedef T1 type;
};

template<
typename T1
, typename T2
>
struct if_<false,T1,T2>
{
    typedef T2 type;
};

    template<unsigned skill_id>
inline bool skill_predicate(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{ return skill_check<static_cast<Skill::Skill>(skill_id)>(fd, dst, src); }

    template<>
inline bool skill_predicate<Skill::enhance>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    if (!is_alive(dst)) return false;
    if (!dst->has_skill(s.s)) return false;
    if (is_active(dst)) return true;
    if (is_defensive_skill(s.s)) return true;
    if (is_instant_debuff_skill(s.s)) return true; // Enhance Sabotage, Inhibit, Disease also without dst being active
    if (is_triggered_skill(s.s) && s.s != Skill::valor) return true;// Enhance Allegiance, Stasis, Bravery ( + not in TU: Flurry, Summon; No enhance on inactive dst: Valor)

    /* Strange Transmission [Gilians]: strange gillian's behavior implementation:
     * The Gillian commander and assaults can enhance any skills on any assaults
     * regardless of jammed/delayed states. But what kind of behavior is in the case
     * when gilians are played among standard assaults, I don't know. :)
     */
    return is_alive_gilian(src);
}

    template<>
inline bool skill_predicate<Skill::evolve>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    if (!is_alive(dst)) return false;
    if (!dst->has_skill(s.s)) return false;
    if (dst->has_skill(s.s2)) return false;
    if (is_active(dst)) return true;
    if (is_defensive_skill(s.s2)) return true;

    /* Strange Transmission [Gilians]: strange gillian's behavior implementation:
     * The Gillian commander and assaults can enhance any skills on any assaults
     * regardless of jammed/delayed states. But what kind of behavior is in the case
     * when gilians are played among standard assaults, I don't know. :)
     */
    return is_alive_gilian(src);
}

    template<>
inline bool skill_predicate<Skill::mimic>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    // skip dead units
    if (!is_alive(dst)) return false;

    //include on activate/attacked/death
    //for(const auto & a  : {dst->m_card->m_skills,dst->m_card->m_skills_on_play,dst->m_card->m_skills_on_death,dst->m_card->m_skills_on_attacked})
    //
    std::vector<std::vector<SkillSpec>> all;
    all.emplace_back(dst->m_card->m_skills);
    all.emplace_back(dst->m_card->m_skills_on_attacked);

    for(std::vector<SkillSpec> & a  : all)
    {
    // scan all enemy skills until first activation
    for (const SkillSpec & ss: a)
    {
        // get skill
        Skill::Skill skill_id = static_cast<Skill::Skill>(ss.id);

        // skip non-activation skills and Mimic (Mimic can't be mimicked)
        if (!is_activation_skill(skill_id) || (skill_id == Skill::mimic))
        { continue; }

        // skip mend for non-assault mimickers
        if ((skill_id == Skill::mend || skill_id == Skill::fortify) && (src->m_card->m_type != CardType::assault))
        { continue; }

        // enemy has at least one activation skill that can be mimicked, so enemy is eligible target for Mimic
        return true;
    }
    }

    // found nothing (enemy has no skills to be mimicked, so enemy isn't eligible target for Mimic)
    return false;
}

    template<>
inline bool skill_predicate<Skill::overload>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    // basic skill check
    if (!skill_check<Skill::overload>(fd, dst, src))
    { return false; }

    // check skills
    bool inhibited_searched = false;
    for (const auto& ss: dst->m_card->m_skills)
    {
        // skip cooldown skills
        if (dst->m_skill_cd[ss.id] > 0)
        { continue; }

        // get evolved skill
        Skill::Skill evolved_skill_id = static_cast<Skill::Skill>(ss.id + dst->m_evolved_skill_offset[ss.id]);

        // unit with an activation hostile skill is always valid target for OL
        if (is_activation_hostile_skill(evolved_skill_id))
        { return true; }

        // unit with an activation helpful skill is valid target only when there are inhibited units
        // TODO check mend/fortify valid overload target?!?
        if ((evolved_skill_id != Skill::mend && evolved_skill_id != Skill::fortify)
                && is_activation_helpful_skill(evolved_skill_id)
                && __builtin_expect(!inhibited_searched, true))
        {
            for (const auto & c: fd->players[dst->m_player]->assaults.m_indirect)
            {
                if (is_alive(c) && c->m_inhibited)
                { return true; }
            }
            inhibited_searched = true;
        }
    }
    return false;
}

    template<>
inline bool skill_predicate<Skill::rally>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    return skill_check<Skill::rally>(fd, dst, src) // basic skill check
        && (__builtin_expect((fd->tapi == dst->m_player), true) // is target on the active side?
                ? is_active(dst) && !has_attacked(dst) // normal case
                : is_active_next_turn(dst) // diverted case / on-death activation
           )
        ;
}

    template<>
inline bool skill_predicate<Skill::enrage>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    return (__builtin_expect((fd->tapi == dst->m_player), true) // is target on the active side?
            ? is_active(dst) && (dst->m_step == CardStep::none) // normal case
            : is_active_next_turn(dst) // on-death activation
           )
        && (dst->attack_power()) // card can perform direct attack
        ;
}

    template<>
inline bool skill_predicate<Skill::rush>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    return (!src->m_rush_attempted)
        && (dst->m_delay >= ((src->m_card->m_type == CardType::assault) && (dst->m_index < src->m_index) ? 2u : 1u));
}

    template<>
inline bool skill_predicate<Skill::weaken>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    if(__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::ironwill], false) && dst->has_skill(Skill::armor))return false;
    if (!dst->attack_power()) { return false; }

    // active player performs Weaken (normal case)
    if (__builtin_expect((fd->tapi == src->m_player), true))
    { return is_active_next_turn(dst); }

    // APN - On-Attacked/Death don't target the attacking card

    // inactive player performs Weaken (inverted case (on-death activation))
    return will_activate_this_turn(dst);
}

    template<>
inline bool skill_predicate<Skill::sunder>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    return skill_predicate<Skill::weaken>(fd, src, dst, s);
}

    template<unsigned skill_id>
inline void perform_skill(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{ assert(false); }

    template<>
inline void perform_skill<Skill::enfeeble>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_enfeebled += s.x;
}

    template<>
inline void perform_skill<Skill::enhance>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_enhanced_value[s.s + dst->m_primary_skill_offset[s.s]] += s.x;
}

    template<>
inline void perform_skill<Skill::evolve>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    auto primary_s1 = dst->m_primary_skill_offset[s.s] + s.s;
    auto primary_s2 = dst->m_primary_skill_offset[s.s2] + s.s2;
    dst->m_primary_skill_offset[s.s] = primary_s2 - s.s;
    dst->m_primary_skill_offset[s.s2] = primary_s1 - s.s2;
    dst->m_evolved_skill_offset[primary_s1] = s.s2 - primary_s1;
    dst->m_evolved_skill_offset[primary_s2] = s.s - primary_s2;
}

    template<>
inline void perform_skill<Skill::heal>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->add_hp(s.x);

    // Passive BGE: ZealotsPreservation
    if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::zealotspreservation], false)
            && (src->m_card->m_type == CardType::assault))
    {
        unsigned bge_value = (s.x + 1) / 2;
        _DEBUG_MSG(1, "Zealot's Preservation: %s Protect %u on %s\n",
                status_description(src).c_str(), bge_value,
                status_description(dst).c_str());
        dst->m_protected += bge_value;
    }
}

    template<>
inline void perform_skill<Skill::jam>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_jammed = true;
}

    template<>
inline void perform_skill<Skill::mend>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->add_hp(s.x);
}

    template<>
inline void perform_skill<Skill::fortify>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->ext_hp(s.x);
}
    template<>
inline void perform_skill<Skill::mortar>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    if (dst->m_card->m_type == CardType::structure)
    {
        remove_hp(fd, dst, remove_absorption(fd,dst,s.x));
    }
    else
    {
        unsigned strike_dmg = remove_absorption(fd,dst,(s.x + 1) / 2 + dst->m_enfeebled);
        strike_dmg = safe_minus(strike_dmg, src->m_overloaded ? 0 : dst->protected_value());
        remove_hp(fd, dst, strike_dmg);
    }
}

    template<>
inline void perform_skill<Skill::overload>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_overloaded = true;
}

    template<>
inline void perform_skill<Skill::protect>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_protected += s.x;
}

    template<>
inline void perform_skill<Skill::rally>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_temp_attack_buff += s.x;
}

    template<>
inline void perform_skill<Skill::enrage>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_enraged += s.x;
    // Passive BGE: Furiosity
    if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::furiosity], false)
            && can_be_healed(dst))
    {
        unsigned bge_value = s.x;
        _DEBUG_MSG(1, "Furiosity: %s Heals %s for %u\n",
                status_description(src).c_str(),
                status_description(dst).c_str(), bge_value);
        dst->add_hp(bge_value);
    }
}

    template<>
inline void perform_skill<Skill::entrap>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_entrapped += s.x;
}

    template<>
inline void perform_skill<Skill::rush>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_delay -= std::min(std::max(s.x, 1u), dst->m_delay);
    if (dst->m_delay == 0)
    {
        check_and_perform_valor(fd, dst);
        if(dst->m_card->m_skill_trigger[Skill::summon] == Skill::Trigger::activate)check_and_perform_summon(fd, dst);
    }
}

    template<>
inline void perform_skill<Skill::siege>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    remove_hp(fd, dst, remove_absorption(fd,dst,s.x));
}

    template<>
inline void perform_skill<Skill::strike>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    unsigned strike_dmg = remove_absorption(fd,dst,s.x+ dst->m_enfeebled);
    strike_dmg = safe_minus(strike_dmg , src->m_overloaded ? 0 : dst->protected_value());
    remove_hp(fd, dst, strike_dmg);
}

    template<>
inline void perform_skill<Skill::weaken>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_temp_attack_buff -= (unsigned)std::min(s.x, dst->attack_power());
}

    template<>
inline void perform_skill<Skill::sunder>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_sundered = true;
    perform_skill<Skill::weaken>(fd, src, dst, s);
}

    template<>
inline void perform_skill<Skill::mimic>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    // collect all mimickable enemy skills
    std::vector<const SkillSpec *> mimickable_skills;
    mimickable_skills.reserve(dst->m_card->m_skills.size()+dst->m_card->m_skills_on_play.size()+dst->m_card->m_skills_on_death.size()+dst->m_card->m_skills_on_attacked.size());
    _DEBUG_MSG(2, " * Mimickable skills of %s\n", status_description(dst).c_str());
    //include on activate/attacked
    std::vector<std::vector<SkillSpec>> all;
    all.emplace_back(dst->m_card->m_skills);
    all.emplace_back(dst->m_card->m_skills_on_attacked);
    for(std::vector<SkillSpec> & a : all)
    {
    for (const SkillSpec & ss: a)
    {
        // get skill
        Skill::Skill skill_id = static_cast<Skill::Skill>(ss.id);

        // skip non-activation skills and Mimic (Mimic can't be mimicked)
        if (!is_activation_skill(skill_id) || (skill_id == Skill::mimic))
        { continue; }

        // skip mend for non-assault mimickers
        if ((skill_id == Skill::mend || skill_id == Skill::fortify) && (src->m_card->m_type != CardType::assault))
        { continue; }

        mimickable_skills.emplace_back(&ss);
        _DEBUG_MSG(2, "  + %s\n", skill_description(fd->cards, ss).c_str());
    }
    }

    // select skill
    unsigned mim_idx = 0;
    switch (mimickable_skills.size())
    {
        case 0: assert(false); break;
        case 1: break;
        default: mim_idx = (fd->re() % mimickable_skills.size()); break;
    }
    // prepare & perform selected skill
    const SkillSpec & mim_ss = *mimickable_skills[mim_idx];
    Skill::Skill mim_skill_id = static_cast<Skill::Skill>(mim_ss.id);
    auto skill_value = s.x + src->enhanced(mim_skill_id); //enhanced skill from mimic ?!?
    SkillSpec mimicked_ss{mim_skill_id, skill_value, allfactions, mim_ss.n, 0, mim_ss.s, mim_ss.s2, mim_ss.all, mim_ss.card_id,};
    _DEBUG_MSG(1, " * Mimicked skill: %s\n", skill_description(fd->cards, mimicked_ss).c_str());
    skill_table[mim_skill_id](fd, src, mimicked_ss);
}

    template<unsigned skill_id>
inline unsigned select_fast(Field* fd, CardStatus* src, const std::vector<CardStatus*>& cards, const SkillSpec& s)
{
    if ((s.y == allfactions)
            || fd->bg_effects[fd->tapi][PassiveBGE::metamorphosis]
            || fd->bg_effects[fd->tapi][PassiveBGE::megamorphosis])
    {
        auto pred = [fd, src, s](CardStatus* c) {
            return(skill_predicate<skill_id>(fd, src, c, s));
        };
        return fd->make_selection_array(cards.begin(), cards.end(), pred);
    }
    else
    {
        auto pred = [fd, src, s](CardStatus* c) {
            return ((c->m_card->m_faction == s.y || c->m_card->m_faction == progenitor) && skill_predicate<skill_id>(fd, src, c, s));
        };
        return fd->make_selection_array(cards.begin(), cards.end(), pred);
    }
}

    template<>
inline unsigned select_fast<Skill::mend>(Field* fd, CardStatus* src, const std::vector<CardStatus*>& cards, const SkillSpec& s)
{
    fd->selection_array.clear();
    bool critical_reach = fd->bg_effects[fd->tapi][PassiveBGE::criticalreach];
    auto& assaults = fd->players[src->m_player]->assaults;
    unsigned adj_size = 1 + (unsigned)(critical_reach);
    unsigned host_idx = src->m_index;
    unsigned from_idx = safe_minus(host_idx, adj_size);
    unsigned till_idx = std::min(host_idx + adj_size, safe_minus(assaults.size(), 1));
    for (; from_idx <= till_idx; ++ from_idx)
    {
        if (from_idx == host_idx) { continue; }
        CardStatus* adj_status = &assaults[from_idx];
        if (!is_alive(adj_status)) { continue; }
        if (skill_predicate<Skill::mend>(fd, src, adj_status, s))
        {
            fd->selection_array.push_back(adj_status);
        }
    }
    return fd->selection_array.size();
}

    template<>
inline unsigned select_fast<Skill::fortify>(Field* fd, CardStatus* src, const std::vector<CardStatus*>& cards, const SkillSpec& s)
{
    fd->selection_array.clear();
    bool critical_reach = fd->bg_effects[fd->tapi][PassiveBGE::criticalreach];
    auto& assaults = fd->players[src->m_player]->assaults;
    unsigned adj_size = 1 + (unsigned)(critical_reach);
    unsigned host_idx = src->m_index;
    unsigned from_idx = safe_minus(host_idx, adj_size);
    unsigned till_idx = std::min(host_idx + adj_size, safe_minus(assaults.size(), 1));
    for (; from_idx <= till_idx; ++ from_idx)
    {
        if (from_idx == host_idx) { continue; }
        CardStatus* adj_status = &assaults[from_idx];
        if (!is_alive(adj_status)) { continue; }
        if (skill_predicate<Skill::fortify>(fd, src, adj_status, s))
        {
            fd->selection_array.push_back(adj_status);
        }
    }
    return fd->selection_array.size();
}
inline std::vector<CardStatus*>& skill_targets_hostile_assault(Field* fd, CardStatus* src)
{
    return(fd->players[opponent(src->m_player)]->assaults.m_indirect);
}

inline std::vector<CardStatus*>& skill_targets_allied_assault(Field* fd, CardStatus* src)
{
    return(fd->players[src->m_player]->assaults.m_indirect);
}

inline std::vector<CardStatus*>& skill_targets_hostile_structure(Field* fd, CardStatus* src)
{
    return(fd->players[opponent(src->m_player)]->structures.m_indirect);
}

inline std::vector<CardStatus*>& skill_targets_allied_structure(Field* fd, CardStatus* src)
{
    return(fd->players[src->m_player]->structures.m_indirect);
}

    template<unsigned skill>
std::vector<CardStatus*>& skill_targets(Field* fd, CardStatus* src)
{
    std::cerr << "skill_targets: Error: no specialization for " << skill_names[skill] << "\n";
    throw;
}

template<> std::vector<CardStatus*>& skill_targets<Skill::enfeeble>(Field* fd, CardStatus* src)
{ return(skill_targets_hostile_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::enhance>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::evolve>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::heal>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::jam>(Field* fd, CardStatus* src)
{ return(skill_targets_hostile_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::mend>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::fortify>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::overload>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::protect>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::rally>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::enrage>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::entrap>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::rush>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::siege>(Field* fd, CardStatus* src)
{ return(skill_targets_hostile_structure(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::strike>(Field* fd, CardStatus* src)
{ return(skill_targets_hostile_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::sunder>(Field* fd, CardStatus* src)
{ return(skill_targets_hostile_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::weaken>(Field* fd, CardStatus* src)
{ return(skill_targets_hostile_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<Skill::mimic>(Field* fd, CardStatus* src)
{ return(skill_targets_hostile_assault(fd, src)); }

    template<Skill::Skill skill_id>
inline bool check_and_perform_skill(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s, bool is_evadable
#ifndef NQUEST
        , bool & has_counted_quest
#endif
        )
{
    if (__builtin_expect(skill_check<skill_id>(fd, dst, src), true))
    {
#ifndef NQUEST
        if (src->m_player == 0 && ! has_counted_quest)
        {
            fd->inc_counter(QuestType::skill_use, skill_id, dst->m_card->m_id);
            has_counted_quest = true;
        }
#endif
        if (is_evadable && (dst->m_evaded < dst->skill(Skill::evade)))
        {
            ++ dst->m_evaded;
            _DEBUG_MSG(1, "%s %s on %s but it evades\n",
                    status_description(src).c_str(), skill_short_description(fd->cards, s).c_str(),
                    status_description(dst).c_str());
            return(false);
        }
        _DEBUG_MSG(1, "%s %s on %s\n",
                status_description(src).c_str(), skill_short_description(fd->cards, s).c_str(),
                status_description(dst).c_str());
        perform_skill<skill_id>(fd, src, dst, s);
        if (s.c > 0)
        {
            src->m_skill_cd[skill_id] = s.c;
        }
        // Skill: Tribute
        if (skill_check<Skill::tribute>(fd, dst, src)
                // only activation helpful skills can be tributed (* except Evolve, Enhance, and Rush)
                && is_activation_helpful_skill(s.id) && (s.id != Skill::evolve) && (s.id != Skill::enhance) && (s.id != Skill::rush)
                && (dst->m_tributed < dst->skill(Skill::tribute))
                && skill_check<skill_id>(fd, src, src))
        {
            ++ dst->m_tributed;
            _DEBUG_MSG(1, "%s tributes %s back to %s\n",
                    status_description(dst).c_str(), skill_short_description(fd->cards, s).c_str(),
                    status_description(src).c_str());
            perform_skill<skill_id>(fd, src, src, s);
        }
        return(true);
    }
    _DEBUG_MSG(1, "(CANCELLED) %s %s on %s\n",
            status_description(src).c_str(), skill_short_description(fd->cards, s).c_str(),
            status_description(dst).c_str());
    return(false);
}
bool check_and_perform_enhance(Field* fd, CardStatus* src, bool early)
{
      if(!is_active(src))return false; // active
      if(!src->has_skill(Skill::enhance))return false; // enhance Skill
      for(auto ss : src->m_card->m_skills)
      {
          if(ss.id != Skill::enhance)continue;
          if(early ^ (ss.s == Skill::allegiance || ss.s == Skill::absorb ||ss.s == Skill::stasis || ss.s == Skill::bravery))continue; //only specified skills are 'early'
          skill_table[ss.id](fd,src,ss);
      }
      return true;
}
bool check_and_perform_early_enhance(Field* fd, CardStatus* src)
{
      return check_and_perform_enhance(fd,src,true);
}
bool check_and_perform_later_enhance(Field* fd, CardStatus* src)
{
      return check_and_perform_enhance(fd,src,false);
}
bool check_and_perform_valor(Field* fd, CardStatus* src)
{
    unsigned valor_value = src->skill(Skill::valor);
    if (valor_value && !src->m_sundered && skill_check<Skill::valor>(fd, src, nullptr))
    {
        _DEBUG_ASSERT(src->m_card->m_type == CardType::assault); //only assaults
        unsigned opponent_player = opponent(src->m_player);
        const CardStatus * dst = fd->players[opponent_player]->assaults.size() > src->m_index ?
            &fd->players[opponent_player]->assaults[src->m_index] :
            nullptr;
        if (dst == nullptr || dst->m_hp <= 0)
        {
            _DEBUG_MSG(1, "%s loses Valor (no blocker)\n", status_description(src).c_str());
            return false;
        }
        else if (dst->attack_power() <= src->attack_power())
        {
            _DEBUG_MSG(1, "%s loses Valor (weak blocker %s)\n", status_description(src).c_str(), status_description(dst).c_str());
            return false;
        }
#ifndef NQUEST
        if (src->m_player == 0)
        {
            fd->inc_counter(QuestType::skill_use, Skill::valor);
        }
#endif
        _DEBUG_MSG(1, "%s activates Valor %u\n", status_description(src).c_str(), valor_value);
        src->m_perm_attack_buff += valor_value;
        return true;
    }
    return false;
}

bool check_and_perform_bravery(Field* fd, CardStatus* src)
{
    unsigned bravery_value = src->skill(Skill::bravery);
    if (bravery_value && !src->m_sundered && skill_check<Skill::bravery>(fd, src, nullptr))
    {
        _DEBUG_ASSERT(src->m_card->m_type == CardType::assault); //only assaults
        unsigned opponent_player = opponent(src->m_player);
        const CardStatus * dst = fd->players[opponent_player]->assaults.size() > src->m_index ?
            &fd->players[opponent_player]->assaults[src->m_index] :
            nullptr;
        if (dst == nullptr || dst->m_hp <= 0)
        {
            _DEBUG_MSG(1, "%s loses Bravery (no blocker)\n", status_description(src).c_str());
            return false;
        }
        else if (dst->attack_power() <= src->attack_power())
        {
            _DEBUG_MSG(1, "%s loses Bravery (weak blocker %s)\n", status_description(src).c_str(), status_description(dst).c_str());
            return false;
        }
#ifndef NQUEST
        if (src->m_player == 0)
        {
            fd->inc_counter(QuestType::skill_use, Skill::bravery);
        }
#endif
        _DEBUG_MSG(1, "%s activates Bravery %u\n", status_description(src).c_str(), bravery_value);
        src->m_perm_attack_buff += bravery_value;

        //BGE: superheroism
        if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::superheroism], false))
        {
            unsigned bge_value = bravery_value * fd->bg_effects[fd->tapi][PassiveBGE::superheroism];
            const SkillSpec ss_heal{Skill::heal, bge_value, allfactions, 0, 0, Skill::no_skill, Skill::no_skill, true, 0,};
            _DEBUG_MSG(1, "%s activates SuperHeroism: %s\n", status_description(src).c_str(),
                    skill_description(fd->cards, ss_heal).c_str());
            //fd->skill_queue.emplace(fd->skill_queue.begin()+0, src, ss_heal);
            skill_table[Skill::heal](fd,src,ss_heal); //Probably better to perform the skill direct instead of queuing it
        }

        return true;
    }
    return false;
}

bool check_and_perform_inhibit(Field* fd, CardStatus* att_status,CardStatus* def_status)
{
      unsigned inhibit_value = att_status->skill(Skill::inhibit);
      if (inhibit_value > def_status->m_inhibited && skill_check<Skill::inhibit>(fd, att_status, def_status))
      {
        _DEBUG_MSG(1, "%s inhibits %s by %u\n",
                status_description(att_status).c_str(),
                status_description(def_status).c_str(), inhibit_value);
        def_status->m_inhibited = inhibit_value;
        return true;
      }
      return false;
}
bool check_and_perform_sabotage(Field* fd, CardStatus* att_status, CardStatus* def_status)
{
    unsigned sabotage_value = att_status->skill(Skill::sabotage);
    if (sabotage_value > def_status->m_sabotaged && skill_check<Skill::sabotage>(fd, att_status, def_status))
    {
        _DEBUG_MSG(1, "%s sabotages %s by %u\n",
                status_description(att_status).c_str(),
                status_description(def_status).c_str(), sabotage_value);
        def_status->m_sabotaged = sabotage_value;
        return true;
    }
    return false;
}
bool check_and_perform_disease(Field* fd, CardStatus* att_status,CardStatus* def_status)
{
    unsigned disease_base = att_status->skill(Skill::disease);
    if(disease_base && skill_check<Skill::disease>(fd, att_status, def_status)) {
        _DEBUG_MSG(1, "%s diseases %s for %u\n",
        status_description(att_status).c_str(), status_description(def_status).c_str(), disease_base);
        def_status->m_diseased += disease_base;
        return true;
    }
    return false;
}

CardStatus* check_and_perform_summon(Field* fd, CardStatus* src)
{
    unsigned summon_card_id = src->m_card->m_skill_value[Skill::summon];
    if (summon_card_id)
    {
        const Card* summoned_card(fd->cards.by_id(summon_card_id));
        _DEBUG_MSG(1, "%s summons %s\n", status_description(src).c_str(), summoned_card->m_name.c_str());
        CardStatus* summoned_status = nullptr;
        switch (summoned_card->m_type)
        {
            case CardType::assault:
                summoned_status = PlayCard(summoned_card, fd, src->m_player, src).op<CardType::assault>(true);
                return summoned_status;
            case CardType::structure:
                summoned_status = PlayCard(summoned_card, fd, src->m_player, src).op<CardType::structure>(true);
                return summoned_status;
            default:
                _DEBUG_MSG(0, "Unknown card type: #%u %s: %u\n",
                        summoned_card->m_id, card_description(fd->cards, summoned_card).c_str(),
                        summoned_card->m_type);
                _DEBUG_ASSERT(false);
                __builtin_unreachable();
        }
    }
    return nullptr;
}


    template<Skill::Skill skill_id>
size_t select_targets(Field* fd, CardStatus* tsrc, const SkillSpec& s)
{
    size_t n_candidates;
    CardStatus* src;
    if(fd->fixes[Fix::revenge_on_death] && s.s2 == Skill::revenge)
    {
    	_DEBUG_MSG(2,"FIX ON DEATH REVENGE SELECTION")
    	src = &fd->players[(tsrc->m_player+1)%2]->commander; // selection like enemy commander
    }
    else
    {
    	src = tsrc;
    }
    switch (skill_id)
    {
        case Skill::mortar:
            n_candidates = select_fast<Skill::siege>(fd, src, skill_targets<Skill::siege>(fd, src), s);
            if (n_candidates == 0)
            {
                n_candidates = select_fast<Skill::strike>(fd, src, skill_targets<Skill::strike>(fd, src), s);
            }
            break;

        default:
            n_candidates = select_fast<skill_id>(fd, src, skill_targets<skill_id>(fd, src), s);
            break;
    }

    // (false-loop)
    unsigned n_selected = n_candidates;
    do
    {
        // no candidates
        if (n_candidates == 0)
        { break; }

        // show candidates (debug)
        _DEBUG_SELECTION("%s", skill_names[skill_id].c_str());

        // analyze targets count / skill
        unsigned n_targets = s.n > 0 ? s.n : 1;
        if (s.all || n_targets >= n_candidates || skill_id == Skill::mend || skill_id == Skill::fortify)  // target all or mend
        { break; }

        // shuffle & trim
        for (unsigned i = 0; i < n_targets; ++i)
        {
            std::swap(fd->selection_array[i], fd->selection_array[fd->rand(i, n_candidates - 1)]);
        }
        fd->selection_array.resize(n_targets);
        if (n_targets > 1)
        {
            std::sort(fd->selection_array.begin(), fd->selection_array.end(),
                    [](const CardStatus * a, const CardStatus * b) { return a->m_index < b->m_index; });
        }
        n_selected = n_targets;

    } while (false); // (end)

    return n_selected;
}

    template<Skill::Skill skill_id>
void perform_targetted_allied_fast(Field* fd, CardStatus* src, const SkillSpec& s)
{
    select_targets<skill_id>(fd, src, s);
    unsigned num_inhibited = 0;
#ifndef NQUEST
    bool has_counted_quest = false;
#endif
    bool src_overloaded = src->m_overloaded;
    unsigned selection_array_len = fd->selection_array.size();
    CardStatus * selection_array[selection_array_len];
    std::memcpy(selection_array, &fd->selection_array[0], selection_array_len * sizeof(CardStatus *));
    for (CardStatus * dst: selection_array)
    {
        if (dst->m_inhibited > 0 && !src_overloaded)
        {
            _DEBUG_MSG(1, "%s %s on %s but it is inhibited\n",
                    status_description(src).c_str(), skill_short_description(fd->cards, s).c_str(),
                    status_description(dst).c_str());
            -- dst->m_inhibited;
            ++ num_inhibited;
            continue;
        }
        check_and_perform_skill<skill_id>(fd, src, dst, s, false
#ifndef NQUEST
                , has_counted_quest
#endif
                );
    }

    // Passive BGE: Divert
    if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::divert], false)
            && (num_inhibited > 0))
    {
        SkillSpec diverted_ss = s;
        diverted_ss.y = allfactions;
        diverted_ss.n = 1;
        diverted_ss.all = false;
        for (unsigned i = 0; i < num_inhibited; ++ i)
        {
            select_targets<skill_id>(fd, &fd->tip->commander, diverted_ss);
            selection_array_len = fd->selection_array.size();
            std::memcpy(selection_array, &fd->selection_array[0], selection_array_len * sizeof(CardStatus *));
            for (CardStatus * dst: selection_array)
            {
                if (dst->m_inhibited > 0)
                {
                    _DEBUG_MSG(1, "%s %s (Diverted) on %s but it is inhibited\n",
                            status_description(src).c_str(), skill_short_description(fd->cards, diverted_ss).c_str(),
                            status_description(dst).c_str());
                    -- dst->m_inhibited;
                    continue;
                }
                _DEBUG_MSG(1, "%s %s (Diverted) on %s\n",
                        status_description(src).c_str(), skill_short_description(fd->cards, diverted_ss).c_str(),
                        status_description(dst).c_str());
                perform_skill<skill_id>(fd, src, dst, diverted_ss);
            }
        }
    }
}

void perform_targetted_allied_fast_rush(Field* fd, CardStatus* src, const SkillSpec& s)
{
    if (src->m_card->m_type == CardType::commander)
    {  // Passive BGE skills are casted as by commander
        perform_targetted_allied_fast<Skill::rush>(fd, src, s);
        return;
    }
    if (src->m_rush_attempted)
    {
        _DEBUG_MSG(2, "%s does not check Rush again.\n", status_description(src).c_str());
        return;
    }
    _DEBUG_MSG(1, "%s attempts to activate Rush.\n", status_description(src).c_str());
    perform_targetted_allied_fast<Skill::rush>(fd, src, s);
    src->m_rush_attempted = true;
}

    template<Skill::Skill skill_id>
void perform_targetted_hostile_fast(Field* fd, CardStatus* src, const SkillSpec& s)
{
    select_targets<skill_id>(fd, src, s);
    std::vector<CardStatus *> paybackers;
#ifndef NQUEST
    bool has_counted_quest = false;
#endif
    const bool has_turningtides = (fd->bg_effects[fd->tapi][PassiveBGE::turningtides] && (skill_id == Skill::weaken || skill_id == Skill::sunder));
    unsigned turningtides_value(0), old_attack(0);

    // apply skill to each target(dst)
    unsigned selection_array_len = fd->selection_array.size();
    CardStatus * selection_array[selection_array_len];
    std::memcpy(selection_array, &fd->selection_array[0], selection_array_len * sizeof(CardStatus *));
    for (CardStatus * dst: selection_array)
    {
        // TurningTides
        if (__builtin_expect(has_turningtides, false))
        {
            old_attack = dst->attack_power();
        }

        // check & apply skill to target(dst)
        if (check_and_perform_skill<skill_id>(fd, src, dst, s, ! (src->m_overloaded || (__builtin_expect(fd->fixes[Fix::dont_evade_mimic_selection],true) &&  skill_id == Skill::mimic))
#ifndef NQUEST
                    , has_counted_quest
#endif
                    ))
        {
            // TurningTides: get max attack decreasing
            if (__builtin_expect(has_turningtides, false))
            {
                turningtides_value = std::max(turningtides_value, safe_minus(old_attack, dst->attack_power()));
            }

            // Payback/Revenge: collect paybackers/revengers
            unsigned payback_value = dst->skill(Skill::payback) + dst->skill(Skill::revenge);
            if ((s.id != Skill::mimic) && (dst->m_paybacked < payback_value) && skill_check<Skill::payback>(fd, dst, src))
            {
                paybackers.reserve(selection_array_len);
                paybackers.push_back(dst);
            }
        }
    }

    // apply TurningTides
    if (__builtin_expect(has_turningtides, false) && (turningtides_value > 0))
    {
        SkillSpec ss_rally{Skill::rally, turningtides_value, allfactions, 0, 0, Skill::no_skill, Skill::no_skill, s.all, 0,};
        _DEBUG_MSG(1, "TurningTides %u!\n", turningtides_value);
        perform_targetted_allied_fast<Skill::rally>(fd, &fd->players[src->m_player]->commander, ss_rally);
    }

    prepend_on_death(fd);  // skills

    // Payback/Revenge
    for (CardStatus * pb_status: paybackers)
    {
        turningtides_value = 0;

        // apply Revenge
        if (pb_status->skill(Skill::revenge))
        {
            unsigned revenged_count(0);
            for (unsigned case_index(0); case_index < 3; ++ case_index)
            {
                CardStatus * target_status;
#ifndef NDEBUG
                const char * target_desc;
#endif
                switch (case_index)
                {
                    // revenge to left
                    case 0:
                        if (!(target_status = fd->left_assault(src))) { continue; }
#ifndef NDEBUG
                        target_desc = "left";
#endif
                        break;

                        // revenge to core
                    case 1:
                        target_status = src;
#ifndef NDEBUG
                        target_desc = "core";
#endif
                        break;

                        // revenge to right
                    case 2:
                        if (!(target_status = fd->right_assault(src))) { continue; }
#ifndef NDEBUG
                        target_desc = "right";
#endif
                        break;

                        // wtf?
                    default:
                        __builtin_unreachable();
                }

                // skip illegal target
                if (!skill_predicate<skill_id>(fd, target_status, target_status, s))
                {
                    continue;
                }

                // skip dead target
                if (!is_alive(target_status))
                {
#ifndef NDEBUG
                    _DEBUG_MSG(1, "(CANCELLED: target unit dead) %s Revenge (to %s) %s on %s\n",
                            status_description(pb_status).c_str(), target_desc,
                            skill_short_description(fd->cards, s).c_str(), status_description(target_status).c_str());
#endif
                    continue;
                }

                // TurningTides
                if (__builtin_expect(has_turningtides, false))
                {
                    old_attack = target_status->attack_power();
                }

                // apply revenged skill
#ifndef NDEBUG
                _DEBUG_MSG(1, "%s Revenge (to %s) %s on %s\n",
                        status_description(pb_status).c_str(), target_desc,
                        skill_short_description(fd->cards, s).c_str(), status_description(target_status).c_str());
#endif
                perform_skill<skill_id>(fd, pb_status, target_status, s);
                ++ revenged_count;

                // revenged TurningTides: get max attack decreasing
                if (__builtin_expect(has_turningtides, false))
                {
                    turningtides_value = std::max(turningtides_value, safe_minus(old_attack, target_status->attack_power()));
                }
            }
            if (revenged_count)
            {
                // consume remaining payback/revenge
                ++ pb_status->m_paybacked;

                // apply TurningTides
                if (__builtin_expect(has_turningtides, false) && (turningtides_value > 0))
                {
                    SkillSpec ss_rally{Skill::rally, turningtides_value, allfactions, 0, 0, Skill::no_skill, Skill::no_skill, false, 0,};
                    _DEBUG_MSG(1, "Paybacked TurningTides %u!\n", turningtides_value);
                    perform_targetted_allied_fast<Skill::rally>(fd, &fd->players[pb_status->m_player]->commander, ss_rally);
                }
            }
        }
        // apply Payback
        else
        {
            // skip illegal target(src)
            if (!skill_predicate<skill_id>(fd, src, src, s))
            {
                continue;
            }

            // skip dead target(src)
            if (!is_alive(src))
            {
                _DEBUG_MSG(1, "(CANCELLED: src unit dead) %s Payback %s on %s\n",
                        status_description(pb_status).c_str(), skill_short_description(fd->cards, s).c_str(),
                        status_description(src).c_str());
                continue;
            }

            // TurningTides
            if (__builtin_expect(has_turningtides, false))
            {
                old_attack = src->attack_power();
            }

            // apply paybacked skill
            _DEBUG_MSG(1, "%s Payback %s on %s\n",
                    status_description(pb_status).c_str(), skill_short_description(fd->cards, s).c_str(), status_description(src).c_str());
            perform_skill<skill_id>(fd, pb_status, src, s);
            ++ pb_status->m_paybacked;

            // handle paybacked TurningTides
            if (__builtin_expect(has_turningtides, false))
            {
                turningtides_value = std::max(turningtides_value, safe_minus(old_attack, src->attack_power()));
                if (turningtides_value > 0)
                {
                    SkillSpec ss_rally{Skill::rally, turningtides_value, allfactions, 0, 0, Skill::no_skill, Skill::no_skill, false, 0,};
                    _DEBUG_MSG(1, "Paybacked TurningTides %u!\n", turningtides_value);
                    perform_targetted_allied_fast<Skill::rally>(fd, &fd->players[pb_status->m_player]->commander, ss_rally);
                }
            }
        }
    }

    prepend_on_death(fd,true);  // paybacked skills
}

//------------------------------------------------------------------------------
inline unsigned evaluate_brawl_score(Field* fd, unsigned player)
{
    const auto & p = fd->players;
    return 55
        // - (10 - p[player]->deck->cards.size())
        // + (10 - p[opponent(player)]->deck->cards.size())
        + p[opponent(player)]->total_cards_destroyed
        + p[player]->deck->shuffled_cards.size()
        - (unsigned)((fd->turn+7)/8);
}

inline unsigned evaluate_war_score(Field* fd, unsigned player)
{
    return 208 - ((unsigned)(fd->turn)/2)*4;
}
int evaluate_card(Field* fd,const Card* cs);
int evaluate_skill(Field* fd,const Card* c , SkillSpec* ss)
{
	// TODO optimize this
	int tvalue = ss->x;

	if(ss->card_id != 0)tvalue += 1*evaluate_card(fd,card_by_id_safe(fd->cards,ss->card_id));
	tvalue += 10*(ss->id==Skill::flurry);
	tvalue += 10*(ss->id==Skill::jam);
	tvalue += 5*(ss->id==Skill::overload);
	tvalue += 2*(ss->id==Skill::flying);
	tvalue += 2*(ss->id==Skill::evolve);
	tvalue += 2*(ss->id==Skill::wall);
	tvalue += 5*(ss->id==Skill::tribute);

	tvalue *= 1.+1.5*(ss->id==Skill::flurry);
	tvalue *= 1.+1.5*(ss->id==Skill::drain);
	tvalue *= 1.+1.5*(ss->id==Skill::mortar);
	tvalue *= 1.+1.5*(ss->id==Skill::scavenge);
	tvalue *= 1.+1.5*(ss->id==Skill::disease);

	tvalue *= 1.+1.3*(ss->id==Skill::rally);
	tvalue *= 1.+1.3*(ss->id==Skill::strike);

	tvalue *= 1.+1.2*(ss->id==Skill::avenge);
	tvalue *= 1.+1.1*(ss->id==Skill::sunder);
	tvalue *= 1.+1.1*(ss->id==Skill::venom);

	tvalue *= 1.+1.0*(ss->id==Skill::evade);
	tvalue *= 1.+1.0*(ss->id==Skill::enfeeble);

	tvalue *= 1.+0.2*(ss->id==Skill::protect);

	tvalue *= 1.+0.2*(ss->id==Skill::fortify);
	tvalue *= 1.+0.5*(ss->id==Skill::mend);

	tvalue *= 1.+0.4*(ss->id==Skill::jam);
	tvalue *= 1.+0.4*(ss->id==Skill::overload);
	//tvalue *= 1.+0.4*(ss->id==Skill::rupture);
	tvalue *= 1.+0.4*(ss->id==Skill::bravery);
	tvalue *= 1.+0.4*(ss->id==Skill::entrap);
	tvalue *= 1.+0.4*(ss->id==Skill::heal);


	tvalue *= 1.+0.3*(ss->id==Skill::revenge);
	tvalue *= 1.+0.3*(ss->id==Skill::enrage);


	//tvalue *= 1.+2.1*(ss->id==Skill::hunt);
	tvalue *= 1.+0.1*(ss->id==Skill::mark);
	tvalue *= 1.+0.1*(ss->id==Skill::coalition);
	tvalue *= 1.+0.1*(ss->id==Skill::legion);
	//tvalue *= 1.+1.1*(ss->id==Skill::barrier);
	tvalue *= 1.+0.1*(ss->id==Skill::pierce);
	tvalue *= 1.+0.1*(ss->id==Skill::armor);
	//tvalue *= 1.+0.1*(ss->id==Skill::swipe);
	//tvalue *= 1.+0.1*(ss->id==Skill::berserk);
	tvalue *= 1.-0.1*(ss->id==Skill::weaken);



	tvalue *= 1.-0.5 *(ss->id==Skill::sabotage); //sucks
	tvalue *= 1.-0.5 *(ss->id==Skill::inhibit); //sucks
	tvalue *= 1.-0.5 *(ss->id==Skill::corrosive); //sucks
	tvalue *= 1.-0.5 *(ss->id==Skill::payback); //sucks
	tvalue *= 1.-0.5 *(ss->id==Skill::leech); //sucks


	tvalue *= 1.+1*ss->all;
	tvalue *= 1.-1./5.*ss->all*(ss->y!=0);
	tvalue *= 1.+1*std::min<int>(3,ss->n);
	tvalue *= 1.-1./3.* ((c->m_skill_trigger[ss->id] == Skill::Trigger::death) + (c->m_skill_trigger[ss->id] == Skill::Trigger::play));
	tvalue *= 1./(2.+ss->c);
	//if(tvalue == 0) std::cout << ss->id << " "<<tvalue << std::endl;
	//if(tvalue > 10000) std::cout << ss->id <<" "<< tvalue << std::endl;
	return 0.9*tvalue; // 0.85
}
int evaluate_card(Field* fd,const Card* cs)
{
	int value = 0;
	value += cs->m_health;
	value += 2*cs->m_attack;
	for( auto ss : cs->m_skills) {
		value += evaluate_skill(fd,cs,&ss);
	}
	int denom_scale = 1+cs->m_delay*0;
	//if(value > 10000) std::cout << cs->m_name << value << std::endl;
	return value /denom_scale;
}
int evaluate_cardstatus(Field* fd,CardStatus* cs)
{
	int value = 0;
	value += cs->m_hp;
	value += 2*cs->attack_power();
	value += cs->protected_value();
	for( auto ss : cs->m_card->m_skills) {
		value += evaluate_skill(fd,cs->m_card,&ss);
	}
	value -= (cs->m_enfeebled);
	int denom_scale = 1+cs->m_delay*0;
	if(value > 10000) std::cout << cs->m_card->m_name << value <<std::endl;
	return value /denom_scale;
}
// calculate a value for current field, high values are better for fd->tap
// dead commander -> the player gets zero value
int evaluate_field(Field* fd)
{
	int value = 0;

	int scale = is_alive(&fd->tap->commander);
	auto& assaults(fd->tap->assaults);
	auto& structures(fd->tap->structures);


	value += 0.5*scale * evaluate_cardstatus(fd,&fd->tap->commander);
	for(unsigned index(0); index < assaults.size();++index)
	{
		value += scale * evaluate_cardstatus(fd,&assaults[index]);
	}
	for(unsigned index(0); index < structures.size();++index)
	{
		value += scale * evaluate_cardstatus(fd,&structures[index]);
	}

	scale = is_alive(&fd->tip->commander);
	auto& eassaults(fd->tip->assaults);
	auto& estructures(fd->tip->structures);
	value -= 0.5*scale * evaluate_cardstatus(fd,&fd->tip->commander);
	for(unsigned index(0); index < eassaults.size();++index)
	{
		value -= (scale * evaluate_cardstatus(fd,&eassaults[index]));
	}
	for(unsigned index(0); index < estructures.size();++index)
	{
		value -= (scale * evaluate_cardstatus(fd,&estructures[index]));
	}
	return value;
}


Results<uint64_t> evaluate_sim_result(Field* fd, bool single_turn_both)
{
    typedef unsigned points_score_type;
    const auto & p = fd->players;
    unsigned raid_damage = 0;
#ifndef NQUEST
    unsigned quest_score = 0;
#endif

    if(single_turn_both)
    {
	bool sign = evaluate_field(fd)<0;
	unsigned val = evaluate_field(fd) *(1-2*sign);
	return {!is_alive(&fd->players[1]->commander),sign,!is_alive(&fd->players[0]->commander),val,1};
    }
    switch (fd->optimization_mode)
    {
        case OptimizationMode::raid:
            raid_damage = 15
                + (p[1]->total_nonsummon_cards_destroyed)
                - (10 * p[1]->commander.m_hp / p[1]->commander.max_hp());
            break;
#ifndef NQUEST
        case OptimizationMode::quest:
            if (fd->quest.quest_type == QuestType::card_survival)
            {
                for (const auto & status: p[0]->assaults.m_indirect)
                { fd->quest_counter += (fd->quest.quest_key == status->m_card->m_id); }
                for (const auto & status: p[0]->structures.m_indirect)
                { fd->quest_counter += (fd->quest.quest_key == status->m_card->m_id); }
                for (const auto & card: p[0]->deck->shuffled_cards)
                { fd->quest_counter += (fd->quest.quest_key == card->m_id); }
            }
            quest_score = fd->quest.must_fulfill ? (fd->quest_counter >= fd->quest.quest_value ? fd->quest.quest_score : 0) : std::min<unsigned>(fd->quest.quest_score, fd->quest.quest_score * fd->quest_counter / fd->quest.quest_value);
            _DEBUG_MSG(1, "Quest: %u / %u = %u%%.\n", fd->quest_counter, fd->quest.quest_value, quest_score);
            break;
#endif
        default:
            break;
    }
    // you lose
    if(!is_alive(&fd->players[0]->commander))
    {
        _DEBUG_MSG(1, "You lose.\n");
        switch (fd->optimization_mode)
        {
            case OptimizationMode::raid: return {0, 0, 1, (points_score_type)raid_damage,1};
            case OptimizationMode::brawl: return {0, 0, 1, (points_score_type) 5,1};
            case OptimizationMode::brawl_defense:
                                          {
                                              unsigned enemy_brawl_score = evaluate_brawl_score(fd, 1);
                                              unsigned max_score = max_possible_score[(size_t)OptimizationMode::brawl_defense];
                                              if(enemy_brawl_score> max_score)
                                                std::cerr << "WARNING: enemy_brawl_score > max_possible_brawl_score" << std::endl;
                                              return {0, 0, 1, (points_score_type)safe_minus(max_score , enemy_brawl_score),1};
                                          }
            case OptimizationMode::war: return {0,0,1, (points_score_type) 20,1};
            case OptimizationMode::war_defense:
                                        {
                                            unsigned enemy_war_score = evaluate_war_score(fd, 1);
                                            unsigned max_score = max_possible_score[(size_t)OptimizationMode::war_defense];
                                            if(enemy_war_score> max_score)
                                                std::cerr << "WARNING: enemy_war_score > max_possible_war_score" << std::endl;
                                            return {0, 0, 1, (points_score_type)safe_minus(max_score , enemy_war_score),1};
                                        }
#ifndef NQUEST
            case OptimizationMode::quest: return {0, 0, 1, (points_score_type)(fd->quest.must_win ? 0 : quest_score),1};
#endif
            default: return {0, 0, 1, 0,1};
        }
    }
    // you win
    if(!is_alive(&fd->players[1]->commander))
    {
        _DEBUG_MSG(1, "You win.\n");
        switch (fd->optimization_mode)
        {
            case OptimizationMode::brawl:
                {
                    unsigned brawl_score = evaluate_brawl_score(fd, 0);
                    return {1, 0, 0, (points_score_type)brawl_score,1};
                }
            case OptimizationMode::brawl_defense:
                {
                    unsigned max_score = max_possible_score[(size_t)OptimizationMode::brawl_defense];
                    unsigned min_score = min_possible_score[(size_t)OptimizationMode::brawl_defense];
                    return {1, 0, 0, (points_score_type)(max_score - min_score),1};
                }
            case OptimizationMode::campaign:
                {
                    unsigned total_dominions_destroyed = (p[0]->deck->alpha_dominion != nullptr) - p[0]->structures.count(is_it_dominion);
                    unsigned campaign_score = 100 - 10 * (p[0]->total_nonsummon_cards_destroyed - total_dominions_destroyed);
                    return {1, 0, 0, (points_score_type)campaign_score,1};
                }
            case OptimizationMode::war:
                {
                    unsigned war_score = evaluate_war_score(fd, 0);
                    return {1,0,0, (points_score_type) war_score,1};
                }
            case OptimizationMode::war_defense:
                {
                    unsigned max_score = max_possible_score[(size_t)OptimizationMode::war_defense];
                    unsigned min_score = min_possible_score[(size_t)OptimizationMode::war_defense];
                    return {1, 0, 0, (points_score_type)(max_score - min_score),1};
                }
#ifndef NQUEST
            case OptimizationMode::quest: return {1, 0, 0, (points_score_type)(fd->quest.win_score + quest_score),1};
#endif
            default:
                                          return {1, 0, 0, 100,1};
        }
    }
    if (fd->turn > turn_limit)
    {
        _DEBUG_MSG(1, "Stall after %u turns.\n", turn_limit);
        switch (fd->optimization_mode)
        {
            case OptimizationMode::defense: return {0, 1, 0, 100,1};
            case OptimizationMode::raid: return {0, 1, 0, (points_score_type)raid_damage,1};
            case OptimizationMode::brawl: return {0, 1, 0, 5,1};
            case OptimizationMode::brawl_defense:
                                          {
                                              unsigned max_score = max_possible_score[(size_t)OptimizationMode::brawl_defense];
                                              unsigned min_score = min_possible_score[(size_t)OptimizationMode::brawl_defense];
                                              return {1, 0, 0, (points_score_type)(max_score - min_score),1};
                                          }
            case OptimizationMode::war: return {0,1,0, (points_score_type) 20,1};
            case OptimizationMode::war_defense:
                                        {
                                            unsigned max_score = max_possible_score[(size_t)OptimizationMode::war_defense];
                                            unsigned min_score = min_possible_score[(size_t)OptimizationMode::war_defense];
                                            return {1, 0, 0, (points_score_type)(max_score - min_score),1};
                                        }
#ifndef NQUEST
            case OptimizationMode::quest: return {0, 1, 0, (points_score_type)(fd->quest.must_win ? 0 : quest_score),1};
#endif
            default: return {0, 1, 0, 0,1};
        }
    }

    // Huh? How did we get here?
    assert(false);
    return {0, 0, 0, 0,1};
}

//------------------------------------------------------------------------------
//turns_both sets the number of turns to sim before exiting before winner exists.
Results<uint64_t> play(Field* fd,bool skip_init, bool skip_preplay,unsigned turns_both)
{
    if(!skip_init){ //>>> start skip init
        fd->players[0]->commander.m_player = 0;
        fd->players[1]->commander.m_player = 1;
        fd->tapi = fd->gamemode == surge ? 1 : 0;
        fd->tipi = opponent(fd->tapi);
        fd->tap = fd->players[fd->tapi];
        fd->tip = fd->players[fd->tipi];
        fd->end = false;

        // Play dominion & fortresses
        for (unsigned _(0), ai(fd->tapi); _ < 2; ++_)
        {
            if (fd->players[ai]->deck->alpha_dominion)
            { PlayCard(fd->players[ai]->deck->alpha_dominion, fd, ai, &fd->players[ai]->commander).op<CardType::structure>(); }
            for (const Card* played_card: fd->players[ai]->deck->shuffled_forts)
            {

                switch (played_card->m_type)
                {
                    case CardType::assault:
                        PlayCard(played_card, fd, ai, &fd->players[ai]->commander).op<CardType::assault>();
                        break;
                    case CardType::structure:
                        PlayCard(played_card, fd, ai, &fd->players[ai]->commander).op<CardType::structure>();
                        break;
                    case CardType::commander:
                    case CardType::num_cardtypes:
                        _DEBUG_MSG(0, "Unknown card type: #%u %s: %u\n",
                                played_card->m_id, card_description(fd->cards, played_card).c_str(), played_card->m_type);
                        assert(false);
                        break;
                }
            }
	    resolve_skill(fd);
            std::swap(fd->tapi, fd->tipi);
            std::swap(fd->tap, fd->tip);
            ai = opponent(ai);
        }
    }//>>> end skip init
    unsigned both_turn_limit = fd->turn+2*turns_both;
    while(__builtin_expect(fd->turn <= turn_limit && !fd->end && (turns_both==0 || fd->turn < both_turn_limit), true))
    {
        if(!skip_preplay){ //>>> start skip init

            fd->current_phase = Field::playcard_phase;
            // Initialize stuff, remove dead cards
            _DEBUG_MSG(1, "------------------------------------------------------------------------\n"
                    "TURN %u begins for %s\n", fd->turn, status_description(&fd->tap->commander).c_str());

            // reduce timers & perform triggered skills (like Summon)
            fd->prepare_action();
            turn_start_phase(fd); // summon may postpone skills to be resolved
            resolve_skill(fd); // resolve postponed skills recursively
            fd->finalize_action();

            //bool bge_megamorphosis = fd->bg_effects[fd->tapi][PassiveBGE::megamorphosis];

        }//>>> end skip init
        else { skip_preplay = false;}
        // Play a card
        const Card* played_card(fd->tap->deck->next(fd));
        if (played_card)
        {

            // Begin 'Play Card' phase action
            fd->prepare_action();

            // Play selected card
            //CardStatus* played_status = nullptr;
            switch (played_card->m_type)
            {
                case CardType::assault:
                    PlayCard(played_card, fd, fd->tapi, &fd->tap->commander).op<CardType::assault>();
                    break;
                case CardType::structure:
                    PlayCard(played_card, fd, fd->tapi, &fd->tap->commander).op<CardType::structure>();
                    break;
                case CardType::commander:
                case CardType::num_cardtypes:
                    _DEBUG_MSG(0, "Unknown card type: #%u %s: %u\n",
                            played_card->m_id, card_description(fd->cards, played_card).c_str(), played_card->m_type);
                    assert(false);
                    break;
            }
            resolve_skill(fd); // resolve postponed skills recursively
            //status_description(played_status)
            //_DEBUG_MSG(3,"Card played: %s", status_description(played_status).c_str());
            // End 'Play Card' phase action
            fd->finalize_action();



        }
        if (__builtin_expect(fd->end, false)) { break; }

        //-------------------------------------------------
        // Phase: (Later-) Enhance, Inhibit, Sabotage, Disease
        //-------------------------------------------------
        //Skill: Enhance
        //Perform later enhance for commander
        if(!fd->fixes[Fix::enhance_early]) {
        check_and_perform_later_enhance(fd,&fd->tap->commander);
        auto& structures(fd->tap->structures);
        for(unsigned index(0); index < structures.size(); ++index)
        {
            CardStatus * status = &structures[index];
            //enhance everything else after card was played
            check_and_perform_later_enhance(fd,status);
        }
        }
        //Perform Inhibit, Sabotage, Disease
        auto& assaults(fd->tap->assaults);
        for(unsigned index(0); index < assaults.size(); ++index)
        {
            CardStatus * att_status = &assaults[index];
            if(att_status->m_index >= fd->tip->assaults.size())continue; //skip no enemy
            auto def_status = &fd->tip->assaults[att_status->m_index];
            if(!is_alive(def_status))continue; //skip dead

            check_and_perform_inhibit(fd,att_status,def_status);
            check_and_perform_sabotage(fd,att_status,def_status);
            check_and_perform_disease(fd,att_status,def_status);
        }
        //-------------------------------------------------

        // Evaluate Passive BGE Heroism skills
        if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::heroism], false))
        {
            for (CardStatus * dst: fd->tap->assaults.m_indirect)
            {
                unsigned bge_value = (dst->skill(Skill::valor) + dst->skill(Skill::bravery)+ 1) / 2;
                if (bge_value <= 0)
                { continue; }
                SkillSpec ss_protect{Skill::protect, bge_value, allfactions, 0, 0, Skill::no_skill, Skill::no_skill, false, 0,};
                if (dst->m_inhibited > 0)
                {
                    _DEBUG_MSG(1, "Heroism: %s on %s but it is inhibited\n",
                            skill_short_description(fd->cards, ss_protect).c_str(), status_description(dst).c_str());
                    -- dst->m_inhibited;

                    // Passive BGE: Divert
                    if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::divert], false))
                    {
                        SkillSpec diverted_ss = ss_protect;
                        diverted_ss.y = allfactions;
                        diverted_ss.n = 1;
                        diverted_ss.all = false;
                        // for (unsigned i = 0; i < num_inhibited; ++ i)
                        {
                            select_targets<Skill::protect>(fd, &fd->tip->commander, diverted_ss);
                            unsigned selection_array_len = fd->selection_array.size();
                            CardStatus * selection_array[selection_array_len];
                            std::memcpy(selection_array, &fd->selection_array[0], selection_array_len * sizeof(CardStatus *));
                            for (CardStatus * dst: selection_array)
                            {
                                if (dst->m_inhibited > 0)
                                {
                                    _DEBUG_MSG(1, "Heroism: %s (Diverted) on %s but it is inhibited\n",
                                            skill_short_description(fd->cards, diverted_ss).c_str(), status_description(dst).c_str());
                                    -- dst->m_inhibited;
                                    continue;
                                }
                                _DEBUG_MSG(1, "Heroism: %s (Diverted) on %s\n",
                                        skill_short_description(fd->cards, diverted_ss).c_str(), status_description(dst).c_str());
                                perform_skill<Skill::protect>(fd, &fd->tap->commander, dst, diverted_ss);  // XXX: the caster
                            }
                        }
                    }
                    continue;
                }
#ifndef NQUEST
                bool has_counted_quest = false;
#endif
                check_and_perform_skill<Skill::protect>(fd, &fd->tap->commander, dst, ss_protect, false
#ifndef NQUEST
                        , has_counted_quest
#endif
                        );
            }
        }

        // Evaluate activation BGE skills
        fd->current_phase = Field::bge_phase;
        for (const auto & bg_skill: fd->bg_skills[fd->tapi])
        {
            fd->prepare_action();
            _DEBUG_MSG(2, "Evaluating BG skill %s\n", skill_description(fd->cards, bg_skill).c_str());
            fd->skill_queue.emplace_back(&fd->tap->commander, bg_skill);
            resolve_skill(fd);
            fd->finalize_action();
        }
        if (__builtin_expect(fd->end, false)) { break; }

        // Evaluate commander
        fd->current_phase = Field::commander_phase;
        evaluate_skills<CardType::commander>(fd, &fd->tap->commander, fd->tap->commander.m_card->m_skills);
        if (__builtin_expect(fd->end, false)) { break; }

        // Evaluate structures
        fd->current_phase = Field::structures_phase;
        for (fd->current_ci = 0; !fd->end && (fd->current_ci < fd->tap->structures.size()); ++fd->current_ci)
        {
            CardStatus* current_status(&fd->tap->structures[fd->current_ci]);
            if (!is_active(current_status))
            {
                _DEBUG_MSG(2, "%s cannot take action.\n", status_description(current_status).c_str());
            }
            else
            {
                evaluate_skills<CardType::structure>(fd, current_status, current_status->m_card->m_skills);
            }
        }

        // Evaluate assaults
        fd->current_phase = Field::assaults_phase;
        fd->bloodlust_value = 0;
        for (fd->current_ci = 0; !fd->end && (fd->current_ci < fd->tap->assaults.size()); ++fd->current_ci)
        {
            CardStatus* current_status(&fd->tap->assaults[fd->current_ci]);
            bool attacked = false;
            if (!is_active(current_status))
            {
                _DEBUG_MSG(2, "%s cannot take action.\n", status_description(current_status).c_str());
                // Passive BGE: HaltedOrders
                /*
                unsigned inhibit_value;
                if (__builtin_expect(fd->bg_effects[fd->tapi][PassiveBGE::haltedorders], false)
                        && (current_status->m_delay > 0) // still frozen
                        && (fd->current_ci < fd->tip->assaults.size()) // across slot isn't empty
                        && is_alive(&fd->tip->assaults[fd->current_ci]) // across assault is alive
                        && ((inhibit_value = current_status->skill(Skill::inhibit))
                            > fd->tip->assaults[fd->current_ci].m_inhibited)) // inhibit/re-inhibit(if higher)
                        {
                            CardStatus* across_status(&fd->tip->assaults[fd->current_ci]);
                            _DEBUG_MSG(1, "Halted Orders: %s inhibits %s by %u\n",
                                    status_description(current_status).c_str(),
                                    status_description(across_status).c_str(), inhibit_value);
                            across_status->m_inhibited = inhibit_value;
                        }
                        */
            }
            else
            {
                if (current_status->m_protected_stasis)
                {
                    _DEBUG_MSG(1, "%s loses Stasis protection (activated)\n",
                            status_description(current_status).c_str());
                }
                current_status->m_protected_stasis = 0;
                fd->assault_bloodlusted = false;
                current_status->m_step = CardStep::attacking;
                evaluate_skills<CardType::assault>(fd, current_status, current_status->m_card->m_skills, &attacked);
                if (__builtin_expect(fd->end, false)) { break; }
                if (__builtin_expect(!is_alive(current_status), false)) { continue; }
            }

            current_status->m_step = CardStep::attacked;
        }
        fd->current_phase = Field::end_phase;
        turn_end_phase(fd);
        if (__builtin_expect(fd->end, false)) { break; }
        _DEBUG_MSG(1, "TURN %u ends for %s\n", fd->turn, status_description(&fd->tap->commander).c_str());
        std::swap(fd->tapi, fd->tipi);
        std::swap(fd->tap, fd->tip);
        ++fd->turn;
    }

    return evaluate_sim_result(fd,turns_both!= 0);
}

//------------------------------------------------------------------------------
void fill_skill_table()
{
    memset(skill_table, 0, sizeof skill_table);
    skill_table[Skill::mortar] = perform_targetted_hostile_fast<Skill::mortar>;
    skill_table[Skill::enfeeble] = perform_targetted_hostile_fast<Skill::enfeeble>;
    skill_table[Skill::enhance] = perform_targetted_allied_fast<Skill::enhance>;
    skill_table[Skill::evolve] = perform_targetted_allied_fast<Skill::evolve>;
    skill_table[Skill::heal] = perform_targetted_allied_fast<Skill::heal>;
    skill_table[Skill::jam] = perform_targetted_hostile_fast<Skill::jam>;
    skill_table[Skill::mend] = perform_targetted_allied_fast<Skill::mend>;
    skill_table[Skill::fortify] = perform_targetted_allied_fast<Skill::fortify>;
    skill_table[Skill::overload] = perform_targetted_allied_fast<Skill::overload>;
    skill_table[Skill::protect] = perform_targetted_allied_fast<Skill::protect>;
    skill_table[Skill::rally] = perform_targetted_allied_fast<Skill::rally>;
    skill_table[Skill::enrage] = perform_targetted_allied_fast<Skill::enrage>;
    skill_table[Skill::entrap] = perform_targetted_allied_fast<Skill::entrap>;
    skill_table[Skill::rush] = perform_targetted_allied_fast_rush;
    skill_table[Skill::siege] = perform_targetted_hostile_fast<Skill::siege>;
    skill_table[Skill::strike] = perform_targetted_hostile_fast<Skill::strike>;
    skill_table[Skill::sunder] = perform_targetted_hostile_fast<Skill::sunder>;
    skill_table[Skill::weaken] = perform_targetted_hostile_fast<Skill::weaken>;
    skill_table[Skill::mimic] = perform_targetted_hostile_fast<Skill::mimic>;
}
