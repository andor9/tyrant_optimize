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
inline std::string status_description(CardStatus* status)
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
inline bool CardStatus::has_skill(Skill skill_id) const
{
    return m_card->m_skill_value[skill_id];
}
//------------------------------------------------------------------------------
template<Skill skill_id>
inline bool CardStatus::has_skill() const
{
    return m_card->m_skill_value[skill_id];
}
//------------------------------------------------------------------------------
template<Skill skill_id>
inline unsigned CardStatus::skill() const
{
    return m_card->m_skill_value[skill_id] + enhanced(skill_id);
}
//------------------------------------------------------------------------------
inline unsigned CardStatus::enhanced(Skill skill) const
{
    return m_enhanced_value[skill];
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
    m_berserk = 0;
    m_corroded_rate = 0;
    m_corroded_weakened = 0;
    m_delay = card.m_delay;
    m_evaded = 0;
    m_enfeebled = 0;
    m_faction = card.m_faction;
    m_hp = card.m_health;
    m_inhibited = 0;
    m_jammed = false;
    m_overloaded = false;
    m_poisoned = 0;
    m_protected = 0;
    m_rallied = 0;
    m_weakened = 0;
    m_valor = 0;
    m_step = CardStep::none;
    std::memset(m_enhanced_value, 0, sizeof m_enhanced_value);
    std::memset(m_skill_cd, 0, sizeof m_skill_cd);
}
//------------------------------------------------------------------------------
inline int attack_power(CardStatus* att)
{
    return(safe_minus(att->m_card->m_attack + att->m_berserk + att->m_rallied + att->m_valor, att->m_weakened + att->m_corroded_weakened));
}
//------------------------------------------------------------------------------
std::string skill_description(const Cards& cards, const SkillSpec& s)
{
    return skill_names[s.id] +
       (s.all ? " all" : s.n == 0 ? "" : std::string(" ") + to_string(s.n)) +
       (s.y == allfactions ? "" : std::string(" ") + faction_names[s.y]) +
       (s.s == no_skill ? "" : std::string(" ") + skill_names[s.s]) +
       (s.x == 0 || s.x == s.n ? "" : std::string(" ") + to_string(s.x)) +
       (s.c == 0 ? "" : std::string(" every ") + to_string(s.c));
}
std::string skill_short_description(const SkillSpec& s)
{
    // NOTE: not support summon
    return skill_names[s.id] +
        (s.s == no_skill ? "" : std::string(" ") + skill_names[s.s]) +
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
    if(c->m_rarity == 4) { desc += " legendary"; }
    if(c->m_faction != allfactions) { desc += " " + faction_names[c->m_faction]; }
    for(auto& skill: c->m_skills) { desc += ", " + skill_description(cards, skill); }
    return(desc);
}
//------------------------------------------------------------------------------
std::string CardStatus::description()
{
    std::string desc;
    switch(m_card->m_type)
    {
    case CardType::commander: desc = "Commander "; break;
    case CardType::assault: desc = "Assault " + to_string(m_index) + " "; break;
    case CardType::structure: desc = "Structure " + to_string(m_index) + " "; break;
    case CardType::num_cardtypes: assert(false); break;
    }
    desc += "[" + m_card->m_name;
    switch(m_card->m_type)
    {
    case CardType::assault:
        desc += " att:" + to_string(m_card->m_attack);
        {
            std::string att_desc;
            if(m_berserk > 0) { att_desc += "+" + to_string(m_berserk) + "(berserk)"; }
            if(m_rallied > 0) { att_desc += "+" + to_string(m_rallied) + "(rallied)"; }
            if(m_weakened > 0) { att_desc += "-" + to_string(m_weakened) + "(weakened)"; }
            if(m_corroded_weakened > 0) { att_desc += "-" + to_string(m_corroded_weakened) + "(corroded)"; }
            if(m_valor > 0) { att_desc += "+" + to_string(m_valor) + "(valor)"; }
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
    if(m_jammed) { desc += ", jammed"; }
    if(m_overloaded) { desc += ", overloaded"; }
    if(m_corroded_rate > 0) { desc += ", corroded " + to_string(m_corroded_rate); }
    if(m_enfeebled > 0) { desc += ", enfeebled " + to_string(m_enfeebled); }
    if(m_inhibited > 0) { desc += ", inhibited " + to_string(m_inhibited); }
    if(m_poisoned > 0) { desc += ", poisoned " + to_string(m_poisoned); }
    if(m_protected > 0) { desc += ", protected " + to_string(m_protected); }
//    if(m_step != CardStep::none) { desc += ", Step " + to_string(static_cast<int>(m_step)); }
    desc += "]";
    return(desc);
}
//------------------------------------------------------------------------------
void Hand::reset(std::mt19937& re)
{
    assaults.reset();
    structures.reset();
    commander.set(deck->get_commander(re));
    deck->shuffle(re);
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
SkillSpec apply_enhance(const SkillSpec& s, unsigned enhanced_value)
{
    SkillSpec enahnced_s = s;
    enahnced_s.x += enhanced_value;
    return(enahnced_s);
}
//------------------------------------------------------------------------------
void(*skill_table[num_skills])(Field*, CardStatus* src_status, const SkillSpec&);
void resolve_skill(Field* fd)
{
    while(!fd->skill_queue.empty())
    {
        auto skill_instance(fd->skill_queue.front());
        auto& status(std::get<0>(skill_instance));
        const auto& skill(std::get<1>(skill_instance));
        fd->skill_queue.pop_front();
        if (!status->m_jammed)
        {
            unsigned enhanced_value = status->enhanced(skill.id);
            auto& enhanced_s = enhanced_value > 0 ? apply_enhance(skill, enhanced_value) : skill;
            auto& modified_s = enhanced_s;
            skill_table[skill.id](fd, status, modified_s);
        }
    }
}
//------------------------------------------------------------------------------
bool attack_phase(Field* fd);
void evaluate_skills(Field* fd, CardStatus* status, const std::vector<SkillSpec>& skills)
{
    assert(status);
    assert(fd->skill_queue.size() == 0);
    for(auto& ss: skills)
    {
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
        _DEBUG_MSG(1, "%s plays %s %u [%s]\n", status_description(&fd->tap->commander).c_str(), cardtype_names[type].c_str(), static_cast<unsigned>(storage->size() - 1), card_description(fd->cards, card).c_str());
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
//------------------------------------------------------------------------------
inline bool has_attacked(CardStatus* c) { return(c->m_step == CardStep::attacked); }
inline bool is_jammed(CardStatus* c) { return(c->m_jammed); }
inline bool is_active(CardStatus* c) { return(c->m_delay == 0); }
inline bool is_active_next_turn(CardStatus* c) { return(c->m_delay <= 1); }
inline bool can_act(CardStatus* c) { return(c->m_hp > 0 && !is_jammed(c)); }
inline bool can_attack(CardStatus* c) { return(can_act(c)); }
// Can be healed / repaired
inline bool can_be_healed(CardStatus* c) { return(c->m_hp > 0 && c->m_hp < c->m_card->m_health); }
//------------------------------------------------------------------------------
void turn_start_phase(Field* fd);
void turn_end_phase(Field* fd);
// return value : (raid points) -> attacker wins, 0 -> defender wins
Results<uint64_t> play(Field* fd)
{
    fd->players[0]->commander.m_player = 0;
    fd->players[1]->commander.m_player = 1;
    fd->tapi = fd->gamemode == surge ? 1 : 0;
    fd->tipi = opponent(fd->tapi);
    fd->tap = fd->players[fd->tapi];
    fd->tip = fd->players[fd->tipi];
    fd->end = false;
    fd->n_player_kills = 0;

    // Play fortresses
    for (unsigned _ = 0; _ < 2; ++ _)
    {
        for (const Card* played_card: fd->tap->deck->fort_cards)
        {
            PlayCard(played_card, fd).op<CardType::structure>();
        }
        std::swap(fd->tapi, fd->tipi);
        std::swap(fd->tap, fd->tip);
    }

#if 0
    // ANP: Last decision point is second-to-last card played.
    fd->points_since_last_decision = 0;
#endif
    unsigned p0_size = fd->players[0]->deck->cards.size();
    unsigned p1_size = fd->players[1]->deck->cards.size();
    fd->players[0]->available_summons = 29 + p0_size;
    fd->players[1]->available_summons = 29 + p1_size;

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

        if (fd->bg_skill.id != no_skill)
        {
            // Evaluate TU Battleground effect (Enhance all)
            _DEBUG_MSG(2, "Evaluating Battleground skill %s\n", skill_description(fd->cards, fd->bg_skill).c_str());
            fd->skill_queue.emplace_back(&fd->tap->commander, fd->bg_skill);
            resolve_skill(fd);
        }

        // Evaluate commander
        fd->current_phase = Field::commander_phase;
        evaluate_skills(fd, &fd->tap->commander, fd->tap->commander.m_card->m_skills);
        if(__builtin_expect(fd->end, false)) { break; }

        // Evaluate structures
        fd->current_phase = Field::structures_phase;
        for(fd->current_ci = 0; !fd->end && fd->current_ci < fd->tap->structures.size(); ++fd->current_ci)
        {
            CardStatus& current_status(fd->tap->structures[fd->current_ci]);
            if(current_status.m_delay == 0 && current_status.m_hp > 0)
            {
                evaluate_skills(fd, &current_status, current_status.m_card->m_skills);
            }
        }
        // Evaluate assaults
        fd->current_phase = Field::assaults_phase;
        for(fd->current_ci = 0; !fd->end && fd->current_ci < fd->tap->assaults.size(); ++fd->current_ci)
        {
            // ca: current assault
            CardStatus* current_status(&fd->tap->assaults[fd->current_ci]);
            bool attacked = false;
            if(!is_active(current_status) || !can_act(current_status))
            {
                _DEBUG_MSG(2, "Assault %s cannot take action.\n", status_description(current_status).c_str());
            }
            else
            {
                unsigned num_actions(1);
                for(unsigned action_index(0); action_index < num_actions; ++action_index)
                {
                    // Evaluate skills
                    evaluate_skills(fd, current_status, current_status->m_card->m_skills);
                    // no commander-killing skill yet // if(__builtin_expect(fd->end, false)) { break; }
                    // Attack
                    if(can_attack(current_status))
                    {
                        attacked = attack_phase(fd) || attacked;
                        if(__builtin_expect(fd->end, false)) { break; }
                    }
                    else
                    {
                        _DEBUG_MSG(2, "Assault %s cannot take attack.\n", status_description(current_status).c_str());
                    }
                    // Flurry
                    if (can_act(current_status) && fd->tip->commander.m_hp > 0 && current_status->has_skill<flurry>() && current_status->m_skill_cd[flurry] == 0)
                    {
                        _DEBUG_MSG(1, "%s activates Flurry\n", status_description(current_status).c_str());
                        num_actions = 2;
                        for (const auto & ss : current_status->m_card->m_skills)
                        {
                            if (ss.id == flurry)
                            {
                                current_status->m_skill_cd[flurry] = ss.c;
                            }
                        }
                    }
                }
            }
            if (current_status->m_corroded_rate > 0)
            {
                if (attacked)
                {
                    unsigned v = std::min(current_status->m_corroded_rate, safe_minus(current_status->m_card->m_attack + current_status->m_berserk, current_status->m_corroded_weakened));
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
    unsigned raid_damage = 15 + fd->n_player_kills - (10 * fd->players[1]->commander.m_hp / fd->players[1]->commander.m_card->m_health);
    unsigned brawl_score = 50 + fd->n_player_kills * 2 - fd->turn / 2;  // Note that turn is +1, which is intentional
    // you lose
    if(fd->players[0]->commander.m_hp == 0)
    {
        _DEBUG_MSG(1, "You lose.\n");
        switch (fd->optimization_mode)
        {
        case OptimizationMode::raid: return {0, 0, 1, raid_damage, 0};
        case OptimizationMode::brawl: return {0, 0, 1, 5, 0};
        default: return {0, 0, 1, 0, 0};
        }
    }
    // you win
    if(fd->players[1]->commander.m_hp == 0)
    {
        _DEBUG_MSG(1, "You win.\n");
        switch (fd->optimization_mode)
        {
        case OptimizationMode::brawl: return {1, 0, 0, brawl_score, 0};
        default: return {1, 0, 0, 100, 0};
        }
    }
    if (fd->turn > turn_limit)
    {
        _DEBUG_MSG(1, "Stall after %u turns.\n", turn_limit);
        switch (fd->optimization_mode)
        {
        case OptimizationMode::defense: return {1, 1, 0, 100, 0};
        case OptimizationMode::raid: return {0, 1, 0, raid_damage, 0};
        case OptimizationMode::brawl: return {0, 1, 0, 5, 0};
        default: return {0, 1, 0, 0, 0};
        }
    }

    // Huh? How did we get here?
    assert(false);
    return {0, 0, 0, 0, 0};
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

void remove_hp(Field* fd, CardStatus& status, unsigned dmg)
{
    assert(status.m_hp > 0);
    _DEBUG_MSG(2, "%s takes %u damage\n", status_description(&status).c_str(), dmg);
    status.m_hp = safe_minus(status.m_hp, dmg);
    if(status.m_hp == 0)
    {
        _DEBUG_MSG(1, "%s dies\n", status_description(&status).c_str());
        if (status.m_player == 1)
        {
            fd->n_player_kills += 1;
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
    target->m_hp = std::min(target->m_hp + v, target->m_card->m_health);
}
void cooldown_skills(CardStatus & status)
{
    for (const auto & ss : status.m_card->m_skills)
    {
        if (status.m_skill_cd[ss.id] > 0)
        {
            _DEBUG_MSG(2, "%s reduces timer (%u) of skill %s\n", status_description(&status).c_str(), status.m_skill_cd[ss.id], skill_names[ss.id].c_str());
            -- status.m_skill_cd[ss.id];
        }
    }
}
void turn_start_phase(Field* fd)
{
    // Active player's commander card:
    cooldown_skills(fd->tap->commander);
    // Active player's assault cards:
    // update index
    // reduce delay; [TU] reduce skill cooldown
    // [WM:T] apply poison damage
    {
        auto& assaults(fd->tap->assaults);
        for(unsigned index(0), end(assaults.size());
            index < end;
            ++index)
        {
            CardStatus& status(assaults[index]);
            status.m_index = index;
            if(status.m_delay > 0)
            {
                _DEBUG_MSG(1, "%s reduces its timer\n", status_description(&status).c_str());
                -- status.m_delay;
            }
            cooldown_skills(status);
        }
    }
    // Active player's structure cards:
    // update index
    // reduce delay; [TU] reduce skill cooldown
    {
        auto& structures(fd->tap->structures);
        for(unsigned index(0), end(structures.size());
            index < end;
            ++index)
        {
            CardStatus& status(structures[index]);
            status.m_index = index;
            if(status.m_delay > 0)
            {
                _DEBUG_MSG(1, "%s reduces its timer\n", status_description(&status).c_str());
                --status.m_delay;
            }
            cooldown_skills(status);
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
    // Active player's assault cards:
    // remove jam, rally, weaken
    // apply poison damage
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
            status.m_jammed = false;
            status.m_overloaded = false;
            status.m_rallied = 0;
            status.m_weakened = 0;
            status.m_step = CardStep::none;
            status.m_inhibited = 0;
            unsigned poison_dmg = safe_minus(status.m_poisoned, status.protected_value());
            if(poison_dmg > 0)
            {
                _DEBUG_MSG(1, "%s takes poison damage %u\n", status_description(&status).c_str(), poison_dmg);
                remove_hp(fd, status, poison_dmg);
            }
#if 0
            // not need to fade out in own turn in TU
            status.m_enfeebled = 0;
#endif
        }
    }
    // Active player's structure cards:
    // nothing so far

    // Defending player's assault cards:
    // remove enfeeble, protect
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
            // so far only useful in Defending turn
            status.m_evaded = 0;
            std::memset(status.m_enhanced_value, 0, sizeof status.m_enhanced_value);
        }
    }
    // Defending player's structure cards:
    // nothing so far
#if 0
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
        }
    }
#endif
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
    return(safe_minus(def->skill<counter>() + att->m_enfeebled, att->protected_value()));
}
inline CardStatus* select_first_enemy_wall(Field* fd)
{
    for(unsigned i(0); i < fd->tip->structures.size(); ++i)
    {
        CardStatus& c(fd->tip->structures[i]);
        if(c.has_skill<wall>() && c.m_hp > 0 && skill_check<wall>(fd, &c, nullptr))
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
    assert(status.m_hp > 0);
    assert(status.m_card->m_type == CardType::commander);
    _DEBUG_MSG(2, "%s takes %u damage\n", status_description(&status).c_str(), dmg);
    status.m_hp = safe_minus(status.m_hp, dmg);
    // ANP: If commander is enemy's, player gets points equal to damage.
    // Points are awarded for overkill, so it is correct to simply add dmg.
    if(count_points && status.m_player == 1)
    {
#if 0
        fd->points_since_last_decision += dmg;
#endif
    }
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
    bool killed_by_attack;

    PerformAttack(Field* fd_, CardStatus* att_status_, CardStatus* def_status_) :
        fd(fd_), att_status(att_status_), def_status(def_status_), att_dmg(0), killed_by_attack(false)
    {}

    template<enum CardType::CardType def_cardtype>
    void op()
    {
        unsigned pre_modifier_dmg = attack_power(att_status);
        if(pre_modifier_dmg == 0) { return; }
        // Evaluation order:
        // modify damage
        // deal damage
        // assaults only: (poison)
        // counter, berserk
        // assaults only: (leech if still alive)

        modify_attack_damage<def_cardtype>(pre_modifier_dmg);

        if(att_dmg > 0)
        {
            attack_damage<def_cardtype>();
            if(__builtin_expect(fd->end, false)) { return; }
            damage_dependant_pre_oa<def_cardtype>();
        }
        if(att_dmg > 0)
        {
            if(att_status->m_hp > 0)
            {
                if(def_status->has_skill<counter>() && skill_check<counter>(fd, def_status, att_status))
                {
                    // perform_skill_counter
                    unsigned counter_dmg(counter_damage(fd, att_status, def_status));
                    _DEBUG_MSG(1, "%s takes %u counter damage from %s\n", status_description(att_status).c_str(), counter_dmg, status_description(def_status).c_str());
                    remove_hp(fd, *att_status, counter_dmg);
                }
                unsigned berserk_value = att_status->skill<berserk>();
                if(berserk_value > 0 && skill_check<berserk>(fd, att_status, nullptr))
                {
                    // perform_skill_berserk
                    att_status->m_berserk += berserk_value;
                }
                unsigned corrosive_value = def_status->skill<corrosive>();
                if (corrosive_value > att_status->m_corroded_rate && skill_check<corrosive>(fd, def_status, att_status))
                {
                    // perform_skill_corrosive
                    _DEBUG_MSG(1, "%s corrodes %s by %u\n", status_description(def_status).c_str(), status_description(att_status).c_str(), corrosive_value);
                    att_status->m_corroded_rate = corrosive_value;
                }
            }
            do_leech<def_cardtype>();
        }
    }

    template<enum CardType::CardType>
    void modify_attack_damage(unsigned pre_modifier_dmg)
    {
        assert(att_status->m_card->m_type == CardType::assault);
        assert(pre_modifier_dmg > 0);
        att_dmg = pre_modifier_dmg;
        // enhance damage
        std::string desc;
        if(def_status->m_enfeebled > 0)
        {
            if(debug_print > 0) { desc += "+" + to_string(def_status->m_enfeebled) + "(enfeebled)"; }
            att_dmg += def_status->m_enfeebled;
        }
        // prevent damage
        std::string reduced_desc;
        unsigned reduced_dmg(0);
        unsigned armor_value = def_status->skill<armor>();
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
        if(reduced_dmg > 0 && att_status->skill<pierce>() > 0)
        {
            if(debug_print > 0) { reduced_desc += "-" + to_string(att_status->skill<pierce>()) + "(pierce)"; }
            reduced_dmg = safe_minus(reduced_dmg, att_status->skill<pierce>());
        }
        att_dmg = safe_minus(att_dmg, reduced_dmg);
        if(debug_print > 0)
        {
            if(!reduced_desc.empty()) { desc += "-[" + reduced_desc + "]"; }
            if(!desc.empty()) { desc += "=" + to_string(att_dmg); }
            _DEBUG_MSG(1, "%s attacks %s for %u%s damage\n", status_description(att_status).c_str(), status_description(def_status).c_str(), pre_modifier_dmg, desc.c_str());
        }
    }

    template<enum CardType::CardType>
    void attack_damage()
    {
        remove_hp(fd, *def_status, att_dmg);
        killed_by_attack = def_status->m_hp == 0;
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
    unsigned poison_value = att_status->skill<poison>();
    if(poison_value > def_status->m_poisoned && skill_check<poison>(fd, att_status, def_status))
    {
        // perform_skill_poison
        _DEBUG_MSG(1, "%s poisons %s by %u\n", status_description(att_status).c_str(), status_description(def_status).c_str(), poison_value);
        def_status->m_poisoned = poison_value;
    }
    unsigned inhibit_value = att_status->skill<inhibit>();
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
    unsigned leech_value = std::min(att_dmg, att_status->skill<leech>());
    if(leech_value > 0 && skill_check<leech>(fd, att_status, nullptr))
    {
        _DEBUG_MSG(1, "%s leeches %u health\n", status_description(att_status).c_str(), leech_value);
        add_hp(fd, att_status, leech_value);
    }
}

// General attack phase by the currently evaluated assault, taking into accounts exotic stuff such as flurry,swipe,etc.
void attack_commander(Field* fd, CardStatus* att_status)
{
    CardStatus* def_status{select_first_enemy_wall(fd)}; // defending wall
    if(def_status != nullptr)
    {
        PerformAttack{fd, att_status, def_status}.op<CardType::structure>();
    }
    else
    {
        PerformAttack{fd, att_status, &fd->tip->commander}.op<CardType::commander>();
    }
}
// Return true if actually attacks
bool attack_phase(Field* fd)
{
    CardStatus* att_status(&fd->tap->assaults[fd->current_ci]); // attacking card
    Storage<CardStatus>& def_assaults(fd->tip->assaults);
    if(attack_power(att_status) == 0)
    {
        return false;
    }

    if (alive_assault(def_assaults, fd->current_ci))
    {
        PerformAttack{fd, att_status, &fd->tip->assaults[fd->current_ci]}.op<CardType::assault>();
    }
    else
    {
        // might be blocked by walls
        attack_commander(fd, att_status);
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
    return dst->has_skill(s.s) && ((BEGIN_DEFENSIVE < s.s && s.s < END_DEFENSIVE) || is_active(dst));
}

template<>
inline bool skill_predicate<heal>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{ return(can_be_healed(dst)); }

template<>
inline bool skill_predicate<jam>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    return can_act(dst) && is_active_next_turn(dst);
}

// valor "targets" opposing assault if my attack is lower than their attack
template<>
inline bool skill_predicate<valor>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s) {
    Storage<CardStatus>& def_assaults(fd->tip->assaults);
    if (alive_assault(def_assaults, fd->current_ci))
    {
        bool is_opposite = (dst == &fd->tip->assaults[fd->current_ci]);
        return (is_opposite && (attack_power(src) < attack_power(dst)));
    }
    return false;
}

template<>
inline bool skill_predicate<overload>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    if (dst->m_overloaded || has_attacked(dst) || !(is_active(dst) && can_act(dst)))
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
    for (const auto & s: dst->m_card->m_skills)
    {
        if (BEGIN_ACTIVATION_HARMFUL < s.id && s.id < END_ACTIVATION_HARMFUL)
        {
            return true;
        }
        if (has_inhibited_unit && (/* s.id == enhance ||*/ s.id == heal || /*s.id == overload ||*/ s.id == protect || s.id == rally))
        {
            return true;
        }
    }
    return false;
}

template<>
inline bool skill_predicate<rally>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    return can_attack(dst) && is_active(dst) && !has_attacked(dst);
}

template<>
inline bool skill_predicate<weaken>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    return can_attack(dst) && attack_power(dst) > 0 && is_active_next_turn(dst);
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
    dst->m_enhanced_value[s.s] += s.x;
}

template<>
inline void perform_skill<heal>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    add_hp(fd, dst, s.x);
}

template<>
inline void perform_skill<jam>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_jammed = true;
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
inline void perform_skill<siege>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    remove_hp(fd, *dst, s.x);
}

template<>
inline void perform_skill<strike>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    unsigned strike_dmg = safe_minus(s.x + dst->m_enfeebled, src->m_overloaded ? 0 : dst->protected_value());
    remove_hp(fd, *dst, strike_dmg);
}

template<>
inline void perform_skill<weaken>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    dst->m_weakened += s.x;
}

template<>
inline void perform_skill<valor>(Field* fd, CardStatus* src, CardStatus* dst, const SkillSpec& s)
{
    src->m_valor += s.x;
}

template<unsigned skill_id>
inline unsigned select_fast(Field* fd, CardStatus* src_status, const std::vector<CardStatus*>& cards, const SkillSpec& s)
{
    if(s.y == allfactions || fd->effect == metamorphosis)
    {
        return(fd->make_selection_array(cards.begin(), cards.end(), [fd, src_status, s](CardStatus* c){return(skill_predicate<skill_id>(fd, src_status, c, s));}));
    }
    else
    {
        return(fd->make_selection_array(cards.begin(), cards.end(), [fd, src_status, s](CardStatus* c){return((c->m_faction == s.y || c->m_faction == progenitor) && skill_predicate<skill_id>(fd, src_status, c, s));}));
    }
}

inline std::vector<CardStatus*>& skill_targets_hostile_assault(Field* fd, CardStatus* src_status)
{
    return(fd->players[opponent(src_status->m_player)]->assaults.m_indirect);
}

inline std::vector<CardStatus*>& skill_targets_allied_assault(Field* fd, CardStatus* src_status)
{
    return(fd->players[src_status->m_player]->assaults.m_indirect);
}

inline std::vector<CardStatus*>& skill_targets_hostile_structure(Field* fd, CardStatus* src_status)
{
    return(fd->players[opponent(src_status->m_player)]->structures.m_indirect);
}

inline std::vector<CardStatus*>& skill_targets_allied_structure(Field* fd, CardStatus* src_status)
{
    return(fd->players[src_status->m_player]->structures.m_indirect);
}

template<unsigned skill>
std::vector<CardStatus*>& skill_targets(Field* fd, CardStatus* src_status)
{
    std::cerr << "skill_targets: Error: no specialization for " << skill_names[skill] << "\n";
    throw;
}

template<> std::vector<CardStatus*>& skill_targets<enfeeble>(Field* fd, CardStatus* src_status)
{ return(skill_targets_hostile_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<enhance>(Field* fd, CardStatus* src_status)
{ return(skill_targets_allied_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<heal>(Field* fd, CardStatus* src_status)
{ return(skill_targets_allied_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<jam>(Field* fd, CardStatus* src_status)
{ return(skill_targets_hostile_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<overload>(Field* fd, CardStatus* src_status)
{ return(skill_targets_allied_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<protect>(Field* fd, CardStatus* src_status)
{ return(skill_targets_allied_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<rally>(Field* fd, CardStatus* src_status)
{ return(skill_targets_allied_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<strike>(Field* fd, CardStatus* src_status)
{ return(skill_targets_hostile_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<weaken>(Field* fd, CardStatus* src_status)
{ return(skill_targets_hostile_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<valor>(Field* fd, CardStatus* src_status)
{ return(skill_targets_hostile_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<siege>(Field* fd, CardStatus* src_status)
{ return(skill_targets_hostile_structure(fd, src_status)); }

template<Skill skill_id>
bool check_and_perform_skill(Field* fd, CardStatus* src_status, CardStatus* dst_status, const SkillSpec& s, bool is_evadable)
{
    if(skill_check<skill_id>(fd, src_status, dst_status))
    {
        if (is_evadable &&
                dst_status->m_evaded < dst_status->skill<evade>() &&
                skill_check<evade>(fd, dst_status, src_status))
        {
            ++ dst_status->m_evaded;
            _DEBUG_MSG(1, "%s %s on %s but it evades\n", status_description(src_status).c_str(), skill_short_description(s).c_str(), status_description(dst_status).c_str());
            return(false);
        }
        _DEBUG_MSG(1, "%s %s on %s\n", status_description(src_status).c_str(), skill_short_description(s).c_str(), status_description(dst_status).c_str());
        perform_skill<skill_id>(fd, src_status, dst_status, s);
        if (s.c > 0)
        {
            src_status->m_skill_cd[skill_id] = s.c;
        }
        return(true);
    }
    _DEBUG_MSG(1, "(CANCELLED) %s %s on %s\n", status_description(src_status).c_str(), skill_short_description(s).c_str(), status_description(dst_status).c_str());
    return(false);
}

template<Skill skill_id>
size_t select_targets(Field* fd, CardStatus* src_status, const SkillSpec& s)
{
    std::vector<CardStatus*>& cards(skill_targets<skill_id>(fd, src_status));
    size_t n_candidates = select_fast<skill_id>(fd, src_status, cards, s);
    if (n_candidates == 0)
    {
        return n_candidates;
    }
    _DEBUG_SELECTION("%s", skill_names[skill_id].c_str());
    unsigned n_targets = s.n > 0 ? s.n : 1;
    if (s.all || n_targets >= n_candidates) // target all
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
void perform_targetted_hostile_fast(Field* fd, CardStatus* src_status, const SkillSpec& s)
{
    select_targets<skill_id>(fd, src_status, s);
    for (CardStatus * c: fd->selection_array)
    {
        check_and_perform_skill<skill_id>(fd, src_status, c, s, ! src_status->m_overloaded);
    }
}

template<Skill skill_id>
void perform_targetted_allied_fast(Field* fd, CardStatus* src_status, const SkillSpec& s)
{
    select_targets<skill_id>(fd, src_status, s);
    for (CardStatus * dst: fd->selection_array)
    {
        if(dst->m_inhibited > 0 && !src_status->m_overloaded)
        {
            _DEBUG_MSG(1, "%s %s on %s but it is inhibited\n", status_description(src_status).c_str(), skill_short_description(s).c_str(), status_description(dst).c_str());
            -- dst->m_inhibited;
            continue;
        }
        check_and_perform_skill<skill_id>(fd, src_status, dst, s, false);
    }
}

template<Skill skill_id>
void perform_self_opposing(Field* fd, CardStatus* src_status, const SkillSpec& s)
{
    select_targets<skill_id>(fd, src_status, s);
    // assuming valor cannot be inhibited or evaded
    for (CardStatus *c: fd->selection_array)
    {
        check_and_perform_skill<skill_id>(fd, src_status, c, s, false);
    }
}

//------------------------------------------------------------------------------
void fill_skill_table()
{
    memset(skill_table, 0, sizeof skill_table);
    skill_table[enfeeble] = perform_targetted_hostile_fast<enfeeble>;
    skill_table[enhance] = perform_targetted_allied_fast<enhance>;
    skill_table[heal] = perform_targetted_allied_fast<heal>;
    skill_table[jam] = perform_targetted_hostile_fast<jam>;
    skill_table[overload] = perform_targetted_allied_fast<overload>;
    skill_table[protect] = perform_targetted_allied_fast<protect>;
    skill_table[rally] = perform_targetted_allied_fast<rally>;
    skill_table[siege] = perform_targetted_hostile_fast<siege>;
    skill_table[strike] = perform_targetted_hostile_fast<strike>;
    skill_table[weaken] = perform_targetted_hostile_fast<weaken>;
    skill_table[valor] = perform_self_opposing<valor>;
}
