#include "sim.h"

#include <boost/range/adaptors.hpp>
#include <boost/range/join.hpp>
#include <iostream>
#include <random>
#include <string>
#include <sstream>
#include <vector>

#include "tyrant.h"
#include "card.h"
#include "cards.h"
#include "deck.h"

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
inline const std::vector<CardStatus *> Field::adjacent_assaults(const CardStatus * status)
{
    std::vector<CardStatus *> res;
    auto & assaults = this->players[status->m_player]->assaults;
    if (status->m_index > 0)
    {
        auto left_status = &assaults[status->m_index - 1];
        if (left_status->m_hp > 0)
        {
            res.push_back(left_status);
        }
    }
    if (status->m_index + 1 < assaults.size())
    {
        auto right_status = &assaults[status->m_index + 1];
        if (right_status->m_hp > 0)
        {
            res.push_back(right_status);
        }
    }
    return res;
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
inline unsigned CardStatus::skill_base_value(Skill skill_id) const
{
    return m_card->m_skill_value[skill_id + m_primary_skill_offset[skill_id]];
}
//------------------------------------------------------------------------------
inline unsigned CardStatus::skill(Skill skill_id) const
{
    return skill_base_value(skill_id) + enhanced(skill_id);
}
//------------------------------------------------------------------------------
inline bool CardStatus::has_skill(Skill skill_id) const
{
    return skill_base_value(skill_id);
}
//------------------------------------------------------------------------------
inline unsigned CardStatus::enhanced(Skill skill_id) const
{
    return m_enhanced_value[skill_id + m_primary_skill_offset[skill_id]];
}
//------------------------------------------------------------------------------
inline unsigned CardStatus::protected_value() const
{
    return m_protected;
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
    m_player = 0;
    m_delay = card.m_delay;
    m_faction = card.m_faction;
    m_attack = card.m_attack;
    m_hp = m_max_hp = card.m_health;
    m_step = CardStep::none;

    m_corroded_rate = 0;
    m_corroded_weakened = 0;
    m_enfeebled = 0;
    m_evaded = 0;
    m_inhibited = 0;
    m_jammed = false;
    m_overloaded = false;
    m_paybacked = 0;
    m_poisoned = 0;
    m_protected = 0;
    m_rallied = 0;
    m_rush_attempted = false;
    m_sundered = false;
    m_weakened = 0;

    std::memset(m_primary_skill_offset, 0, sizeof m_primary_skill_offset);
    std::memset(m_evolved_skill_offset, 0, sizeof m_evolved_skill_offset);
    std::memset(m_enhanced_value, 0, sizeof m_enhanced_value);
    std::memset(m_skill_cd, 0, sizeof m_skill_cd);
}
//------------------------------------------------------------------------------
inline unsigned attack_power(const CardStatus* att)
{
    return(safe_minus(att->m_attack + att->m_rallied, att->m_weakened + att->m_corroded_weakened));
}
//------------------------------------------------------------------------------
std::string skill_description(const Cards& cards, const SkillSpec& s)
{
    return skill_names[s.id] +
       (s.all ? " all" : s.n == 0 ? "" : std::string(" ") + to_string(s.n)) +
       (s.y == allfactions ? "" : std::string(" ") + faction_names[s.y]) +
       (s.s == no_skill ? "" : std::string(" ") + skill_names[s.s]) +
       (s.s2 == no_skill ? "" : std::string(" ") + skill_names[s.s2]) +
       (s.x == 0 ? "" : std::string(" ") + to_string(s.x)) +
       (s.c == 0 ? "" : std::string(" every ") + to_string(s.c));
}
std::string skill_short_description(const SkillSpec& s)
{
    // NOTE: not support summon
    return skill_names[s.id] +
        (s.s == no_skill ? "" : std::string(" ") + skill_names[s.s]) +
        (s.s2 == no_skill ? "" : std::string(" ") + skill_names[s.s2]) +
        (s.x == 0 ? "" : std::string(" ") + to_string(s.x));
}
//------------------------------------------------------------------------------
std::string card_description(const Cards& cards, const Card* c)
{
    std::string desc;
    desc = c->m_name;
    switch(c->m_type)
    {
    case CardType::assault:
        desc += ": " + to_string(c->m_attack) + "/" + to_string(c->m_health) + "/" + to_string(c->m_delay);
        break;
    case CardType::structure:
        desc += ": " + to_string(c->m_health) + "/" + to_string(c->m_delay);
        break;
    case CardType::commander:
        desc += ": hp:" + to_string(c->m_health);
        break;
    case CardType::num_cardtypes:
        assert(false);
        break;
    }
    if(c->m_rarity >= 4) { desc += " " + rarity_names[c->m_rarity]; }
    if(c->m_faction != allfactions) { desc += " " + faction_names[c->m_faction]; }
    for(auto& skill: c->m_skills) { desc += ", " + skill_description(cards, skill); }
    return(desc);
}
//------------------------------------------------------------------------------
std::string CardStatus::description() const
{
    std::string desc = "P" + to_string(m_player) + " ";
    switch(m_card->m_type)
    {
    case CardType::commander: desc += "Commander "; break;
    case CardType::assault: desc += "Assault " + to_string(m_index) + " "; break;
    case CardType::structure: desc += "Structure " + to_string(m_index) + " "; break;
    case CardType::num_cardtypes: assert(false); break;
    }
    desc += "[" + m_card->m_name;
    switch(m_card->m_type)
    {
    case CardType::assault:
        desc += " att:" + to_string(m_attack);
        {
            std::string att_desc;
            if(m_rallied > 0) { att_desc += "+" + to_string(m_rallied) + "(rallied)"; }
            if(m_weakened > 0) { att_desc += "-" + to_string(m_weakened) + "(weakened)"; }
            if(m_corroded_weakened > 0) { att_desc += "-" + to_string(m_corroded_weakened) + "(corroded)"; }
            if(!att_desc.empty()) { desc += att_desc + "=" + to_string(attack_power(this)); }
        }
    case CardType::structure:
    case CardType::commander:
        desc += " hp:" + to_string(m_hp);
        break;
    case CardType::num_cardtypes:
        assert(false);
        break;
    }
    if(m_delay > 0) {
        desc += " cd:" + to_string(m_delay);
    }
    // Status w/o value
    if(m_jammed) { desc += ", jammed"; }
    if(m_overloaded) { desc += ", overloaded"; }
    if(m_sundered) { desc += ", sundered"; }
    // Status w/ value
    if(m_corroded_rate > 0) { desc += ", corroded " + to_string(m_corroded_rate); }
    if(m_enfeebled > 0) { desc += ", enfeebled " + to_string(m_enfeebled); }
    if(m_inhibited > 0) { desc += ", inhibited " + to_string(m_inhibited); }
    if(m_poisoned > 0) { desc += ", poisoned " + to_string(m_poisoned); }
    if(m_protected > 0) { desc += ", protected " + to_string(m_protected); }
//    if(m_step != CardStep::none) { desc += ", Step " + to_string(static_cast<int>(m_step)); }
    for (const auto & ss: m_card->m_skills)
    {
        std::string skill_desc;
        if (m_evolved_skill_offset[ss.id] != 0) { skill_desc += "->" + skill_names[ss.id + m_evolved_skill_offset[ss.id]]; }
        if (m_enhanced_value[ss.id] != 0) { skill_desc += " +" + to_string(m_enhanced_value[ss.id]); }
        if (!skill_desc.empty()) { desc += ", " + skill_names[ss.id] + skill_desc; }
    }
    desc += "]";
    return(desc);
}
//------------------------------------------------------------------------------
void Hand::reset(std::mt19937& re)
{
    assaults.reset();
    structures.reset();
    deck->shuffle(re);
    commander.set(deck->shuffled_commander);
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
    evolved_s.id = static_cast<Skill>(evolved_s.id + offset);
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
void prepend_on_death(Field* fd)
{
    if (fd->killed_units.empty())
    {
        return;
    }
    std::vector<std::tuple<CardStatus*, SkillSpec>> od_skills;
    auto & assaults = fd->players[fd->killed_units[0]->m_player]->assaults;
    unsigned stacked_poison_value = 0;
    unsigned last_index = 99;
    CardStatus * left_virulence_victim = nullptr;
    for (auto status: fd->killed_units)
    {
        if (status->m_card->m_type == CardType::assault)
        {
            // Avenge
            for (auto && adj_status: fd->adjacent_assaults(status))
            {
                unsigned avenge_value = adj_status->skill(avenge);
                if (avenge_value > 0)
                {
                    _DEBUG_MSG(1, "%s activates Avenge %u\n", status_description(adj_status).c_str(), avenge_value);
                    if (! adj_status->m_sundered)
                    { adj_status->m_attack += avenge_value; }
                    adj_status->m_max_hp += avenge_value;
                    adj_status->m_hp += avenge_value;
                }
            }
            // Virulence
            if (fd->bg_effects.count(virulence))
            {
                if (status->m_index != last_index + 1)
                {
                    stacked_poison_value = 0;
                    left_virulence_victim = nullptr;
                    if (status->m_index > 0)
                    {
                        auto left_status = &assaults[status->m_index - 1];
                        if (left_status->m_hp > 0)
                        {
                            left_virulence_victim = left_status;
                        }
                    }
                }
                if (status->m_poisoned > 0)
                {
                    if (left_virulence_victim != nullptr)
                    {
                        _DEBUG_MSG(1, "Virulence: %s spreads left poison +%u to %s\n", status_description(status).c_str(), status->m_poisoned, status_description(left_virulence_victim).c_str());
                        left_virulence_victim->m_poisoned += status->m_poisoned;
                    }
                    stacked_poison_value += status->m_poisoned;
                    _DEBUG_MSG(1, "Virulence: %s spreads right poison +%u = %u\n", status_description(status).c_str(), status->m_poisoned, stacked_poison_value);
                }
                if (status->m_index + 1 < assaults.size())
                {
                    auto right_status = &assaults[status->m_index + 1];
                    if (right_status->m_hp > 0)
                    {
                        _DEBUG_MSG(1, "Virulence: spreads stacked poison +%u to %s\n", stacked_poison_value, status_description(right_status).c_str());
                        right_status->m_poisoned += stacked_poison_value;
                    }
                }
                last_index = status->m_index;
            }
        }
        // Revenge
        if (fd->bg_effects.count(revenge))
        {
            SkillSpec ss_heal{heal, fd->bg_effects.at(revenge), allfactions, 0, 0, no_skill, no_skill, true,};
            SkillSpec ss_rally{rally, fd->bg_effects.at(revenge), allfactions, 0, 0, no_skill, no_skill, true,};
            CardStatus * commander = &fd->players[status->m_player]->commander;
            _DEBUG_MSG(2, "Revenge: Preparing skill %s and %s\n", skill_description(fd->cards, ss_heal).c_str(), skill_description(fd->cards, ss_rally).c_str());
            od_skills.emplace_back(commander, ss_heal);
            od_skills.emplace_back(commander, ss_rally);
        }
    }
    fd->skill_queue.insert(fd->skill_queue.begin(), od_skills.begin(), od_skills.end());
    fd->killed_units.clear();
}
//------------------------------------------------------------------------------
void(*skill_table[num_skills])(Field*, CardStatus* src, const SkillSpec&);
void resolve_skill(Field* fd)
{
    while(!fd->skill_queue.empty())
    {
        auto skill_instance(fd->skill_queue.front());
        auto& status(std::get<0>(skill_instance));
        const auto& ss(std::get<1>(skill_instance));
        fd->skill_queue.pop_front();
        if (status->m_jammed)
        {
            _DEBUG_MSG(2, "%s failed to %s because it is Jammed.", status_description(status).c_str(), skill_description(fd->cards, ss).c_str());
            continue;
        }
        signed evolved_offset = status->m_evolved_skill_offset[ss.id];
        auto& evolved_s = status->m_evolved_skill_offset[ss.id] != 0 ? apply_evolve(ss, evolved_offset) : ss;
        unsigned enhanced_value = status->enhanced(evolved_s.id);
        auto& enhanced_s = enhanced_value > 0 ? apply_enhance(evolved_s, enhanced_value) : evolved_s;
        auto& modified_s = enhanced_s;
        skill_table[modified_s.id](fd, status, modified_s);
    }
}
//------------------------------------------------------------------------------
inline bool has_attacked(CardStatus* c) { return(c->m_step == CardStep::attacked); }
inline bool can_act(CardStatus* c) { return(c->m_hp > 0 && !c->m_jammed); }
inline bool is_active(CardStatus* c) { return(can_act(c) && c->m_delay == 0); }
inline bool is_active_next_turn(CardStatus* c) { return(can_act(c) && c->m_delay <= 1); }
// Can be healed / repaired
inline bool can_be_healed(CardStatus* c) { return(c->m_hp > 0 && c->m_hp < c->m_max_hp); }
//------------------------------------------------------------------------------
bool attack_phase(Field* fd);
template<Skill skill_id>
bool check_and_perform_skill(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s, bool is_evadable, bool & has_counted_quest);
bool check_and_perform_valor(Field* fd, CardStatus* src);
template <enum CardType::CardType type>
void evaluate_skills(Field* fd, CardStatus* status, const std::vector<SkillSpec>& skills, bool* attacked=nullptr)
{
    assert(status);
    unsigned num_actions(1);
    for (unsigned action_index(0); action_index < num_actions; ++ action_index)
    {
        assert(fd->skill_queue.size() == 0);
        for (auto & ss: skills)
        {
            // check if activation skill, assuming activation skills can be evolved from only activation skills
            if (skill_table[ss.id] == nullptr)
            {
                continue;
            }
            if (status->m_skill_cd[ss.id] > 0)
            {
                continue;
            }
            _DEBUG_MSG(2, "Evaluating %s skill %s\n", status_description(status).c_str(), skill_description(fd->cards, ss).c_str());
            fd->skill_queue.emplace_back(status, ss);
            resolve_skill(fd);
            if(__builtin_expect(fd->end, false)) { break; }
        }
        if (type == CardType::assault)
        {
            // Attack
            if (can_act(status))
            {
                if (attack_phase(fd) && !*attacked)
                {
                    *attacked = true;
                    if (__builtin_expect(fd->end, false)) { break; }
                }
            }
            else
            {
                _DEBUG_MSG(2, "%s cannot take attack.\n", status_description(status).c_str());
            }
        }
        // Flurry
        if (can_act(status) && fd->tip->commander.m_hp > 0 && status->has_skill(flurry) && status->m_skill_cd[flurry] == 0)
        {
            if (status->m_player == 0)
            {
                fd->inc_counter(QuestType::skill_use, flurry);
            }
            _DEBUG_MSG(1, "%s activates Flurry\n", status_description(status).c_str());
            num_actions = 2;
            for (const auto & ss : skills)
            {
                Skill evolved_skill_id = static_cast<Skill>(ss.id + status->m_evolved_skill_offset[ss.id]);
                if (evolved_skill_id == flurry)
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

    PlayCard(const Card* card_, Field* fd_) :
        card{card_},
        fd{fd_},
        status{nullptr},
        storage{nullptr}
    {}

    template <enum CardType::CardType type>
    bool op()
    {
        setStorage<type>();
        placeCard<type>();
        return(true);
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
        status->m_player = fd->tapi;
        if (status->m_player == 0)
        {
            if (status->m_card->m_type == CardType::assault)
            {
                fd->inc_counter(QuestType::faction_assault_card_use, card->m_faction);
            }
            fd->inc_counter(QuestType::type_card_use, type);
        }
        _DEBUG_MSG(1, "%s plays %s %u [%s]\n", status_description(&fd->tap->commander).c_str(), cardtype_names[type].c_str(), static_cast<unsigned>(storage->size() - 1), card_description(fd->cards, card).c_str());
        if (status->m_delay == 0)
        {
            check_and_perform_valor(fd, status);
        }
    }
};
// assault
template <>
void PlayCard::setStorage<CardType::assault>()
{
    storage = &fd->tap->assaults;
}
// structure
template <>
void PlayCard::setStorage<CardType::structure>()
{
    storage = &fd->tap->structures;
}

// Check if a skill actually proc'ed.
template<Skill>
inline bool skill_check(Field* fd, CardStatus* c, CardStatus* ref)
{ return(true); }

template<>
inline bool skill_check<evade>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(c->m_player != ref->m_player);
}

template<>
inline bool skill_check<leech>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(can_be_healed(c));
}

template<>
inline bool skill_check<legion>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(is_active(c));
}

template<>
inline bool skill_check<payback>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(ref->m_card->m_type == CardType::assault && ref->m_hp > 0);
}

template<>
inline bool skill_check<refresh>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(can_be_healed(c));
}

void remove_hp(Field* fd, CardStatus* status, unsigned dmg)
{
    assert(status->m_hp > 0);
    _DEBUG_MSG(2, "%s takes %u damage\n", status_description(status).c_str(), dmg);
    status->m_hp = safe_minus(status->m_hp, dmg);
    if(status->m_hp == 0)
    {
        if (status->m_player == 1)
        {
            if (status->m_card->m_type == CardType::assault)
            {
                fd->inc_counter(QuestType::faction_assault_card_kill, status->m_card->m_faction);
            }
            fd->inc_counter(QuestType::type_card_kill, status->m_card->m_type);
        }
        _DEBUG_MSG(1, "%s dies\n", status_description(status).c_str());
        if(status->m_card->m_type != CardType::commander)
        {
            fd->killed_units.push_back(status);
        }
        if (status->m_player == 0 && fd->players[0]->deck->vip_cards.count(status->m_card->m_id))
        {
            fd->players[0]->commander.m_hp = 0;
            fd->end = true;
        }
    }
}

inline bool is_it_dead(CardStatus& c)
{
    if(c.m_hp == 0) // yes it is
    {
        _DEBUG_MSG(1, "Dead and removed: %s\n", status_description(&c).c_str());
        return(true);
    }
    else { return(false); } // nope still kickin'
}
inline void remove_dead(Storage<CardStatus>& storage)
{
    storage.remove(is_it_dead);
}
inline void add_hp(Field* fd, CardStatus* target, unsigned v)
{
    target->m_hp = std::min(target->m_hp + v, target->m_max_hp);
}
void cooldown_skills(CardStatus * status)
{
    for (const auto & ss : status->m_card->m_skills)
    {
        if (status->m_skill_cd[ss.id] > 0)
        {
            _DEBUG_MSG(2, "%s reduces timer (%u) of skill %s\n", status_description(status).c_str(), status->m_skill_cd[ss.id], skill_names[ss.id].c_str());
            -- status->m_skill_cd[ss.id];
        }
    }
}
void turn_start_phase(Field* fd)
{
    // Active player's commander card:
    cooldown_skills(&fd->tap->commander);
    // Active player's assault cards:
    // update index
    // reduce delay; reduce skill cooldown
    {
        auto& assaults(fd->tap->assaults);
        for(unsigned index(0), end(assaults.size());
            index < end;
            ++index)
        {
            CardStatus * status = &assaults[index];
            status->m_index = index;
            if(status->m_delay > 0)
            {
                _DEBUG_MSG(1, "%s reduces its timer\n", status_description(status).c_str());
                -- status->m_delay;
                if (status->m_delay == 0)
                {
                    check_and_perform_valor(fd, status);
                }
            }
            else
            {
                cooldown_skills(status);
            }
        }
    }
    // Active player's structure cards:
    // update index
    // reduce delay; reduce skill cooldown
    {
        auto& structures(fd->tap->structures);
        for(unsigned index(0), end(structures.size());
            index < end;
            ++index)
        {
            CardStatus * status = &structures[index];
            status->m_index = index;
            if(status->m_delay > 0)
            {
                _DEBUG_MSG(1, "%s reduces its timer\n", status_description(status).c_str());
                --status->m_delay;
            }
            else
            {
                cooldown_skills(status);
            }
        }
    }
    // Defending player's assault cards:
    // update index
    {
        auto& assaults(fd->tip->assaults);
        for(unsigned index(0), end(assaults.size());
            index < end;
            ++index)
        {
            CardStatus& status(assaults[index]);
            status.m_index = index;
        }
    }
    // Defending player's structure cards:
    // update index
    {
        auto& structures(fd->tip->structures);
        for(unsigned index(0), end(structures.size());
            index < end;
            ++index)
        {
            CardStatus& status(structures[index]);
            status.m_index = index;
        }
    }
}
void turn_end_phase(Field* fd)
{
    // Inactive player's assault cards:
    {
        auto& assaults(fd->tip->assaults);
        for(unsigned index(0), end(assaults.size());
            index < end;
            ++index)
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
        }
    }
    // Inactive player's structure cards:
    {
        auto& structures(fd->tip->structures);
        for(unsigned index(0), end(structures.size());
            index < end;
            ++index)
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
        for(unsigned index(0), end(assaults.size());
            index < end;
            ++index)
        {
            CardStatus& status(assaults[index]);
            if (status.m_hp <= 0)
            {
                continue;
            }
            unsigned refresh_value = status.skill(refresh);
            if (refresh_value > 0 && skill_check<refresh>(fd, &status, nullptr))
            {
                _DEBUG_MSG(1, "%s refreshes %u health\n", status_description(&status).c_str(), refresh_value);
                add_hp(fd, &status, refresh_value);
            }
            if (status.m_poisoned > 0)
            {
                unsigned poison_dmg = safe_minus(status.m_poisoned + status.m_enfeebled, status.protected_value());
                if (poison_dmg > 0)
                {
                    if (status.m_player == 1)
                    {
                        fd->inc_counter(QuestType::skill_damage, poison, 0, poison_dmg);
                    }
                    _DEBUG_MSG(1, "%s takes poison damage %u\n", status_description(&status).c_str(), poison_dmg);
                    remove_hp(fd, &status, poison_dmg);  // simultaneous
                }
            }
            // end of the opponent's next turn for enemy units
            status.m_jammed = false;
            status.m_rallied = 0;
            status.m_sundered = false;
            status.m_weakened = 0;
            status.m_inhibited = 0;
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
// Counter damage dealt to the attacker (att) by defender (def)
// pre-condition: only valid if m_card->m_counter > 0
inline unsigned counter_damage(Field* fd, CardStatus* att, CardStatus* def)
{
    assert(att->m_card->m_type == CardType::assault);
    return(safe_minus(def->skill(counter) + att->m_enfeebled, att->protected_value()));
}
inline CardStatus* select_first_enemy_wall(Field* fd)
{
    for(unsigned i(0); i < fd->tip->structures.size(); ++i)
    {
        CardStatus& c(fd->tip->structures[i]);
        if(c.has_skill(wall) && c.m_hp > 0 && skill_check<wall>(fd, &c, nullptr))
        {
            return(&c);
        }
    }
    return(nullptr);
}

inline bool alive_assault(Storage<CardStatus>& assaults, unsigned index)
{
    return(assaults.size() > index && assaults[index].m_hp > 0);
}

void remove_commander_hp(Field* fd, CardStatus& status, unsigned dmg, bool count_points)
{
    //assert(status.m_hp > 0);
    assert(status.m_card->m_type == CardType::commander);
    _DEBUG_MSG(2, "%s takes %u damage\n", status_description(&status).c_str(), dmg);
    status.m_hp = safe_minus(status.m_hp, dmg);
    if(status.m_hp == 0)
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
        unsigned pre_modifier_dmg = attack_power(att_status);

        // Evaluation order:
        // modify damage
        // deal damage
        // assaults only: (poison)
        // counter, berserk
        // assaults only: (leech if still alive)

        modify_attack_damage<def_cardtype>(pre_modifier_dmg);
        if (att_dmg == 0) { return 0; }

        attack_damage<def_cardtype>();
        if(__builtin_expect(fd->end, false)) { return att_dmg; }
        damage_dependant_pre_oa<def_cardtype>();

        if (att_status->m_hp > 0 && def_status->has_skill(counter) && skill_check<counter>(fd, def_status, att_status))
        {
            // perform_skill_counter
            unsigned counter_dmg(counter_damage(fd, att_status, def_status));
            if (def_status->m_player == 0)
            {
                fd->inc_counter(QuestType::skill_use, counter);
                fd->inc_counter(QuestType::skill_damage, counter, 0, counter_dmg);
            }
            _DEBUG_MSG(1, "%s takes %u counter damage from %s\n", status_description(att_status).c_str(), counter_dmg, status_description(def_status).c_str());
            remove_hp(fd, att_status, counter_dmg);
            prepend_on_death(fd);
            resolve_skill(fd);
            if (def_cardtype == CardType::assault && def_status->m_hp > 0 && fd->bg_effects.count(counterflux))
            {
                unsigned flux_denominator = fd->bg_effects.at(counterflux) ? fd->bg_effects.at(counterflux) : 4;
                unsigned flux_value = (def_status->skill(counter) - 1) / flux_denominator + 1;
                _DEBUG_MSG(1, "Counterflux: %s heals itself and berserks for %u\n", status_description(def_status).c_str(), flux_value);
                add_hp(fd, def_status, flux_value);
                if (! def_status->m_sundered)
                { def_status->m_attack += flux_value; }
            }
        }
        unsigned corrosive_value = def_status->skill(corrosive);
        if (att_status->m_hp > 0 && corrosive_value > att_status->m_corroded_rate && skill_check<corrosive>(fd, def_status, att_status))
        {
            // perform_skill_corrosive
            _DEBUG_MSG(1, "%s corrodes %s by %u\n", status_description(def_status).c_str(), status_description(att_status).c_str(), corrosive_value);
            att_status->m_corroded_rate = corrosive_value;
        }
        unsigned berserk_value = att_status->skill(berserk);
        if (att_status->m_hp > 0 && ! att_status->m_sundered && berserk_value > 0 && skill_check<berserk>(fd, att_status, nullptr))
        {
            // perform_skill_berserk
            att_status->m_attack += berserk_value;
            if (att_status->m_player == 0)
            {
                fd->inc_counter(QuestType::skill_use, berserk);
            }
            if (fd->bg_effects.count(enduringrage))
            {
                unsigned bge_denominator = fd->bg_effects.at(enduringrage) ? fd->bg_effects.at(enduringrage) : 2;
                unsigned bge_value = (berserk_value - 1) / bge_denominator + 1;
                _DEBUG_MSG(1, "EnduringRage: %s heals and protects itself for %u\n", status_description(att_status).c_str(), bge_value);
                add_hp(fd, att_status, bge_value);
                att_status->m_protected += bge_value;
            }
        }
        do_leech<def_cardtype>();
        unsigned valor_value = att_status->skill(valor);
        if (valor_value > 0 && ! att_status->m_sundered && fd->bg_effects.count(heroism) && def_cardtype == CardType::assault && def_status->m_hp <= 0)
        {
            _DEBUG_MSG(1, "Heroism: %s gain %u attack\n", status_description(att_status).c_str(), valor_value);
            att_status->m_attack += valor_value;
        }
        return att_dmg;
    }

    template<enum CardType::CardType>
    void modify_attack_damage(unsigned pre_modifier_dmg)
    {
        assert(att_status->m_card->m_type == CardType::assault);
        att_dmg = pre_modifier_dmg;
        if (att_dmg == 0)
        { return; }
        std::string desc;
        unsigned legion_value = 0;
        if (! att_status->m_sundered)
        {
            // enhance damage
            unsigned legion_base = att_status->skill(legion);
            if (legion_base > 0)
            {
                auto & assaults = fd->tap->assaults;
                legion_value += att_status->m_index > 0 && assaults[att_status->m_index - 1].m_hp > 0 && assaults[att_status->m_index - 1].m_faction == att_status->m_faction;
                legion_value += att_status->m_index + 1 < assaults.size() && assaults[att_status->m_index + 1].m_hp > 0 && assaults[att_status->m_index + 1].m_faction == att_status->m_faction;
                if (legion_value > 0 && skill_check<legion>(fd, att_status, nullptr))
                {
                    legion_value *= legion_base;
                    if (debug_print > 0) { desc += "+" + to_string(legion_value) + "(legion)"; }
                    att_dmg += legion_value;
                }
            }
            unsigned rupture_value = att_status->skill(rupture);
            if (rupture_value > 0)
            {
                if (debug_print > 0) { desc += "+" + to_string(rupture_value) + "(rupture)"; }
                att_dmg += rupture_value;
            }
            unsigned venom_value = att_status->skill(venom);
            if (venom_value > 0 && def_status->m_poisoned > 0)
            {
                if (debug_print > 0) { desc += "+" + to_string(venom_value) + "(venom)"; }
                att_dmg += venom_value;
            }
            if (fd->bloodlust_value > 0)
            {
                if (debug_print > 0) { desc += "+" + to_string(fd->bloodlust_value) + "(bloodlust)"; }
                att_dmg += fd->bloodlust_value;
            }
            if(def_status->m_enfeebled > 0)
            {
                if(debug_print > 0) { desc += "+" + to_string(def_status->m_enfeebled) + "(enfeebled)"; }
                att_dmg += def_status->m_enfeebled;
            }
        }
        // prevent damage
        std::string reduced_desc;
        unsigned reduced_dmg(0);
        unsigned armor_value = def_status->skill(armor);
        if (def_status->m_card->m_type == CardType::assault && fd->bg_effects.count(fortification))
        {
            for (auto && adj_status: fd->adjacent_assaults(def_status))
            {
                armor_value = std::max(armor_value, adj_status->skill(armor));
            }
        }
        if(armor_value > 0)
        {
            if(debug_print > 0) { reduced_desc += to_string(armor_value) + "(armor)"; }
            reduced_dmg += armor_value;
        }
        if(def_status->protected_value() > 0)
        {
            if(debug_print > 0) { reduced_desc += (reduced_desc.empty() ? "" : "+") + to_string(def_status->protected_value()) + "(protected)"; }
            reduced_dmg += def_status->protected_value();
        }
        unsigned pierce_value = att_status->skill(pierce) + att_status->skill(rupture);
        if (reduced_dmg > 0 && pierce_value > 0)
        {
            if (debug_print > 0) { reduced_desc += "-" + to_string(pierce_value) + "(pierce)"; }
            reduced_dmg = safe_minus(reduced_dmg, pierce_value);
        }
        att_dmg = safe_minus(att_dmg, reduced_dmg);
        if(debug_print > 0)
        {
            if(!reduced_desc.empty()) { desc += "-[" + reduced_desc + "]"; }
            if(!desc.empty()) { desc += "=" + to_string(att_dmg); }
            _DEBUG_MSG(1, "%s attacks %s for %u%s damage\n", status_description(att_status).c_str(), status_description(def_status).c_str(), pre_modifier_dmg, desc.c_str());
        }
        if (legion_value > 0 && can_be_healed(att_status) && fd->bg_effects.count(brigade))
        {
            _DEBUG_MSG(1, "Brigade: %s heals itself for %u\n", status_description(att_status).c_str(), legion_value);
            add_hp(fd, att_status, legion_value);
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
    void damage_dependant_pre_oa() {}

    template<enum CardType::CardType>
    void do_leech() {}
};

template<>
void PerformAttack::attack_damage<CardType::commander>()
{
    remove_commander_hp(fd, *def_status, att_dmg, true);
}

template<>
void PerformAttack::damage_dependant_pre_oa<CardType::assault>()
{
    unsigned poison_value = std::max(att_status->skill(poison), att_status->skill(venom));
    if (poison_value > def_status->m_poisoned && skill_check<poison>(fd, att_status, def_status))
    {
        // perform_skill_poison
        if (att_status->m_player == 0)
        {
            fd->inc_counter(QuestType::skill_use, poison);
        }
        _DEBUG_MSG(1, "%s poisons %s by %u\n", status_description(att_status).c_str(), status_description(def_status).c_str(), poison_value);
        def_status->m_poisoned = poison_value;
    }
    unsigned inhibit_value = att_status->skill(inhibit);
    if (inhibit_value > def_status->m_inhibited && skill_check<inhibit>(fd, att_status, def_status))
    {
        // perform_skill_inhibit
        _DEBUG_MSG(1, "%s inhibits %s by %u\n", status_description(att_status).c_str(), status_description(def_status).c_str(), inhibit_value);
        def_status->m_inhibited = inhibit_value;
    }
}

template<>
void PerformAttack::do_leech<CardType::assault>()
{
    unsigned leech_value = std::min(att_dmg, att_status->skill(leech));
    if(leech_value > 0 && skill_check<leech>(fd, att_status, nullptr))
    {
        if (att_status->m_player == 0)
        {
            fd->inc_counter(QuestType::skill_use, leech);
        }
        _DEBUG_MSG(1, "%s leeches %u health\n", status_description(att_status).c_str(), leech_value);
        add_hp(fd, att_status, leech_value);
    }
}

// General attack phase by the currently evaluated assault, taking into accounts exotic stuff such as flurry, etc.
unsigned attack_commander(Field* fd, CardStatus* att_status)
{
    CardStatus* def_status{select_first_enemy_wall(fd)}; // defending wall
    if(def_status != nullptr)
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

    if (attack_power(att_status) == 0)
    {
        { // Bizarre behavior: Swipe activates if attack is corroded to 0
            CardStatus * def_status = &fd->tip->assaults[fd->current_ci];
            unsigned swipe_value = att_status->skill(swipe);
            if (alive_assault(def_assaults, fd->current_ci) && att_status->m_attack + att_status->m_rallied > att_status->m_weakened && swipe_value > 0)
            {
                for (auto && adj_status: fd->adjacent_assaults(def_status))
                {
                    unsigned swipe_dmg = safe_minus(swipe_value + def_status->m_enfeebled, def_status->protected_value());
                    _DEBUG_MSG(1, "%s swipes %s for %u damage\n", status_description(att_status).c_str(), status_description(adj_status).c_str(), swipe_dmg);
                    remove_hp(fd, adj_status, swipe_dmg);
                }
                prepend_on_death(fd);
                resolve_skill(fd);
            }
        }
        return false;
    }

    unsigned att_dmg = 0;
    if (alive_assault(def_assaults, fd->current_ci))
    {
        CardStatus * def_status = &fd->tip->assaults[fd->current_ci];
        att_dmg = PerformAttack{fd, att_status, def_status}.op<CardType::assault>();
        unsigned swipe_value = att_status->skill(swipe);
        if (att_dmg > 0 && swipe_value > 0)
        {
            for (auto && adj_status: fd->adjacent_assaults(def_status))
            {
                unsigned swipe_dmg = safe_minus(swipe_value + def_status->m_enfeebled, def_status->protected_value());
                _DEBUG_MSG(1, "%s swipes %s for %u damage\n", status_description(att_status).c_str(), status_description(adj_status).c_str(), swipe_dmg);
                remove_hp(fd, adj_status, swipe_dmg);
            }
            prepend_on_death(fd);
            resolve_skill(fd);
        }
    }
    else
    {
        // might be blocked by walls
        att_dmg = attack_commander(fd, att_status);
    }

    if (att_dmg > 0 && !fd->assault_bloodlusted && fd->bg_effects.count(bloodlust))
    {
        fd->bloodlust_value += fd->bg_effects.at(bloodlust);
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
{ return dst->m_hp > 0; }

template<>
inline bool skill_predicate<enhance>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    return dst->has_skill(s.s) && (!(BEGIN_ACTIVATION < s.s && s.s < END_ACTIVATION) || is_active(dst));
}

/*
 * Target active units: Activation (Mortar)
 * Target everything: Defensive (Refresh), Combat-Modifier (Rupture, Venom)
 */
template<>
inline bool skill_predicate<evolve>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    return dst->has_skill(s.s) && !dst->has_skill(s.s2) && (!(BEGIN_ACTIVATION < s.s2 && s.s2 < END_ACTIVATION) || is_active(dst));
}

template<>
inline bool skill_predicate<mend>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{ return(can_be_healed(dst)); }

template<>
inline bool skill_predicate<heal>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{ return(can_be_healed(dst)); }

template<>
inline bool skill_predicate<jam>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    return is_active_next_turn(dst);
}

template<>
inline bool skill_predicate<overload>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    if (dst->m_overloaded || has_attacked(dst) || !is_active(dst))
    {
        return false;
    }
    bool has_inhibited_unit = false;
    for (const auto & c: fd->players[dst->m_player]->assaults.m_indirect)
    {
        if (c->m_hp > 0 && c->m_inhibited)
        {
            has_inhibited_unit = true;
            break;
        }
    }
    for (const auto & ss: dst->m_card->m_skills)
    {
        if (dst->m_skill_cd[ss.id] > 0)
        {
            continue;
        }
        Skill evolved_skill_id = static_cast<Skill>(ss.id + dst->m_evolved_skill_offset[ss.id]);
        if (BEGIN_ACTIVATION_HARMFUL < evolved_skill_id && evolved_skill_id < END_ACTIVATION_HARMFUL)
        {
            return true;
        }
        if (has_inhibited_unit && (evolved_skill_id == heal || evolved_skill_id == protect || evolved_skill_id == rally))
        {
            return true;
        }
    }
    return false;
}

template<>
inline bool skill_predicate<rally>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    return ! dst->m_sundered && (fd->tapi == dst->m_player ? is_active(dst) && !has_attacked(dst) : is_active_next_turn(dst));
}

template<>
inline bool skill_predicate<rush>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    return ! src->m_rush_attempted && dst->m_delay >= (src->m_card->m_type == CardType::assault && dst->m_index < src->m_index ? 2u : 1u);
}

template<>
inline bool skill_predicate<sunder>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    return attack_power(dst) > 0 && is_active_next_turn(dst);
}

template<>
inline bool skill_predicate<weaken>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    return attack_power(dst) > 0 && is_active_next_turn(dst);
}

template<unsigned skill_id>
inline void perform_skill(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{ assert(false); }

template<>
inline void perform_skill<enfeeble>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_enfeebled += s.x;
}

template<>
inline void perform_skill<enhance>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_enhanced_value[s.s + dst->m_primary_skill_offset[s.s]] += s.x;
}

template<>
inline void perform_skill<evolve>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    auto primary_s1 = dst->m_primary_skill_offset[s.s] + s.s;
    auto primary_s2 = dst->m_primary_skill_offset[s.s2] + s.s2;
    dst->m_primary_skill_offset[s.s] = primary_s2 - s.s;
    dst->m_primary_skill_offset[s.s2] = primary_s1 - s.s2;
    dst->m_evolved_skill_offset[primary_s1] = s.s2 - primary_s1;
    dst->m_evolved_skill_offset[primary_s2] = s.s - primary_s2;
}

template<>
inline void perform_skill<heal>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    add_hp(fd, dst, s.x);
    if (src->m_card->m_type == CardType::assault && fd->bg_effects.count(zealotspreservation))
    {
        unsigned bge_value = (s.x + 1) / 2;
        _DEBUG_MSG(1, "Zealot's Preservation: %s Protect %u on %s\n",
            status_description(src).c_str(), bge_value, status_description(dst).c_str());
        dst->m_protected += bge_value;
    }
}

template<>
inline void perform_skill<jam>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_jammed = true;
}

template<>
inline void perform_skill<mend>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    add_hp(fd, dst, s.x);
}

template<>
inline void perform_skill<mortar>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    if (dst->m_card->m_type == CardType::structure)
    {
        remove_hp(fd, dst, s.x);
    }
    else
    {
        unsigned strike_dmg = safe_minus((s.x + 1) / 2 + dst->m_enfeebled, src->m_overloaded ? 0 : dst->protected_value());
        remove_hp(fd, dst, strike_dmg);
    }
}

template<>
inline void perform_skill<overload>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_overloaded = true;
}

template<>
inline void perform_skill<protect>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_protected += s.x;
}

template<>
inline void perform_skill<rally>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_rallied += s.x;
}

template<>
inline void perform_skill<rush>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_delay -= 1;
    if (dst->m_delay == 0)
    {
        check_and_perform_valor(fd, dst);
    }
}

template<>
inline void perform_skill<siege>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    remove_hp(fd, dst, s.x);
}

template<>
inline void perform_skill<strike>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    unsigned strike_dmg = safe_minus(s.x + dst->m_enfeebled, src->m_overloaded ? 0 : dst->protected_value());
    remove_hp(fd, dst, strike_dmg);
}

template<>
inline void perform_skill<sunder>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_sundered = true;
    dst->m_weakened += std::min(s.x, attack_power(dst));
}

template<>
inline void perform_skill<weaken>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_weakened += std::min(s.x, attack_power(dst));
}

template<unsigned skill_id>
inline unsigned select_fast(Field* fd, CardStatus* src, const std::vector<CardStatus*>& cards, const SkillSpec& s)
{
    if (s.y == allfactions || fd->bg_effects.count(metamorphosis))
    {
        return(fd->make_selection_array(cards.begin(), cards.end(), [fd, src, s](CardStatus* c){return(skill_predicate<skill_id>(fd, src, c, s));}));
    }
    else
    {
        return(fd->make_selection_array(cards.begin(), cards.end(), [fd, src, s](CardStatus* c){return((c->m_faction == s.y || c->m_faction == progenitor) && skill_predicate<skill_id>(fd, src, c, s));}));
    }
}

template<>
inline unsigned select_fast<mend>(Field* fd, CardStatus* src, const std::vector<CardStatus*>& cards, const SkillSpec& s)
{
    fd->selection_array.clear();
    for (auto && adj_status: fd->adjacent_assaults(src))
    {
        if (skill_predicate<mend>(fd, src, adj_status, s))
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

template<> std::vector<CardStatus*>& skill_targets<enfeeble>(Field* fd, CardStatus* src)
{ return(skill_targets_hostile_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<enhance>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<evolve>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<heal>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<jam>(Field* fd, CardStatus* src)
{ return(skill_targets_hostile_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<mend>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<overload>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<protect>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<rally>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<rush>(Field* fd, CardStatus* src)
{ return(skill_targets_allied_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<siege>(Field* fd, CardStatus* src)
{ return(skill_targets_hostile_structure(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<strike>(Field* fd, CardStatus* src)
{ return(skill_targets_hostile_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<sunder>(Field* fd, CardStatus* src)
{ return(skill_targets_hostile_assault(fd, src)); }

template<> std::vector<CardStatus*>& skill_targets<weaken>(Field* fd, CardStatus* src)
{ return(skill_targets_hostile_assault(fd, src)); }

template<Skill skill_id>
bool check_and_perform_skill(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s, bool is_evadable, bool & has_counted_quest)
{
    if(skill_check<skill_id>(fd, src, dst))
    {
        if (src->m_player == 0 && ! has_counted_quest)
        {
            fd->inc_counter(QuestType::skill_use, skill_id, dst->m_card->m_id);
            has_counted_quest = true;
        }
        if (is_evadable &&
                dst->m_evaded < dst->skill(evade) &&
                skill_check<evade>(fd, dst, src))
        {
            ++ dst->m_evaded;
            _DEBUG_MSG(1, "%s %s on %s but it evades\n", status_description(src).c_str(), skill_short_description(s).c_str(), status_description(dst).c_str());
            return(false);
        }
        _DEBUG_MSG(1, "%s %s on %s\n", status_description(src).c_str(), skill_short_description(s).c_str(), status_description(dst).c_str());
        perform_skill<skill_id>(fd, src, dst, s);
        if (s.c > 0)
        {
            src->m_skill_cd[skill_id] = s.c;
        }
        return(true);
    }
    _DEBUG_MSG(1, "(CANCELLED) %s %s on %s\n", status_description(src).c_str(), skill_short_description(s).c_str(), status_description(dst).c_str());
    return(false);
}

bool check_and_perform_valor(Field* fd, CardStatus* src)
{
    unsigned valor_value = src->skill(valor);
    if (valor_value > 0 && ! src->m_sundered && skill_check<valor>(fd, src, nullptr))
    {
        unsigned opponent_player = opponent(src->m_player);
        const CardStatus * dst = fd->players[opponent_player]->assaults.size() > src->m_index ?
            &fd->players[opponent_player]->assaults[src->m_index] :
            nullptr;
        if (dst == nullptr || dst->m_hp <= 0)
        {
            _DEBUG_MSG(1, "%s loses Valor (no blocker)\n", status_description(src).c_str());
            return false;
        }
        else if (attack_power(dst) <= attack_power(src))
        {
            _DEBUG_MSG(1, "%s loses Valor (weak blocker %s)\n", status_description(src).c_str(), status_description(dst).c_str());
            return false;
        }
        if (src->m_player == 0)
        {
            fd->inc_counter(QuestType::skill_use, valor);
        }
        _DEBUG_MSG(1, "%s activates Valor %u\n", status_description(src).c_str(), valor_value);
        src->m_attack += valor_value;
        return true;
    }
    return false;
}

template<Skill skill_id>
size_t select_targets(Field* fd, CardStatus* src, const SkillSpec& s)
{
    std::vector<CardStatus*>& cards(skill_targets<skill_id>(fd, src));
    size_t n_candidates = select_fast<skill_id>(fd, src, cards, s);
    if (n_candidates == 0)
    {
        return n_candidates;
    }
    _DEBUG_SELECTION("%s", skill_names[skill_id].c_str());
    unsigned n_targets = s.n > 0 ? s.n : 1;
    if (s.all || n_targets >= n_candidates || skill_id == mend)  // target all or mend
    {
        return n_candidates;
    }
    for (unsigned i = 0; i < n_targets; ++i)
    {
        std::swap(fd->selection_array[i], fd->selection_array[fd->rand(i, n_candidates - 1)]);
    }
    fd->selection_array.resize(n_targets);
    if (n_targets > 1)
    {
        std::sort(fd->selection_array.begin(), fd->selection_array.end(), [](const CardStatus * a, const CardStatus * b) { return a->m_index < b->m_index; });
    }
    return n_targets;
}

template<>
size_t select_targets<mortar>(Field* fd, CardStatus* src, const SkillSpec& s)
{
    size_t n_candidates = select_fast<siege>(fd, src, skill_targets<siege>(fd, src), s);
    if (n_candidates == 0)
    {
        n_candidates = select_fast<strike>(fd, src, skill_targets<strike>(fd, src), s);
        if (n_candidates == 0)
        {
            return n_candidates;
        }
    }
    _DEBUG_SELECTION("%s", skill_names[mortar].c_str());
    unsigned n_targets = s.n > 0 ? s.n : 1;
    if (s.all || n_targets >= n_candidates)
    {
        return n_candidates;
    }
    for (unsigned i = 0; i < n_targets; ++i)
    {
        std::swap(fd->selection_array[i], fd->selection_array[fd->rand(i, n_candidates - 1)]);
    }
    fd->selection_array.resize(n_targets);
    if (n_targets > 1)
    {
        std::sort(fd->selection_array.begin(), fd->selection_array.end(), [](const CardStatus * a, const CardStatus * b) { return a->m_index < b->m_index; });
    }
    return n_targets;
}

template<Skill skill_id>
void perform_targetted_allied_fast(Field* fd, CardStatus* src, const SkillSpec& s)
{
    select_targets<skill_id>(fd, src, s);
    unsigned num_inhibited = 0;
    bool has_counted_quest = false;
    for (CardStatus * dst: fd->selection_array)
    {
        if (dst->m_inhibited > 0 && !src->m_overloaded)
        {
            _DEBUG_MSG(1, "%s %s on %s but it is inhibited\n", status_description(src).c_str(), skill_short_description(s).c_str(), status_description(dst).c_str());
            -- dst->m_inhibited;
            ++ num_inhibited;
            continue;
        }
        check_and_perform_skill<skill_id>(fd, src, dst, s, false, has_counted_quest);
    }
    if (num_inhibited > 0 && fd->bg_effects.count(divert))
    {
        SkillSpec diverted_ss = s;
        diverted_ss.y = allfactions;
        diverted_ss.n = 1;
        diverted_ss.all = false;
        for (unsigned i = 0; i < num_inhibited; ++ i)
        {
            select_targets<skill_id>(fd, &fd->tip->commander, diverted_ss);
            for (CardStatus * dst: fd->selection_array)
            {
                if (dst->m_inhibited > 0)
                {
                    _DEBUG_MSG(1, "%s %s (Diverted) on %s but it is inhibited\n", status_description(src).c_str(), skill_short_description(diverted_ss).c_str(), status_description(dst).c_str());
                    -- dst->m_inhibited;
                    continue;
                }
                _DEBUG_MSG(1, "%s %s (Diverted) on %s\n", status_description(src).c_str(), skill_short_description(diverted_ss).c_str(), status_description(dst).c_str());
                perform_skill<skill_id>(fd, src, dst, diverted_ss);
            }
        }
    }
}

void perform_targetted_allied_fast_rush(Field* fd, CardStatus* src, const SkillSpec& s)
{
    if (src->m_card->m_type == CardType::commander)
    {  // BGE skills are casted as by commander
        perform_targetted_allied_fast<rush>(fd, src, s);
        return;
    }
    if (src->m_rush_attempted)
    {
        _DEBUG_MSG(2, "%s does not check Rush again.\n", status_description(src).c_str());
        return;
    }
    _DEBUG_MSG(1, "%s attempts to activate Rush.\n", status_description(src).c_str());
    perform_targetted_allied_fast<rush>(fd, src, s);
    src->m_rush_attempted = true;
}

template<Skill skill_id>
void perform_targetted_hostile_fast(Field* fd, CardStatus* src, const SkillSpec& s)
{
    select_targets<skill_id>(fd, src, s);
    bool has_counted_quest = false;
    std::vector<CardStatus *> paybackers;
    if (fd->bg_effects.count(turningtides) && skill_id == weaken)
    {
        unsigned turningtides_value = 0;
        for (CardStatus * dst: fd->selection_array)
        {
            unsigned old_attack = attack_power(dst);
            if (check_and_perform_skill<skill_id>(fd, src, dst, s, ! src->m_overloaded, has_counted_quest))
            {
                turningtides_value = std::max(turningtides_value, safe_minus(old_attack, attack_power(dst)));
                // Payback
                if(dst->m_paybacked < dst->skill(payback) && skill_check<payback>(fd, dst, src) &&
                        skill_predicate<skill_id>(fd, src, src, s) && skill_check<skill_id>(fd, src, dst))
                {
                    paybackers.push_back(dst);
                }
            }
        }
        if (turningtides_value > 0)
        {
            SkillSpec ss_rally{rally, turningtides_value, allfactions, 0, 0, no_skill, no_skill, s.all,};
            _DEBUG_MSG(1, "TurningTides %u!\n", turningtides_value);
            perform_targetted_allied_fast<rally>(fd, &fd->players[src->m_player]->commander, ss_rally);
        }
        for (CardStatus * pb_status: paybackers)
        {
            ++ pb_status->m_paybacked;
            unsigned old_attack = attack_power(src);
            _DEBUG_MSG(1, "%s Payback %s on %s\n", status_description(pb_status).c_str(), skill_short_description(s).c_str(), status_description(src).c_str());
            perform_skill<skill_id>(fd, pb_status, src, s);
            turningtides_value = std::max(turningtides_value, safe_minus(old_attack, attack_power(src)));
            if (turningtides_value > 0)
            {
                SkillSpec ss_rally{rally, turningtides_value, allfactions, 0, 0, no_skill, no_skill, false,};
                _DEBUG_MSG(1, "Paybacked TurningTides %u!\n", turningtides_value);
                perform_targetted_allied_fast<rally>(fd, &fd->players[pb_status->m_player]->commander, ss_rally);
            }
        }
        return;
    }
    for (CardStatus * dst: fd->selection_array)
    {
        if (check_and_perform_skill<skill_id>(fd, src, dst, s, ! src->m_overloaded, has_counted_quest))
        {
            // Payback
            if(dst->m_paybacked < dst->skill(payback) && skill_check<payback>(fd, dst, src) &&
                    skill_predicate<skill_id>(fd, src, src, s) && skill_check<skill_id>(fd, src, dst))
            {
                paybackers.push_back(dst);
            }
        }
    }
    prepend_on_death(fd);  // skills
    for (CardStatus * pb_status: paybackers)
    {
        if (src->m_hp == 0)
        {
            _DEBUG_MSG(1, "(CANCELLED: src unit dead) %s Payback %s on %s\n",
                status_description(pb_status).c_str(), skill_short_description(s).c_str(), status_description(src).c_str());
            continue;
        }
        ++ pb_status->m_paybacked;
        _DEBUG_MSG(1, "%s Payback %s on %s\n", status_description(pb_status).c_str(), skill_short_description(s).c_str(), status_description(src).c_str());
        perform_skill<skill_id>(fd, pb_status, src, s);
    }
    prepend_on_death(fd);  // paybacked skills
}

//------------------------------------------------------------------------------
Results<uint64_t> play(Field* fd)
{
    fd->players[0]->commander.m_player = 0;
    fd->players[1]->commander.m_player = 1;
    fd->tapi = fd->gamemode == surge ? 1 : 0;
    fd->tipi = opponent(fd->tapi);
    fd->tap = fd->players[fd->tapi];
    fd->tip = fd->players[fd->tipi];
    fd->end = false;

    // Play fortresses
    for (unsigned _ = 0; _ < 2; ++ _)
    {
        for (const Card* played_card: fd->tap->deck->shuffled_forts)
        {
            PlayCard(played_card, fd).op<CardType::structure>();
        }
        std::swap(fd->tapi, fd->tipi);
        std::swap(fd->tap, fd->tip);
    }

    while(__builtin_expect(fd->turn <= turn_limit && !fd->end, true))
    {
        fd->current_phase = Field::playcard_phase;
        // Initialize stuff, remove dead cards
        _DEBUG_MSG(1, "------------------------------------------------------------------------\n"
                "TURN %u begins for %s\n", fd->turn, status_description(&fd->tap->commander).c_str());
        turn_start_phase(fd);

        // Play a card
        const Card* played_card(fd->tap->deck->next());
        if(played_card)
        {
            // Evaluate skill Allegiance
            for (CardStatus * status : fd->tap->assaults.m_indirect)
            {
                unsigned allegiance_value = status->skill(allegiance);
                assert(status->m_card);
                if (allegiance_value > 0 && status->m_hp > 0 && status->m_card->m_faction == played_card->m_faction)
                {
                    _DEBUG_MSG(1, "%s activates Allegiance %u\n", status_description(status).c_str(), allegiance_value);
                    if (! status->m_sundered)
                    { status->m_attack += allegiance_value; }
                    status->m_max_hp += allegiance_value;
                    status->m_hp += allegiance_value;
                }
            }
            // End Evaluate skill Allegiance
            switch(played_card->m_type)
            {
            case CardType::assault:
                PlayCard(played_card, fd).op<CardType::assault>();
                break;
            case CardType::structure:
                PlayCard(played_card, fd).op<CardType::structure>();
                break;
            case CardType::commander:
            case CardType::num_cardtypes:
                _DEBUG_MSG(0, "Unknown card type: #%u %s: %u\n", played_card->m_id, card_description(fd->cards, played_card).c_str(), played_card->m_type);
                assert(false);
                break;
            }
        }
        if(__builtin_expect(fd->end, false)) { break; }

        // Evaluate Heroism BGE skills
        if (fd->bg_effects.count(heroism))
        {
            for (CardStatus * dst: fd->tap->assaults.m_indirect)
            {
                unsigned bge_value = (dst->skill(valor) + 1) / 2;
                if (bge_value <= 0)
                { continue; }
                SkillSpec ss_protect{protect, bge_value, allfactions, 0, 0, no_skill, no_skill, false,};
                if (dst->m_inhibited > 0)
                {
                    _DEBUG_MSG(1, "Heroism: %s on %s but it is inhibited\n", skill_short_description(ss_protect).c_str(), status_description(dst).c_str());
                    -- dst->m_inhibited;
                    if (fd->bg_effects.count(divert))
                    {
                        SkillSpec diverted_ss = ss_protect;
                        diverted_ss.y = allfactions;
                        diverted_ss.n = 1;
                        diverted_ss.all = false;
                        // for (unsigned i = 0; i < num_inhibited; ++ i)
                        {
                            select_targets<protect>(fd, &fd->tip->commander, diverted_ss);
                            for (CardStatus * dst: fd->selection_array)
                            {
                                if (dst->m_inhibited > 0)
                                {
                                    _DEBUG_MSG(1, "Heroism: %s (Diverted) on %s but it is inhibited\n", skill_short_description(diverted_ss).c_str(), status_description(dst).c_str());
                                    -- dst->m_inhibited;
                                    continue;
                                }
                                _DEBUG_MSG(1, "Heroism: %s (Diverted) on %s\n", skill_short_description(diverted_ss).c_str(), status_description(dst).c_str());
                                perform_skill<protect>(fd, &fd->tap->commander, dst, diverted_ss);  // XXX: the caster
                            }
                        }
                    }
                    continue;
                }
                bool has_counted_quest = false;
                check_and_perform_skill<protect>(fd, &fd->tap->commander, dst, ss_protect, false, has_counted_quest);
            }
        }

        // Evaluate activation BGE skills
        for (const auto & bg_skill: fd->bg_skills[fd->tapi])
        {
            _DEBUG_MSG(2, "Evaluating BG skill %s\n", skill_description(fd->cards, bg_skill).c_str());
            fd->skill_queue.emplace_back(&fd->tap->commander, bg_skill);
            resolve_skill(fd);
        }
        if (__builtin_expect(fd->end, false)) { break; }

        // Evaluate commander
        fd->current_phase = Field::commander_phase;
        evaluate_skills<CardType::commander>(fd, &fd->tap->commander, fd->tap->commander.m_card->m_skills);
        if(__builtin_expect(fd->end, false)) { break; }

        // Evaluate structures
        fd->current_phase = Field::structures_phase;
        for(fd->current_ci = 0; !fd->end && fd->current_ci < fd->tap->structures.size(); ++fd->current_ci)
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
        for(fd->current_ci = 0; !fd->end && fd->current_ci < fd->tap->assaults.size(); ++fd->current_ci)
        {
            // ca: current assault
            CardStatus* current_status(&fd->tap->assaults[fd->current_ci]);
            bool attacked = false;
            if (!is_active(current_status))
            {
                _DEBUG_MSG(2, "%s cannot take action.\n", status_description(current_status).c_str());
            }
            else
            {
                fd->assault_bloodlusted = false;
                evaluate_skills<CardType::assault>(fd, current_status, current_status->m_card->m_skills, &attacked);
                if (__builtin_expect(fd->end, false)) { break; }
            }
            if (current_status->m_corroded_rate > 0)
            {
                if (attacked)
                {
                    unsigned v = std::min(current_status->m_corroded_rate, attack_power(current_status));
                    _DEBUG_MSG(1, "%s loses Attack by %u.\n", status_description(current_status).c_str(), v);
                    current_status->m_corroded_weakened += v;
                }
                else
                {
                    _DEBUG_MSG(1, "%s loses Status corroded.\n", status_description(current_status).c_str());
                    current_status->m_corroded_rate = 0;
                    current_status->m_corroded_weakened = 0;
                }
            }
            current_status->m_step = CardStep::attacked;
        }
        fd->current_phase = Field::end_phase;
        turn_end_phase(fd);
        if(__builtin_expect(fd->end, false)) { break; }
        _DEBUG_MSG(1, "TURN %u ends for %s\n", fd->turn, status_description(&fd->tap->commander).c_str());
        std::swap(fd->tapi, fd->tipi);
        std::swap(fd->tap, fd->tip);
        ++fd->turn;
    }
    const auto & p = fd->players;
    unsigned raid_damage = 0;
    unsigned quest_score = 0;
    switch (fd->optimization_mode)
    {
        case OptimizationMode::raid:
            raid_damage = 15 + (std::min<unsigned>(p[1]->deck->deck_size, (fd->turn + 1) / 2) - p[1]->assaults.size() - p[1]->structures.size()) - (10 * p[1]->commander.m_hp / p[1]->commander.m_max_hp);
            break;
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
        default:
            break;
    }
    // you lose
    if(fd->players[0]->commander.m_hp == 0)
    {
        _DEBUG_MSG(1, "You lose.\n");
        switch (fd->optimization_mode)
        {
        case OptimizationMode::raid: return {0, 0, 1, raid_damage};
        case OptimizationMode::brawl: return {0, 0, 1, 5};
        case OptimizationMode::quest: return {0, 0, 1, fd->quest.must_win ? 0 : quest_score};
        default: return {0, 0, 1, 0};
        }
    }
    // you win
    if(fd->players[1]->commander.m_hp == 0)
    {
        _DEBUG_MSG(1, "You win.\n");
        switch (fd->optimization_mode)
        {
        case OptimizationMode::brawl:
            {
                unsigned brawl_score = 57
                    - (10 * (p[0]->commander.m_max_hp - p[0]->commander.m_hp) / p[0]->commander.m_max_hp)
                    + (p[0]->assaults.size() + p[0]->structures.size() + p[0]->deck->shuffled_cards.size())
                    - (p[1]->assaults.size() + p[1]->structures.size() + p[1]->deck->shuffled_cards.size())
                    - fd->turn / 4;
                return {1, 0, 0, brawl_score};
            }
        case OptimizationMode::campaign:
            {
                unsigned campaign_score = 100 - 10 * (std::min<unsigned>(p[0]->deck->cards.size(), (fd->turn + 1) / 2) - p[0]->assaults.size() - p[0]->structures.size());
                return {1, 0, 0, campaign_score};
            }
        case OptimizationMode::quest: return {1, 0, 0, fd->quest.win_score + quest_score};
        default:
            return {1, 0, 0, 100};
        }
    }
    if (fd->turn > turn_limit)
    {
        _DEBUG_MSG(1, "Stall after %u turns.\n", turn_limit);
        switch (fd->optimization_mode)
        {
        case OptimizationMode::defense: return {0, 1, 0, 100};
        case OptimizationMode::raid: return {0, 1, 0, raid_damage};
        case OptimizationMode::brawl: return {0, 1, 0, 5};
        case OptimizationMode::quest: return {0, 1, 0, fd->quest.must_win ? 0 : quest_score};
        default: return {0, 1, 0, 0};
        }
    }

    // Huh? How did we get here?
    assert(false);
    return {0, 0, 0, 0};
}
//------------------------------------------------------------------------------
void fill_skill_table()
{
    memset(skill_table, 0, sizeof skill_table);
    skill_table[mortar] = perform_targetted_hostile_fast<mortar>;
    skill_table[enfeeble] = perform_targetted_hostile_fast<enfeeble>;
    skill_table[enhance] = perform_targetted_allied_fast<enhance>;
    skill_table[evolve] = perform_targetted_allied_fast<evolve>;
    skill_table[heal] = perform_targetted_allied_fast<heal>;
    skill_table[jam] = perform_targetted_hostile_fast<jam>;
    skill_table[mend] = perform_targetted_allied_fast<mend>;
    skill_table[overload] = perform_targetted_allied_fast<overload>;
    skill_table[protect] = perform_targetted_allied_fast<protect>;
    skill_table[rally] = perform_targetted_allied_fast<rally>;
    skill_table[rush] = perform_targetted_allied_fast_rush;
    skill_table[siege] = perform_targetted_hostile_fast<siege>;
    skill_table[strike] = perform_targetted_hostile_fast<strike>;
    skill_table[sunder] = perform_targetted_hostile_fast<sunder>;
    skill_table[weaken] = perform_targetted_hostile_fast<weaken>;
}
