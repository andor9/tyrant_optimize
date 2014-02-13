#include "sim.h"

#include <boost/range/adaptors.hpp>
#include <boost/range/join.hpp>
#include <random>
#include <string>
#include <sstream>
#include <vector>

#include "card.h"
#include "cards.h"
#include "deck.h"
#include "achievement.h"

//---------------------- $00 general stuff -------------------------------------
template<typename T>
std::string to_string(T val)
{
    std::stringstream s;
    s << val;
    return s.str();
}
//---------------------- Debugging stuff ---------------------------------------
unsigned debug_print(0);
unsigned debug_cached(0);
bool debug_line(false);
std::string debug_str;
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
    for(auto c: this->selection_array)
    {
        _DEBUG_MSG(2, "+ %s\n", status_description(c).c_str());
    }
}
//------------------------------------------------------------------------------
CardStatus::CardStatus(const Card* card) :
    m_card(card),
    m_index(0),
    m_player(0),
    m_augmented(0),
    m_berserk(0),
    m_blitzing(false),
    m_chaosed(false),
    m_delay(card->m_delay),
    m_diseased(false),
    m_enfeebled(0),
    m_faction(card->m_faction),
    m_frozen(false),
    m_hp(card->m_health),
    m_immobilized(false),
    m_infused(false),
    m_jammed(false),
    m_phased(false),
    m_poisoned(0),
    m_protected(0),
    m_rallied(0),
    m_stunned(0),
    m_sundered(false),
    m_weakened(0),
    m_temporary_split(false),
    m_is_summoned(false),
    m_step(CardStep::none)
{
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
    m_augmented = 0;
    m_berserk = 0;
    m_blitzing = false;
    m_chaosed = false;
    m_delay = card.m_delay;
    m_diseased = false;
    m_enfeebled = 0;
    m_faction = card.m_faction;
    m_frozen = false;
    m_hp = card.m_health;
    m_immobilized = false;
    m_infused = false;
    m_jammed = false;
    m_phased = false;
    m_poisoned = 0;
    m_protected = 0;
    m_rallied = 0;
    m_stunned = 0;
    m_sundered = false;
    m_temporary_split = false;
    m_weakened = 0;
    m_is_summoned = false;
    m_step = CardStep::none;
}
//------------------------------------------------------------------------------
inline int attack_power(CardStatus* att)
{
    return(safe_minus(att->m_card->m_attack + att->m_berserk + att->m_rallied, att->m_weakened));
}
//------------------------------------------------------------------------------
std::string skill_description(const Cards& cards, const SkillSpec& s)
{
    switch(std::get<0>(s))
    {
    case summon:
        if(std::get<1>(s) == 0)
        {
            // Summon X
            return(skill_names[std::get<0>(s)] + " X" +
                    skill_activation_modifier_names[std::get<4>(s)]);
        }
        else
        {
            return(skill_names[std::get<0>(s)] +
                    " " + cards.by_id(std::get<1>(s))->m_name.c_str() +
                    skill_activation_modifier_names[std::get<4>(s)]);
        }
    default:
        return(skill_names[std::get<0>(s)] +
           (std::get<3>(s) ? " all" : "") +
           (std::get<2>(s) == allfactions ? "" : std::string(" ") + faction_names[std::get<2>(s)]) +
           (std::get<1>(s) == 0 ? "" : std::string(" ") + to_string(std::get<1>(s))) +
           skill_activation_modifier_names[std::get<4>(s)]);
    }
}
//------------------------------------------------------------------------------
std::string card_description(const Cards& cards, const Card* c)
{
    std::string desc;
    desc = c->m_name;
    switch(c->m_type)
    {
    case CardType::action:
        break;
    case CardType::assault:
        desc += " " + to_string(c->m_attack) + "/" + to_string(c->m_health) + "/" + to_string(c->m_delay);
        break;
    case CardType::structure:
        desc += " " + to_string(c->m_health) + "/" + to_string(c->m_delay);
        break;
    case CardType::commander:
        desc += " hp:" + to_string(c->m_health);
        break;
    case CardType::num_cardtypes:
        assert(false);
        break;
    }
    if(c->m_unique) { desc += " unique"; }
    if(c->m_rarity == 4) { desc += " legendary"; }
    if(c->m_faction != allfactions) { desc += " " + faction_names[c->m_faction]; }
    if(c->m_antiair > 0) { desc += ", antiair " + to_string(c->m_antiair); }
    if(c->m_armored > 0) { desc += ", armored " + to_string(c->m_armored); }
    if(c->m_berserk > 0) { desc += ", berserk " + to_string(c->m_berserk); }
    if(c->m_blitz) { desc += ", blitz"; }
    if(c->m_burst > 0) { desc += ", burst " + to_string(c->m_burst); }
    if(c->m_counter > 0) { desc += ", counter " + to_string(c->m_counter); }
    if(c->m_crush > 0) { desc += ", crush " + to_string(c->m_crush); }
    if(c->m_disease) { desc += ", disease"; }
    if(c->m_emulate) { desc += ", emulate"; }
    if(c->m_evade) { desc += ", evade"; }
    if(c->m_fear) { desc += ", fear"; }
    if(c->m_flurry > 0) { desc += ", flurry " + to_string(c->m_flurry); }
    if(c->m_flying) { desc += ", flying"; }
    if(c->m_fusion) { desc += ", fusion"; }
    if(c->m_immobilize) { desc += ", immobilize"; }
    if(c->m_intercept) { desc += ", intercept"; }
    if(c->m_leech > 0) { desc += ", leech " + to_string(c->m_leech); }
    if(c->m_legion > 0) { desc += ", legion " + to_string(c->m_legion); }
    if(c->m_payback) { desc += ", payback"; }
    if(c->m_pierce > 0) { desc += ", pierce " + to_string(c->m_pierce); }
    if(c->m_phase) { desc += ", phase"; }
    if(c->m_poison > 0) { desc += ", poison " + to_string(c->m_poison); }
    if(c->m_refresh) { desc += ", refresh"; }
    if(c->m_regenerate > 0) { desc += ", regenerate " + to_string(c->m_regenerate); }
    if(c->m_siphon > 0) { desc += ", siphon " + to_string(c->m_siphon); }
    if(c->m_stun) { desc += ", stun"; }
    if(c->m_sunder) { desc += ", sunder"; }
    if(c->m_swipe) { desc += ", swipe"; }
    if(c->m_tribute) { desc += ", tribute"; }
    if(c->m_valor > 0) { desc += ", valor " + to_string(c->m_valor); }
    if(c->m_wall) { desc += ", wall"; }
    for(auto& skill: c->m_skills) { desc += ", " + skill_description(cards, skill); }
    for(auto& skill: c->m_skills_on_play) { desc += ", " + skill_description(cards, skill); }
    for(auto& skill: c->m_skills_on_kill) { desc += ", " + skill_description(cards, skill); }
    if(c->m_berserk_oa > 0) { desc += ", berserk " + to_string(c->m_berserk_oa); }
    if(c->m_disease_oa) { desc += ", disease on Attacked"; }
    if(c->m_poison_oa > 0) { desc += ", poison " + to_string(c->m_poison_oa) + " on Attacked"; }
    if(c->m_sunder_oa) { desc += ", sunder on Attacked"; }
    for(auto& skill: c->m_skills_on_attacked) { desc += ", " + skill_description(cards, skill); }
    for(auto& skill: c->m_skills_on_death) { desc += ", " + skill_description(cards, skill); }
    return(desc);
}
//------------------------------------------------------------------------------
std::string CardStatus::description()
{
    std::string desc;
    switch(m_card->m_type)
    {
    case CardType::commander: desc = "Commander "; break;
    case CardType::action: desc = "Action "; break;
    case CardType::assault: desc = "Assault " + to_string(m_index) + " "; break;
    case CardType::structure: desc = "Structure " + to_string(m_index) + " "; break;
    case CardType::num_cardtypes: assert(false); break;
    }
    desc += "[" + m_card->m_name;
    switch(m_card->m_type)
    {
    case CardType::action:
        break;
    case CardType::assault:
        desc += " att:" + to_string(m_card->m_attack);
        {
            std::string att_desc;
            if(m_berserk > 0) { att_desc += "+" + to_string(m_berserk) + "(berserk)"; }
            if(m_rallied > 0) { att_desc += "+" + to_string(m_rallied) + "(rallied)"; }
            if(m_weakened > 0) { att_desc += "-" + to_string(m_weakened) + "(weakened)"; }
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
        if(m_blitzing) { desc += "(blitzing)"; }
    }
    if(m_chaosed) { desc += ", chaosed"; }
    if(m_diseased) { desc += ", diseaseded"; }
    if(m_frozen) { desc += ", frozen"; }
    if(m_immobilized) { desc += ", immobilized"; }
    if(m_infused) { desc += ", infused"; }
    if(m_jammed) { desc += ", jammed"; }
    if(m_phased) { desc += ", phased"; }
    if(m_sundered) { desc += ", sundered"; }
    if(m_temporary_split) { desc += ", cloning"; }
    if(m_augmented > 0) { desc += ", augmented " + to_string(m_augmented); }
    if(m_enfeebled > 0) { desc += ", enfeebled " + to_string(m_enfeebled); }
    if(m_poisoned > 0) { desc += ", poisoned " + to_string(m_poisoned); }
    if(m_protected > 0) { desc += ", protected " + to_string(m_protected); }
    if(m_stunned > 0) { desc += ", stunned " + to_string(m_stunned); }
//    if(m_step != CardStep::none) { desc += ", Step " + to_string(static_cast<int>(m_step)); }
    desc += "]";
    return(desc);
}
//------------------------------------------------------------------------------
inline void print_achievement_results(Field* fd)
{
#ifndef NDEBUG
    if(fd->achievement.req_counter.size() == 0)
    {
        return;
    }
    _DEBUG_MSG(1, "Achievement:\n");
    for(auto i : fd->achievement.skill_used)
    {
        _DEBUG_MSG(1, "  Use skills: %s %u%s? %s\n", skill_names[i.first].c_str(), fd->achievement_counter[i.second], fd->achievement.req_counter[i.second].str().c_str(), fd->achievement.req_counter[i.second].check(fd->achievement_counter[i.second]) ? "Yes" : "No");
    }
    for(auto i : fd->achievement.unit_played)
    {
        _DEBUG_MSG(1, "  Play units: %s %u%s? %s\n", fd->cards.by_id(i.first)->m_name.c_str(), fd->achievement_counter[i.second], fd->achievement.req_counter[i.second].str().c_str(), fd->achievement.req_counter[i.second].check(fd->achievement_counter[i.second]) ? "Yes" : "No");
    }
    for(auto i : fd->achievement.unit_type_played)
    {
        _DEBUG_MSG(1, "  Play units of type: %s %u%s? %s\n", cardtype_names[i.first].c_str(), fd->achievement_counter[i.second], fd->achievement.req_counter[i.second].str().c_str(), fd->achievement.req_counter[i.second].check(fd->achievement_counter[i.second]) ? "Yes" : "No");
    }
    for(auto i : fd->achievement.unit_faction_played)
    {
        _DEBUG_MSG(1, "  Play units of faction: %s %u%s? %s\n", faction_names[i.first].c_str(), fd->achievement_counter[i.second], fd->achievement.req_counter[i.second].str().c_str(), fd->achievement.req_counter[i.second].check(fd->achievement_counter[i.second]) ? "Yes" : "No");
    }
    for(auto i : fd->achievement.unit_rarity_played)
    {
        _DEBUG_MSG(1, "  Play units of rarity: %s %u%s? %s\n", rarity_names[i.first].c_str(), fd->achievement_counter[i.second], fd->achievement.req_counter[i.second].str().c_str(), fd->achievement.req_counter[i.second].check(fd->achievement_counter[i.second]) ? "Yes" : "No");
    }
    for(auto i : fd->achievement.unit_type_killed)
    {
        _DEBUG_MSG(1, "  Kill units of type: %s %u%s? %s\n", cardtype_names[i.first].c_str(), fd->achievement_counter[i.second], fd->achievement.req_counter[i.second].str().c_str(), fd->achievement.req_counter[i.second].check(fd->achievement_counter[i.second]) ? "Yes" : "No");
    }
    for(auto i : fd->achievement.misc_req)
    {
        _DEBUG_MSG(1, "  %s %u%s? %s\n", achievement_misc_req_names[i.first].c_str(), fd->achievement_counter[i.second], fd->achievement.req_counter[i.second].str().c_str(), fd->achievement.req_counter[i.second].check(fd->achievement_counter[i.second]) ? "Yes" : "No");
    }
#endif
}
//------------------------------------------------------------------------------
void Hand::reset(std::mt19937& re)
{
    assaults.reset();
    structures.reset();
    commander = CardStatus(deck->get_commander());
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
SkillSpec apply_augment(const CardStatus* status, const SkillSpec& s)
{
    if (std::get<0>(s) == augment || std::get<0>(s) == summon || std::get<1>(s) == 0)
    {
        return s;
    }
    SkillSpec augmented_s = s;
    std::get<1>(augmented_s) += status->m_augmented;
    return(augmented_s);
}
SkillSpec apply_fusion(const SkillSpec& s)
{
    SkillSpec fusioned_s = s;
    std::get<1>(fusioned_s) *= 2;
    return(fusioned_s);
}
SkillSpec apply_infuse(const SkillSpec& s)
{
    if (std::get<2>(s) == allfactions || std::get<2>(s) == bloodthirsty || helpful_skills.find(std::get<0>(s)) == helpful_skills.end())
    {
        return s;
    }
    SkillSpec infused_s = s;
    std::get<2>(infused_s) = bloodthirsty;
    return(infused_s);
}
//------------------------------------------------------------------------------
bool may_change_skill(const Field* fd, const CardStatus* status, const SkillMod::SkillMod mod)
{
    switch (mod)
    {
        case SkillMod::on_activate:
            switch (status->m_card->m_type)
            {
                case CardType::commander:
                    return (fd->effect == Effect::time_surge ||
                            fd->effect == Effect::friendly_fire ||
                            fd->effect == Effect::genesis ||
                            (fd->effect == Effect::artillery_strike && fd->turn >= 9 && status->m_player == (fd->optimization_mode == OptimizationMode::defense ? 1u : 0u)) ||
                            fd->effect == Effect::decrepit ||
                            fd->effect == Effect::forcefield ||
                            fd->effect == Effect::chilling_touch);
                case CardType::assault:
                    return (fd->effect == Effect::friendly_fire ||
                            ((fd->effect == Effect::clone_project || fd->effect == Effect::clone_experiment) && status->m_temporary_split));
                default:
                    break;
            }
            break;
        case SkillMod::on_death:
            return ((status->m_card->m_type == CardType::assault || status->m_card->m_type == CardType::structure) &&
                    fd->effect == Effect::haunt && status->m_card->m_faction != bloodthirsty);
        default:
            break;
    }
    return false;
}
SkillSpec apply_battleground_effect(const Field* fd, const CardStatus* status, const SkillSpec& ss, const SkillMod::SkillMod mod, bool& need_add_skill)
{
    const auto& skill = std::get<0>(ss);
    switch (fd->effect)
    {
        case Effect::time_surge:
            // replace other instance of the skill
            if(skill == rush || skill == new_skill)
            {
                need_add_skill = false;
                return SkillSpec(rush, 1, allfactions, false, mod);
            }
            break;
        case Effect::clone_project:
        case Effect::clone_experiment:
            // no gain the skill if already have
            if(skill == split)
            {
                need_add_skill = false;
            }
            else if(skill == new_skill)
            {
                return SkillSpec(split, 0, allfactions, false, mod);
            }
            break;
        case Effect::friendly_fire:
            switch (status->m_card->m_type)
            {
                case CardType::assault:
                    // no gain the skill if already have
                    if(skill == strike)
                    {
                        need_add_skill = false;
                    }
                    else if(skill == new_skill)
                    {
                        return SkillSpec(strike, 1, allfactions, false, mod);
                    }
                    break;
                case CardType::commander:
                    // replace other instance of the skill
                    if(skill == chaos || skill == new_skill)
                    {
                        need_add_skill = false;
                        return SkillSpec(chaos, 0, allfactions, true, mod);
                    }
                    break;
                default:
                    break;
            }
            break;
        case Effect::genesis:
            // replace other instance of the skill
            if(skill == summon || skill == new_skill)
            {
                need_add_skill = false;
                return SkillSpec(summon, 0, allfactions, false, mod);
            }
            break;
        case Effect::artillery_strike:
            // replace other instance of the skill
            if(skill == strike || skill == new_skill)
            {
                need_add_skill = false;
                return SkillSpec(strike, 3, allfactions, true, mod);
            }
            break;
        case Effect::decrepit:
            // replace other instance of the skill
            if(skill == enfeeble || skill == new_skill)
            {
                need_add_skill = false;
                return SkillSpec(enfeeble, 1, allfactions, true, mod);
            }
            break;
        case Effect::forcefield:
            // replace other instance of the skill
            if(skill == protect || skill == new_skill)
            {
                need_add_skill = false;
                return SkillSpec(protect, 1, allfactions, true, mod);
            }
            break;
        case Effect::chilling_touch:
            // replace other instance of the skill
            if(skill == freeze || skill == new_skill)
            {
                need_add_skill = false;
                return SkillSpec(freeze, 0, allfactions, false, mod);
            }
            break;
        case Effect::haunt:
            // replace other instance of the skill
            if(skill == summon || skill == new_skill)
            {
                need_add_skill = false;
                return SkillSpec(summon, 0, bloodthirsty, false, mod);
            }
            break;
        default:
            break;
    }
    return ss;
}
//------------------------------------------------------------------------------
void prepend_on_death(Field* fd)
{
    std::vector<std::tuple<CardStatus*, SkillSpec>> od_skills;
    auto mod = SkillMod::on_death;
    for(auto status: fd->killed_with_on_death)
    {
        if(status->m_jammed)
        {
            _DEBUG_MSG(2, "%s is jammed and cannot activate its on Death skill.\n", status_description(status).c_str());
            continue;
        }
        bool need_add_skill = may_change_skill(fd, status, mod);
        for(auto& ss: status->m_card->m_skills_on_death)
        {
            auto& battleground_s = need_add_skill ? apply_battleground_effect(fd, status, ss, mod, need_add_skill) : ss;
            _DEBUG_MSG(2, "Preparing %s skill %s\n", status_description(status).c_str(), skill_description(fd->cards, battleground_s).c_str());
            od_skills.emplace_back(status, battleground_s);
            //if(__builtin_expect(fd->end, false)) { return; }  // so far no "on Death" skill may end the battle
        }
        if(need_add_skill)
        {
            auto battleground_s = apply_battleground_effect(fd, status, SkillSpec(new_skill, 0, allfactions, false, mod), mod, need_add_skill);
            assert(std::get<0>(battleground_s) != new_skill);
            _DEBUG_MSG(2, "Preparing %s skill %s\n", status_description(status).c_str(), skill_description(fd->cards, battleground_s).c_str());
            od_skills.emplace_back(status, battleground_s);
        }
    }
    fd->skill_queue.insert(fd->skill_queue.begin(), od_skills.begin(), od_skills.end());
    fd->killed_with_on_death.clear();
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
        if(!status)
        {
            // trigger_regen
            skill_table[std::get<0>(skill)](fd, status, skill);
        }
        else if(!status->m_jammed)
        {
            bool fusion_active = status->m_card->m_fusion && status->m_player == fd->tapi && fd->fusion_count >= 3;
            auto& augmented_s = status->m_augmented > 0 ? apply_augment(status, skill) : skill;
            auto& fusioned_s = fusion_active ? apply_fusion(augmented_s) : augmented_s;
            auto& infused_s = status->m_infused ? apply_infuse(fusioned_s) : fusioned_s;
            skill_table[std::get<0>(skill)](fd, status, infused_s);
        }
    }
}
//------------------------------------------------------------------------------
void attack_phase(Field* fd);
void evaluate_skills(Field* fd, CardStatus* status, const std::vector<SkillSpec>& skills, const SkillMod::SkillMod mod)
{
    assert(status);
    assert(fd->skill_queue.size() == 0);
    bool need_add_skill = may_change_skill(fd, status, mod);
    for(auto& ss: skills)
    {
        auto& battleground_s = need_add_skill ? apply_battleground_effect(fd, status, ss, mod, need_add_skill) : ss;
        _DEBUG_MSG(2, "Evaluating %s skill %s\n", status_description(status).c_str(), skill_description(fd->cards, battleground_s).c_str());
        fd->skill_queue.emplace_back(status, battleground_s);
        resolve_skill(fd);
        if(__builtin_expect(fd->end, false)) { break; }
    }
    if(need_add_skill)
    {
        auto battleground_s = apply_battleground_effect(fd, status, SkillSpec(new_skill, 0, allfactions, false, mod), mod, need_add_skill);
        assert(std::get<0>(battleground_s) != new_skill);
        _DEBUG_MSG(2, "Evaluating %s skill %s\n", status_description(status).c_str(), skill_description(fd->cards, battleground_s).c_str());
        fd->skill_queue.emplace_back(status, battleground_s);
        resolve_skill(fd);
    }
}
bool check_and_perform_blitz(Field* fd, CardStatus* src_status);
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
        blitz<type>();
        onPlaySkills<type>();
        fieldEffects<type>();
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
        if((fd->turn == 1 && fd->gamemode == tournament && status->m_delay > 0) || (type == CardType::assault && fd->effect == Effect::harsh_conditions))
        {
            ++status->m_delay;
        }
        if(status->m_player == 0)
        {
            fd->inc_counter(fd->achievement.unit_played, card->m_id);
            fd->inc_counter(fd->achievement.unit_type_played, card->m_type);
            fd->inc_counter(fd->achievement.unit_faction_played, card->m_faction);
            fd->inc_counter(fd->achievement.unit_rarity_played, card->m_rarity);
        }
        _DEBUG_MSG(1, "%s plays %s %u [%s]\n", status_description(&fd->tap->commander).c_str(), cardtype_names[type].c_str(), static_cast<unsigned>(storage->size() - 1), card_description(fd->cards, card).c_str());
    }

    // all except assault: noop
    template <enum CardType::CardType>
    void blitz()
    {
    }

    // all except assault: noop
    template <enum CardType::CardType>
    void fieldEffects()
    {
    }

    // assault + structure
    template <enum CardType::CardType>
    void onPlaySkills()
    {
        evaluate_skills(fd, status, card->m_skills_on_play, SkillMod::on_play);
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
// action: played on structure slots
template <>
void PlayCard::setStorage<CardType::action>()
{
    storage = &fd->tap->structures;
}
// assault
template <>
void PlayCard::blitz<CardType::assault>()
{
    if(status->m_card->m_blitz)
    {
        check_and_perform_blitz(fd, status);
    }
}
// assault
template <>
void PlayCard::fieldEffects<CardType::assault>()
{
    if(fd->effect == Effect::toxic)
    {
        status->m_poisoned = 1;
    }
    else if(fd->effect == Effect::decay)
    {
        status->m_poisoned = 1;
        status->m_diseased = true;
    }
}
// action
template <>
void PlayCard::onPlaySkills<CardType::action>()
{
    evaluate_skills(fd, status, card->m_skills, SkillMod::on_play);
}
//------------------------------------------------------------------------------
inline bool is_attacking_or_has_attacked(CardStatus* c) { return(c->m_step >= CardStep::attacking); }
inline bool is_attacking(CardStatus* c) { return(c->m_step == CardStep::attacking); }
inline bool has_attacked(CardStatus* c) { return(c->m_step == CardStep::attacked); }
inline bool is_jammed(CardStatus* c) { return(c->m_jammed || c->m_frozen); }
inline bool is_active(CardStatus* c) { return(c->m_delay == 0 || c->m_blitzing); }
inline bool is_active_next_turn(CardStatus* c) { return(c->m_delay <= 1); }
inline bool can_act(CardStatus* c) { return(c->m_hp > 0 && !is_jammed(c)); }
inline bool can_attack(CardStatus* c) { return(can_act(c) && !c->m_immobilized && !c->m_stunned); }
// Can be healed / repaired
inline bool can_be_healed(CardStatus* c) { return(c->m_hp > 0 && c->m_hp < c->m_card->m_health && !c->m_diseased); }
//------------------------------------------------------------------------------
void turn_start_phase(Field* fd);
void evaluate_legion(Field* fd);
bool check_and_perform_refresh(Field* fd, CardStatus* src_status);
// return value : (raid points) -> attacker wins, 0 -> defender wins
Results<uint64_t> play(Field* fd)
{
    fd->players[0]->commander.m_player = 0;
    fd->players[1]->commander.m_player = 1;
    fd->tapi = fd->gamemode == surge ? 1 : 0;
    fd->tipi = opponent(fd->tapi);
    fd->tap = fd->players[fd->tapi];
    fd->tip = fd->players[fd->tipi];
    fd->fusion_count = 0;
    fd->end = false;
    fd->achievement_counter.clear();
    fd->achievement_counter.resize(fd->achievement.req_counter.size());

#if 0
    // ANP: Last decision point is second-to-last card played.
    fd->points_since_last_decision = 0;
#endif
    unsigned p0_size = fd->players[0]->deck->cards.size();
    unsigned p1_size = fd->players[1]->deck->cards.size();
    fd->players[0]->available_summons = 29 + p0_size;
    fd->players[1]->available_summons = 29 + p1_size;
    fd->last_decision_turn = p0_size == 1 ? 0 : p0_size * 2 - (fd->gamemode == surge ? 2 : 3);

    // Count commander as played for achievements (not count in type / faction / rarity requirements)
    fd->inc_counter(fd->achievement.unit_played, fd->players[0]->commander.m_card->m_id);

    fd->set_counter(fd->achievement.misc_req, AchievementMiscReq::turns, 1);
    while(__builtin_expect(fd->turn <= turn_limit && !fd->end, true))
    {
        fd->current_phase = Field::playcard_phase;
        // Initialize stuff, remove dead cards
        _DEBUG_MSG(1, "------------------------------------------------------------------------\n"
                "TURN %u begins for %s\n", fd->turn, status_description(&fd->tap->commander).c_str());
#if 0
        // ANP: If it's the player's turn and he's making a decision,
        // reset his points to 0.
        if(fd->tapi == 0 && fd->turn <= fd->last_decision_turn)
        {
            fd->points_since_last_decision = 0;
        }
#endif
        turn_start_phase(fd);
        // Special case: refresh on commander
        if(fd->tip->commander.m_card->m_refresh)
        {
            check_and_perform_refresh(fd, &fd->tip->commander);
        }

        if(fd->effect == Effect::clone_project ||
           (fd->effect == Effect::clone_experiment && (fd->turn == 9 || fd->turn == 10)))
        {
            if(fd->make_selection_array(fd->tap->assaults.m_indirect.begin(), fd->tap->assaults.m_indirect.end(), [](CardStatus* c){return(c->m_delay == 0 && c->m_hp > 0);}) > 0)
            {
                _DEBUG_SELECTION("Clone effect");
                CardStatus* c(fd->selection_array[fd->rand(0, fd->selection_array.size() - 1)]);
                _DEBUG_MSG(1, "%s gains skill Split until end of turn.\n", status_description(c).c_str());
                c->m_temporary_split = true;
            }
        }

        // Play a card
        const Card* played_card(fd->tap->deck->next());
        if(played_card)
        {
            switch(played_card->m_type)
            {
            case CardType::action:
                // end: handles commander death by shock
                PlayCard(played_card, fd).op<CardType::action>();
                break;
            case CardType::assault:
                PlayCard(played_card, fd).op<CardType::assault>();
                break;
            case CardType::structure:
                PlayCard(played_card, fd).op<CardType::structure>();
                break;
            case CardType::commander:
            case CardType::num_cardtypes:
                assert(false);
                break;
            }
        }
        if(__builtin_expect(fd->end, false)) { break; }

        // Evaluate Legion skill
        fd->current_phase = Field::legion_phase;
        evaluate_legion(fd);

        // Evaluate commander
        fd->current_phase = Field::commander_phase;
        evaluate_skills(fd, &fd->tap->commander, fd->tap->commander.m_card->m_skills, SkillMod::on_activate);
        if(__builtin_expect(fd->end, false)) { break; }

        // Evaluate structures
        fd->current_phase = Field::structures_phase;
        for(fd->current_ci = 0; !fd->end && fd->current_ci < fd->tap->structures.size(); ++fd->current_ci)
        {
            CardStatus& current_status(fd->tap->structures[fd->current_ci]);
            if(current_status.m_delay == 0 && current_status.m_hp > 0)
            {
                evaluate_skills(fd, &current_status, current_status.m_card->m_skills, SkillMod::on_activate);
            }
        }
        // Evaluate assaults
        fd->current_phase = Field::assaults_phase;
        for(fd->current_ci = 0; !fd->end && fd->current_ci < fd->tap->assaults.size(); ++fd->current_ci)
        {
            // ca: current assault
            CardStatus& current_status(fd->tap->assaults[fd->current_ci]);
            if(!is_active(&current_status) || !can_act(&current_status))
            {
                _DEBUG_MSG(2, "Assault %s cannot take action.\n", status_description(&current_status).c_str());
                current_status.m_step = CardStep::attacked;
                continue;
            }
            current_status.m_blitzing = false;
            // Evaluate skills
            evaluate_skills(fd, &current_status, current_status.m_card->m_skills, SkillMod::on_activate);
            if(__builtin_expect(fd->end, false)) { break; }

            // Attack
            if(can_attack(&current_status))
            {
                current_status.m_step = CardStep::attacking;
                attack_phase(fd);
            }
            current_status.m_step = CardStep::attacked;
        }
        if(__builtin_expect(fd->end, false)) { break; }
        _DEBUG_MSG(1, "TURN %u ends for %s\n", fd->turn, status_description(&fd->tap->commander).c_str());
        std::swap(fd->tapi, fd->tipi);
        std::swap(fd->tap, fd->tip);
        ++fd->turn;
        fd->inc_counter(fd->achievement.misc_req, AchievementMiscReq::turns);
    }
    bool made_achievement = true;
    if(fd->optimization_mode == OptimizationMode::achievement)
    {
        for(unsigned i(0); made_achievement && i < fd->achievement.req_counter.size(); ++i)
        {
            made_achievement = made_achievement && fd->achievement.req_counter[i].check(fd->achievement_counter[i]);
        }
        if(debug_print)
        {
            print_achievement_results(fd);
        }
    }
    // you lose
    if(fd->players[0]->commander.m_hp == 0)
    {
        _DEBUG_MSG(1, "You lose.\n");
        return {0, 0, 1, 0, 0};
    }
    // you win in raid
    if(fd->optimization_mode == OptimizationMode::raid)
    {
        if(fd->players[1]->commander.m_hp == 0)
        {
            _DEBUG_MSG(1, "You win (boss killed).\n");
            return {1, 0, 0, fd->players[1]->commander.m_card->m_health + 50, 0};
        }
        else
        {
            _DEBUG_MSG(1, "You win (survival).\n");
            return {0, 1, 0, fd->players[1]->commander.m_card->m_health - fd->players[1]->commander.m_hp, 0};
        }
    }
    // you win
    if(fd->players[1]->commander.m_hp == 0)
    {
        if (fd->optimization_mode == OptimizationMode::achievement && !made_achievement)
        {
            _DEBUG_MSG(1, "You win but no achievement.\n");
            return {1, 0, 0, 0, 0};
        }
        _DEBUG_MSG(1, "You win.\n");
#if 0
        // ANP: Speedy if turn < last_decision + 10.
        bool speedy = fd->turn < fd->last_decision_turn + 10;
        if(fd->points_since_last_decision > 10)
        {
            fd->points_since_last_decision = 10;
        }
        return {1, 0, 0, 10 + (speedy ? 5 : 0) + (fd->gamemode == surge ? 20 : 0) + fd->points_since_last_decision, 0};
#endif
        return {1, 0, 0, 100, 0};
    }
    if (fd->turn > turn_limit)
    {
        _DEBUG_MSG(1, "Stall after %u turns.\n", turn_limit);
        if (fd->optimization_mode == OptimizationMode::defense)
        { return {1, 1, 0, 100, 0}; }
        else
        { return {0, 1, 0, 0, 0}; }
    }

    // Huh? How did we get here?
    assert(false);
    return {0, 0, 0, 0, 0};
}

// Roll a coin in case an Activation skill has 50% chance to proc.
template<Skill>
inline bool skill_roll(Field* fd)
{ return(true); }

template<>
inline bool skill_roll<jam>(Field* fd)
{ return(fd->flip()); }

// Check if a skill actually proc'ed.
template<Skill>
inline bool skill_check(Field* fd, CardStatus* c, CardStatus* ref)
{ return(true); }

template<>
inline bool skill_check<antiair>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(ref->m_card->m_flying);
}

template<>
inline bool skill_check<berserk>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(!c->m_sundered);
}

template<>
inline bool skill_check<blitz>(Field* fd, CardStatus* c, CardStatus* ref)
{
    unsigned opponent_player = opponent(c->m_player);
    return(fd->current_phase != Field::assaults_phase &&
            fd->players[opponent_player]->assaults.size() > c->m_index &&
            fd->players[opponent_player]->assaults[c->m_index].m_hp > 0 &&
            fd->players[opponent_player]->assaults[c->m_index].m_delay == 0);
}

template<>
inline bool skill_check<burst>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(ref->m_card->m_type == CardType::assault && ref->m_hp == ref->m_card->m_health);
}

template<>
inline bool skill_check<disease>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(!ref->m_diseased);
}

template<>
inline bool skill_check<evade>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(c->m_player != ref->m_player);
}

template<>
inline bool skill_check<flying>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(!ref->m_card->m_flying && !ref->m_card->m_antiair > 0);
}

// Not yet support on Attacked/on Death.
template<>
inline bool skill_check<immobilize>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(!ref->m_immobilized && !is_jammed(ref) && is_active_next_turn(ref));
}

template<>
inline bool skill_check<leech>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(can_be_healed(c));
}

template<>
inline bool skill_check<payback>(Field* fd, CardStatus* c, CardStatus* ref)
{
    // Never payback allied units (chaosed).
    return(ref->m_card->m_type == CardType::assault && c->m_player != ref->m_player && ref->m_hp > 0);
}

template<>
inline bool skill_check<phase>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(!ref->m_phased);
}

template<>
inline bool skill_check<refresh>(Field* fd, CardStatus* c, CardStatus* ref)
{ return(can_be_healed(c)); }

template<>
inline bool skill_check<regenerate>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(c->m_hp == 0 && !c->m_diseased);
}

template<>
inline bool skill_check<siphon>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(can_be_healed(&fd->players[c->m_player]->commander));
}

template<>
inline bool skill_check<stun>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(ref->m_card->m_type == CardType::assault);
}

template<>
inline bool skill_check<sunder>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(!ref->m_sundered);
}

template<>
inline bool skill_check<tribute>(Field* fd, CardStatus* c, CardStatus* ref)
{
    return(ref->m_card->m_type == CardType::assault && ref != c && ref->m_hp > 0);
}

template<>
inline bool skill_check<valor>(Field* fd, CardStatus* c, CardStatus* ref)
{
    unsigned count_ta(0);
    unsigned count_td(0);
    for(unsigned i(0); i < fd->tap->assaults.size(); ++i)
    {
        if(fd->tap->assaults[i].m_hp > 0) { ++count_ta; }
    }
    for(unsigned i(0); i < fd->tip->assaults.size(); ++i)
    {
        if(fd->tip->assaults[i].m_hp > 0) { ++count_td; }
    }
    return(count_ta < count_td);
}

template<Skill skill_id>
inline bool count_achievement(Field* fd, const CardStatus* c)
{
    if(c->m_player == 0)
    {
        fd->inc_counter(fd->achievement.skill_used, skill_id);
        if(skill_id != Skill::attack)
        {
            fd->inc_counter(fd->achievement.misc_req, AchievementMiscReq::skill_activated);
        }
    }
    return(true);
}

//------------------------------------------------------------------------------
// All the stuff that happens at the beginning of a turn, before a card is played
// returns true iff the card died.
inline void count_killed_achievements(Field* fd, const CardStatus* status)
{
    if(status->m_player == 1)
    {
        if(!status->m_is_summoned)
        {
            fd->inc_counter(fd->achievement.unit_type_killed, status->m_card->m_type);
        }
        if(status->m_card->m_flying)
        {
            fd->inc_counter(fd->achievement.misc_req, AchievementMiscReq::unit_with_flying_killed);
        }
    }
}
void remove_hp(Field* fd, CardStatus& status, unsigned dmg)
{
    assert(status.m_hp > 0);
    _DEBUG_MSG(2, "%s takes %u damage\n", status_description(&status).c_str(), dmg);
    status.m_hp = safe_minus(status.m_hp, dmg);
    if(status.m_hp == 0)
    {
        _DEBUG_MSG(1, "%s dies\n", status_description(&status).c_str());
        if(status.m_card->m_skills_on_death.size() > 0 || fd->effect == Effect::haunt)
        {
            fd->killed_with_on_death.push_back(&status);
        }
        if(status.m_card->m_regenerate > 0)
        {
            fd->killed_with_regen.push_back(&status);
        }
        else
        {
            count_killed_achievements(fd, &status);
        }
    }
}
inline bool is_it_dead(CardStatus& c)
{
    if(c.m_hp == 0) // yes it is
    {
        if(c.m_card->m_type != CardType::action)
        {
            _DEBUG_MSG(1, "Dead and removed: %s\n", status_description(&c).c_str());
        }
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
    unsigned old_hp = target->m_hp;
    target->m_hp = std::min(target->m_hp + v, target->m_card->m_health);
    if(fd->effect == Effect::invigorate && target->m_card->m_type == CardType::assault && skill_check<berserk>(fd, target, nullptr))
    {
        unsigned healed = target->m_hp - old_hp;
        target->m_berserk += healed;
    }
}
void check_regeneration(Field* fd)
{
    for(unsigned i(0); i < fd->killed_with_regen.size(); ++i)
    {
        CardStatus* status = fd->killed_with_regen[i];
        if(fd->flip() && skill_check<regenerate>(fd, status, nullptr))
        {
            count_achievement<regenerate>(fd, status);
            _DEBUG_MSG(1, "%s regenerates with %u health\n", status_description(status).c_str(), status->m_card->m_health);
            add_hp(fd, status, status->m_card->m_regenerate);
        }
        else
        {
            count_killed_achievements(fd, status);
        }
    }
    fd->killed_with_regen.clear();
}
void turn_start_phase(Field* fd)
{
    remove_dead(fd->tap->assaults);
    remove_dead(fd->tap->structures);
    remove_dead(fd->tip->assaults);
    remove_dead(fd->tip->structures);
    fd->fusion_count = 0;
    // Active player's assault cards:
    // update index
    // remove enfeeble, protect; apply poison damage, reduce delay
    {
        auto& assaults(fd->tap->assaults);
        for(unsigned index(0), end(assaults.size());
            index < end;
            ++index)
        {
            CardStatus& status(assaults[index]);
            status.m_index = index;
            status.m_enfeebled = 0;
            status.m_protected = 0;
            if(status.m_poisoned > 0)
            {
                _DEBUG_MSG(1, "%s takes poison damage\n", status_description(&status).c_str());
                remove_hp(fd, status, status.m_poisoned);
            }
            if(status.m_delay > 0 && !status.m_frozen)
            {
                _DEBUG_MSG(1, "%s reduces its timer\n", status_description(&status).c_str());
                --status.m_delay;
            }
            if(status.m_card->m_fusion && status.m_delay == 0) { ++fd->fusion_count; }
        }
    }
    // Active player's structure cards:
    // update index
    // reduce delay
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
            if(status.m_card->m_fusion && status.m_delay == 0) { ++fd->fusion_count; }
        }
    }
    // Defending player's assault cards:
    // update index
    // remove augment, chaos, freeze, immobilize, jam, rally, weaken, apply refresh
    // remove temp split
    {
        auto& assaults(fd->tip->assaults);
        for(unsigned index(0), end(assaults.size());
            index < end;
            ++index)
        {
            CardStatus& status(assaults[index]);
            status.m_index = index;
            status.m_augmented = 0;
            status.m_chaosed = false;
            status.m_enfeebled = 0;
            status.m_frozen = false;
            status.m_immobilized = false;
            status.m_jammed = false;
            status.m_phased = false;
            status.m_rallied = 0;
            if(status.m_stunned > 0) { -- status.m_stunned; }
            status.m_weakened = 0;
            status.m_temporary_split = false;
            status.m_step = CardStep::none;
            if(status.m_card->m_refresh)
            {
                check_and_perform_refresh(fd, &status);
            }
        }
    }
    // Defending player's structure cards:
    // update index
    // apply refresh
    {
        auto& structures(fd->tip->structures);
        for(unsigned index(0), end(structures.size());
            index < end;
            ++index)
        {
            CardStatus& status(structures[index]);
            status.m_index = index;
            if(status.m_card->m_refresh && fd->effect != Effect::impenetrable)
            {
                check_and_perform_refresh(fd, &status);
            }
        }
    }
    // Perform on death skills (from cards killed by poison damage)
    prepend_on_death(fd);
    resolve_skill(fd);
    // Regen from poison
    check_regeneration(fd);
}
void evaluate_legion(Field* fd)
{
    // Not subject to Mimic / Emulate / Augment
    // Not prevented by Jam / Freeze / Immobilize
    // Honor Infused faction
    auto& assaults = fd->tap->assaults;
    for(fd->current_ci = 0; fd->current_ci < assaults.size(); ++fd->current_ci)
    {
        CardStatus* status(&assaults[fd->current_ci]);
        unsigned legion_base = fd->effect == Effect::united_front ? status->m_card->m_delay : status->m_card->m_legion;
        if(legion_base == 0)
        {
            continue;
        }
        unsigned legion_size(0);
        legion_size += status->m_index > 0 && assaults[status->m_index - 1].m_hp > 0 && assaults[status->m_index - 1].m_faction == status->m_faction;
        legion_size += status->m_index + 1 < assaults.size() && assaults[status->m_index + 1].m_hp > 0 && assaults[status->m_index + 1].m_faction == status->m_faction;
        if(legion_size == 0)
        {
            continue;
        }
        // skill_check<legion>
        bool do_heal = can_be_healed(status);
        bool do_rally = status->m_hp > 0 && !status->m_sundered && (fd->tapi == status->m_player ? status->m_delay == 0 || status->m_blitzing : status->m_delay <= 1);
        if(!do_heal && !do_rally)
        {
            continue;
        }
        count_achievement<legion>(fd, status);
        unsigned legion_value = legion_base * legion_size;
        _DEBUG_MSG(1, "%s activates Legion %u, %s%s%s by %u\n", status_description(status).c_str(), legion_base,
                do_heal ? "healed" : "", do_heal && do_rally ? " and " : "", do_rally ? "rallied" : "", legion_value);
        if(do_heal)
        {
            add_hp(fd, status, legion_value);
        }
        if(do_rally)
        {
            status->m_rallied += legion_value;
        }
    }
}
//---------------------- $50 attack by assault card implementation -------------
// Counter damage dealt to the attacker (att) by defender (def)
// pre-condition: only valid if m_card->m_counter > 0
inline unsigned counter_damage(CardStatus* att, CardStatus* def)
{
    assert(att->m_card->m_type == CardType::assault);
    assert(def->m_card->m_type != CardType::action);
    return(safe_minus(def->m_card->m_counter + att->m_enfeebled, att->m_protected));
}
inline CardStatus* select_first_enemy_wall(Field* fd)
{
    for(unsigned i(0); i < fd->tip->structures.size(); ++i)
    {
        CardStatus& c(fd->tip->structures[i]);
        if(c.m_card->m_wall && c.m_hp > 0 && skill_check<wall>(fd, &c, nullptr))
        {
            count_achievement<wall>(fd, &c);
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
        fd->inc_counter(fd->achievement.misc_req, AchievementMiscReq::com_total, dmg);
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
        count_achievement<attack>(fd, att_status);
        // Evaluation order:
        // assaults only: fly check
        // modify damage
        // assaults only: immobilize
        // deal damage
        // assaults only: (siphon, poison, disease, sunder, phase, on_kill)
        // on_attacked: poison, disease, sunder, assaults only: berserk, skills
        // counter, berserk
        // assaults only: (crush, leech if still alive)
        // check regeneration
        if(def_status->m_card->m_flying && (fd->effect == Effect::high_skies || fd->flip()) && skill_check<flying>(fd, def_status, att_status))
        {
            count_achievement<flying>(fd, def_status);
            _DEBUG_MSG(1, "%s attacks %s but it dodges with Flying\n", status_description(att_status).c_str(), status_description(def_status).c_str());
            return;
        }

        modify_attack_damage<def_cardtype>(pre_modifier_dmg);
        if(att_status->m_player == 0)
        {
            fd->update_max_counter(fd->achievement.misc_req, AchievementMiscReq::damage, att_dmg);
        }

        // If Impenetrable, prevent attack damage against walls,
        // but still activate Counter!
        if(att_dmg > 0 && fd->effect == Effect::impenetrable && def_status->m_card->m_wall)
        {
            _DEBUG_MSG(1, "%s is impenetrable\n", status_description(def_status).c_str());
            att_dmg = 0;
        }
        if(att_dmg > 0)
        {
            immobilize<def_cardtype>();
            attack_damage<def_cardtype>();
            if(__builtin_expect(fd->end, false)) { return; }
            damage_dependant_pre_oa<def_cardtype>();
            on_kill<def_cardtype>();
        }
        on_attacked<def_cardtype>();
        if(att_dmg > 0)
        {
            if(att_status->m_hp > 0)
            {
               if(def_status->m_card->m_stun && skill_check<stun>(fd, def_status, att_status))
                {
                    count_achievement<stun>(fd, def_status);
                    // perform_skill_stun
                    _DEBUG_MSG(1, "%s stuns %s\n", status_description(def_status).c_str(), status_description(att_status).c_str());
                    att_status->m_stunned = 2;
                }
                if(def_status->m_card->m_counter > 0 && skill_check<counter>(fd, def_status, att_status))
                {
                    count_achievement<counter>(fd, def_status);
                    // perform_skill_counter
                    unsigned counter_dmg(counter_damage(att_status, def_status));
                    _DEBUG_MSG(1, "%s takes %u counter damage from %s\n", status_description(att_status).c_str(), counter_dmg, status_description(def_status).c_str());
                    remove_hp(fd, *att_status, counter_dmg);
                }
                if(att_status->m_card->m_berserk > 0 && skill_check<berserk>(fd, att_status, nullptr))
                {
                    count_achievement<berserk>(fd, att_status);
                    // perform_skill_berserk
                    att_status->m_berserk += att_status->m_card->m_berserk;
                }
            }
            crush_leech<def_cardtype>();
        }

        prepend_on_death(fd);
        resolve_skill(fd);
        check_regeneration(fd);
    }

    template<enum CardType::CardType>
    void modify_attack_damage(unsigned pre_modifier_dmg)
    {
        const Card& att_card(*att_status->m_card);
        const Card& def_card(*def_status->m_card);
        assert(att_card.m_type == CardType::assault);
        assert(pre_modifier_dmg > 0);
        att_dmg = pre_modifier_dmg;
        // enhance damage
        std::string desc;
        if(att_card.m_valor > 0 && skill_check<valor>(fd, att_status, nullptr))
        {
            count_achievement<valor>(fd, att_status);
            if(debug_print) { desc += "+" + to_string(att_card.m_valor) + "(valor)"; }
            att_dmg += att_card.m_valor;
        }
        if(att_card.m_antiair > 0 && skill_check<antiair>(fd, att_status, def_status))
        {
            count_achievement<antiair>(fd, att_status);
            if(debug_print) { desc += "+" + to_string(att_card.m_antiair) + "(antiair)"; }
            att_dmg += att_card.m_antiair;
        }
        if(att_card.m_burst > 0 && skill_check<burst>(fd, att_status, def_status))
        {
            count_achievement<burst>(fd, att_status);
            if(debug_print) { desc += "+" + to_string(att_card.m_burst) + "(burst)"; }
            att_dmg += att_card.m_burst;
        }
        if(def_status->m_enfeebled > 0)
        {
            if(debug_print) { desc += "+" + to_string(def_status->m_enfeebled) + "(enfeebled)"; }
            att_dmg += def_status->m_enfeebled;
        }
        // prevent damage
        std::string reduced_desc;
        unsigned reduced_dmg(0);
        unsigned armored_value(def_card.m_armored);
        if(armored_value == 0 && fd->effect == Effect::photon_shield && def_status->m_player == (fd->optimization_mode == OptimizationMode::defense ? 0u : 1u))
        {
            armored_value = 2;
        }
        if(armored_value > 0)
        {
            // Armored counts if not totally cancelled by Pierce. TODO how if Armored + Proteced > Pierce?
            if(armored_value > att_card.m_pierce)
            {
                count_achievement<armored>(fd, def_status);
            }
            if(debug_print) { reduced_desc += to_string(armored_value) + "(armored)"; }
            reduced_dmg += armored_value;
        }
        if(def_status->m_protected > 0)
        {
            if(debug_print) { reduced_desc += (reduced_desc.empty() ? "" : "+") + to_string(def_status->m_protected) + "(protected)"; }
            reduced_dmg += def_status->m_protected;
        }
        if(reduced_dmg > 0 && att_card.m_pierce > 0)
        {
            // TODO No Pierce achievement yet, so no count_achievement<pierce>(fd, att_status)
            if(debug_print) { reduced_desc += "-" + to_string(att_card.m_pierce) + "(pierce)"; }
            reduced_dmg = safe_minus(reduced_dmg, att_card.m_pierce);
        }
        att_dmg = safe_minus(att_dmg, reduced_dmg);
        if(debug_print)
        {
            if(!reduced_desc.empty()) { desc += "-[" + reduced_desc + "]"; }
            if(!desc.empty()) { desc += "=" + to_string(att_dmg); }
            _DEBUG_MSG(1, "%s attacks %s for %u%s damage\n", status_description(att_status).c_str(), status_description(def_status).c_str(), pre_modifier_dmg, desc.c_str());
        }
    }

    template<enum CardType::CardType>
    void immobilize() {}

    template<enum CardType::CardType>
    void attack_damage()
    {
        remove_hp(fd, *def_status, att_dmg);
        killed_by_attack = def_status->m_hp == 0;
    }

    template<enum CardType::CardType>
    void damage_dependant_pre_oa() {}

    template<enum CardType::CardType>
    void on_kill() {}

    template<enum CardType::CardType def_cardtype>
    void on_attacked()
    {
        if(def_status->m_card->m_poison_oa > att_status->m_poisoned && skill_check<poison>(fd, def_status, att_status))
        {
            count_achievement<poison>(fd, def_status);
            unsigned v = def_status->m_card->m_poison_oa;
            _DEBUG_MSG(1, "%s (on attacked) poisons %s by %u\n", status_description(def_status).c_str(), status_description(att_status).c_str(), v);
            att_status->m_poisoned = v;
        }
        if(def_status->m_card->m_disease_oa && skill_check<disease>(fd, def_status, att_status))
        {
            count_achievement<disease>(fd, def_status);
            // perform_skill_disease
            _DEBUG_MSG(1, "%s (on attacked) diseases %s\n", status_description(def_status).c_str(), status_description(att_status).c_str());
            att_status->m_diseased = true;
        }
        if(def_status->m_hp > 0 && def_status->m_card->m_berserk_oa > 0 && skill_check<berserk>(fd, def_status, nullptr))
        {
            count_achievement<berserk>(fd, def_status);
            def_status->m_berserk += def_status->m_card->m_berserk_oa;
        }
        if(def_status->m_card->m_sunder_oa && skill_check<sunder>(fd, def_status, att_status))
        {
            count_achievement<sunder>(fd, def_status);
            // perform_skill_sunder
            _DEBUG_MSG(1, "%s (on attacked) sunders %s\n", status_description(def_status).c_str(), status_description(att_status).c_str());
            att_status->m_sundered = true;
        }
        evaluate_skills(fd, def_status, def_status->m_card->m_skills_on_attacked, SkillMod::on_attacked);
    }

    template<enum CardType::CardType>
    void crush_leech() {}
};

template<>
void PerformAttack::immobilize<CardType::assault>()
{
    if(att_status->m_card->m_immobilize && fd->flip() && skill_check<Skill::immobilize>(fd, att_status, def_status))
    {
        count_achievement<Skill::immobilize>(fd, att_status);
        _DEBUG_MSG(1, "%s immobilizes %s\n", status_description(att_status).c_str(), status_description(def_status).c_str());
        def_status->m_immobilized = true;
    }
}

template<>
void PerformAttack::attack_damage<CardType::commander>()
{
    remove_commander_hp(fd, *def_status, att_dmg, true);
}

template<>
void PerformAttack::damage_dependant_pre_oa<CardType::assault>()
{
    if(att_status->m_card->m_siphon > 0 && skill_check<siphon>(fd, att_status, def_status))
    {
        count_achievement<siphon>(fd, att_status);
        // perform_skill_siphon
        unsigned v = std::min(att_dmg, att_status->m_card->m_siphon);
        _DEBUG_MSG(1, "%s siphons %u health for %s\n", status_description(att_status).c_str(), v, status_description(&fd->tap->commander).c_str());
        add_hp(fd, &fd->tap->commander, v);
    }
    if(att_status->m_card->m_poison > def_status->m_poisoned && skill_check<poison>(fd, att_status, def_status))
    {
        count_achievement<poison>(fd, att_status);
        // perform_skill_poison
        unsigned v = att_status->m_card->m_poison;
        _DEBUG_MSG(1, "%s poisons %s by %u\n", status_description(att_status).c_str(), status_description(def_status).c_str(), v);
        def_status->m_poisoned = v;
    }
    if(att_status->m_card->m_disease && skill_check<disease>(fd, att_status, def_status))
    {
        count_achievement<disease>(fd, att_status);
        // perform_skill_disease
        _DEBUG_MSG(1, "%s diseases %s\n", status_description(att_status).c_str(), status_description(def_status).c_str());
        def_status->m_diseased = true;
    }
    if(att_status->m_card->m_sunder && skill_check<sunder>(fd, att_status, def_status))
    {
        count_achievement<sunder>(fd, att_status);
        // perform_skill_sunder
        _DEBUG_MSG(1, "%s sunders %s\n", status_description(att_status).c_str(), status_description(def_status).c_str());
        def_status->m_sundered = true;
    }
    if(att_status->m_card->m_phase && skill_check<phase>(fd, att_status, def_status))
    {
        count_achievement<phase>(fd, att_status);
        // perform_skill_phase
        _DEBUG_MSG(1, "%s phases %s\n", status_description(att_status).c_str(), status_description(def_status).c_str());
        def_status->m_phased = true;
    }
}

template<>
void PerformAttack::on_kill<CardType::assault>()
{
    if(killed_by_attack)
    {
        evaluate_skills(fd, att_status, att_status->m_card->m_skills_on_kill, SkillMod::on_kill);
    }
}

template<>
void PerformAttack::crush_leech<CardType::assault>()
{
    if(att_status->m_card->m_crush > 0 && killed_by_attack && skill_check<crush>(fd, att_status, nullptr))
    {
        count_achievement<crush>(fd, att_status);
        // perform_skill_crush
        CardStatus* def_status{select_first_enemy_wall(fd)}; // defending wall
        if (def_status != nullptr)
        {
            _DEBUG_MSG(1, "%s crushes %s for %u damage\n", status_description(att_status).c_str(), status_description(def_status).c_str(), att_status->m_card->m_crush);
            remove_hp(fd, *def_status, att_status->m_card->m_crush);
        }
        else
        {
            _DEBUG_MSG(1, "%s crushes %s for %u damage\n", status_description(att_status).c_str(), status_description(&fd->tip->commander).c_str(), att_status->m_card->m_crush);
            remove_commander_hp(fd, fd->tip->commander, att_status->m_card->m_crush, true);
        }
    }
    if(att_status->m_card->m_leech > 0 && skill_check<leech>(fd, att_status, nullptr))
    {
        count_achievement<leech>(fd, att_status);
        _DEBUG_MSG(1, "%s leeches %u health\n", status_description(att_status).c_str(), std::min(att_dmg, att_status->m_card->m_leech));
        add_hp(fd, att_status, std::min(att_dmg, att_status->m_card->m_leech));
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
void attack_phase(Field* fd)
{
    CardStatus* att_status(&fd->tap->assaults[fd->current_ci]); // attacking card
    Storage<CardStatus>& def_assaults(fd->tip->assaults);
    if(attack_power(att_status) == 0) { return; }
    unsigned num_attacks(1);
    if(att_status->m_card->m_flurry > 0 && fd->flip() && skill_check<flurry>(fd, att_status, nullptr))
    {
        count_achievement<flurry>(fd, att_status);
        _DEBUG_MSG(1, "%s activates Flurry\n", status_description(att_status).c_str());
        num_attacks += att_status->m_card->m_flurry;
    }
    for(unsigned attack_index(0); attack_index < num_attacks && can_attack(att_status) && fd->tip->commander.m_hp > 0; ++attack_index)
    {
        // 3 possibilities:
        // - 1. attack against the assault in front
        // - 2. swipe attack the assault in front and adjacent assaults if any
        //      * Attack Commander/wall if opposing Assault is already dead before Swipe finishes (Swipe will attempt to attack the third Assault)
        //        See http://www.kongregate.com/forums/65-tyrant/topics/289416?page=22#posts-6861970
        // - 3. attack against the commander or walls (if there is no assault or if the attacker has the fear attribute)
        // Check if attack mode is 1. or 2. (there is a living assault card in front, and no fear)
        if(alive_assault(def_assaults, fd->current_ci) && !(att_status->m_card->m_fear && skill_check<fear>(fd, att_status, nullptr) && count_achievement<fear>(fd, att_status)))
        {
            // attack mode 1.
            if(!(att_status->m_card->m_swipe && skill_check<swipe>(fd, att_status, nullptr) && count_achievement<swipe>(fd, att_status)))
            {
                PerformAttack{fd, att_status, &fd->tip->assaults[fd->current_ci]}.op<CardType::assault>();
            }
            // attack mode 2.
            else
            {
                // perform_skill_swipe
                _DEBUG_MSG(1, "%s activates Swipe\n", status_description(att_status).c_str());
                // attack the card on the left
                if(fd->current_ci > 0 && alive_assault(def_assaults, fd->current_ci - 1))
                {
                    PerformAttack{fd, att_status, &fd->tip->assaults[fd->current_ci-1]}.op<CardType::assault>();
                }
                if(fd->end || !can_attack(att_status)) { return; }
                // attack the card in front (or attacks the commander if the card in front is just died)
                if(alive_assault(def_assaults, fd->current_ci))
                {
                    PerformAttack{fd, att_status, &fd->tip->assaults[fd->current_ci]}.op<CardType::assault>();
                }
                else
                {
                    attack_commander(fd, att_status);
                }
                if(fd->end || !can_attack(att_status)) { return; }
                // attack the card on the right
                if(alive_assault(def_assaults, fd->current_ci + 1))
                {
                    PerformAttack{fd, att_status, &fd->tip->assaults[fd->current_ci+1]}.op<CardType::assault>();
                }
            }
        }
        // attack mode 3.
        else
        {
            attack_commander(fd, att_status);
        }
    }
}

//---------------------- $65 active skills implementation ----------------------
unsigned strike_damage(CardStatus* target, unsigned v)
{
    return(safe_minus(v + target->m_enfeebled, target->m_protected));
}

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
inline bool skill_predicate(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{ assert(false); return(false); }

template<>
inline bool skill_predicate<augment>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{
    const auto& mod = std::get<4>(s);
    if(can_act(c) &&  // (fd->tapi == c->m_player ? is_active(c) && !is_attacking_or_has_attacked(c) : is_active_next_turn(c)))
        (src->m_player != c->m_player || mod == SkillMod::on_death ? (fd->tapi == c->m_player ? is_active(c) && !is_attacking_or_has_attacked(c) : is_active_next_turn(c)) :
         mod == SkillMod::on_attacked ? is_active_next_turn(c) :
         is_active(c) && !is_attacking_or_has_attacked(c)))
    {
        for(auto& s: c->m_card->m_skills)
        {
            // Any quantifiable skill except augment
            if(std::get<1>(s) > 0 && std::get<0>(s) != augment && std::get<0>(s) != summon) { return(true); }
        }
        bool need_add_skill = true;
        auto mod = SkillMod::on_activate;
        if(may_change_skill(fd, c, mod))
        {
            auto s = apply_battleground_effect(fd, c, SkillSpec(new_skill, 0, allfactions, false, mod), mod, need_add_skill);
            assert(std::get<0>(s) != new_skill);
            if(std::get<1>(s) > 0 && std::get<0>(s) != augment && std::get<0>(s) != summon) { return(true); }
        }
    }
    return(false);
}

template<>
inline bool skill_predicate<chaos>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{
    const auto& mod = std::get<4>(s);
    return(!c->m_chaosed && can_act(c) &&  // (fd->tapi == c->m_player ? is_active(c) && !is_attacking_or_has_attacked(c) : is_active_next_turn(c)));
            (mod == SkillMod::on_attacked ? is_active(c) && c->m_index > fd->current_ci :
             mod == SkillMod::on_death ? c->m_index >= src->m_index && (fd->tapi != src->m_player ? is_active(c) : is_active_next_turn(c)) :
             is_active(c) || is_active_next_turn(c)));
}

template<>
inline bool skill_predicate<cleanse>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{
    return(fd->effect != Effect::decay &&
            c->m_hp > 0 && (
               c->m_chaosed ||
               c->m_diseased ||
               c->m_enfeebled > 0 ||
               (c->m_frozen && c->m_delay == 0) ||
               c->m_immobilized ||
               c->m_jammed ||
               c->m_poisoned ||
               c->m_stunned ||
               c->m_sundered
               ));
}

template<>
inline bool skill_predicate<enfeeble>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{ return(c->m_hp > 0); }

template<>
inline bool skill_predicate<freeze>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{ return(c->m_hp > 0 && !c->m_jammed && !c->m_frozen); }

template<>
inline bool skill_predicate<heal>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{ return(can_be_healed(c)); }

template<>
inline bool skill_predicate<infuse>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{ return(c->m_faction != bloodthirsty); }

template<>
inline bool skill_predicate<jam>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{
    const auto& mod = std::get<4>(s);
    return(can_act(c) &&  // (fd->tapi == c->m_player ? is_active(c) && !is_attacking_or_has_attacked(c) : is_active_next_turn(c)));
            (mod == SkillMod::on_attacked ? is_active(c) && c->m_index > fd->current_ci :
             mod == SkillMod::on_death ? c->m_index >= src->m_index && (fd->tapi != src->m_player ? is_active(c) : is_active_next_turn(c)) :
             is_active(c) || is_active_next_turn(c)));
}

template<>
inline bool skill_predicate<mimic>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{ return(c->m_hp > 0); }

template<>
inline bool skill_predicate<protect>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{ return(c->m_hp > 0); }

template<>
inline bool skill_predicate<rally>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{
    const auto& mod = std::get<4>(s);
    return(can_attack(c) && !c->m_sundered &&
        (src->m_player != c->m_player || mod == SkillMod::on_death ? (fd->tapi == c->m_player ? is_active(c) && !is_attacking_or_has_attacked(c) : is_active_next_turn(c)) :
         mod == SkillMod::on_attacked ? is_active_next_turn(c) :
         is_active(c) && !is_attacking_or_has_attacked(c)));
}

template<>
inline bool skill_predicate<repair>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{ return(can_be_healed(c)); }

template<>
inline bool skill_predicate<rush>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{ return(c->m_delay > 0); }

template<>
inline bool skill_predicate<siege>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{ return(c->m_hp > 0); }

template<>
inline bool skill_predicate<strike>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{ return(c->m_hp > 0); }

template<>
inline bool skill_predicate<supply>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{ return(can_be_healed(c)); }

template<>
inline bool skill_predicate<weaken>(Field* fd, CardStatus* src, CardStatus* c, const SkillSpec& s)
{
    const auto& mod = std::get<4>(s);
    return(can_act(c) && !c->m_immobilized && !c->m_stunned && attack_power(c) > 0
        && (mod == SkillMod::on_attacked ? is_active(c) && c->m_index > fd->current_ci :
             mod == SkillMod::on_death ? c->m_index >= src->m_index && (fd->tapi != src->m_player ? is_active(c) : is_active_next_turn(c)) :
             is_active(c) || is_active_next_turn(c)));
}

template<unsigned skill_id>
inline void perform_skill(Field* fd, CardStatus* c, unsigned v)
{ assert(false); }

template<>
inline void perform_skill<augment>(Field* fd, CardStatus* c, unsigned v)
{
    c->m_augmented += v;
}

template<>
inline void perform_skill<backfire>(Field* fd, CardStatus* c, unsigned v)
{
    // backfire damage counts in ARD.
    remove_commander_hp(fd, *c, v, true);
}

template<>
inline void perform_skill<chaos>(Field* fd, CardStatus* c, unsigned v)
{
    c->m_chaosed = true;
}

template<>
inline void perform_skill<cleanse>(Field* fd, CardStatus* c, unsigned v)
{
    c->m_chaosed = false;
    c->m_diseased = false;
    c->m_enfeebled = 0;
    c->m_frozen = false;
    c->m_immobilized = false;
    c->m_jammed = false;
    c->m_poisoned = 0;
    c->m_stunned = 0;
    c->m_sundered = false;
}

template<>
inline void perform_skill<enfeeble>(Field* fd, CardStatus* c, unsigned v)
{
    c->m_enfeebled += v;
}

template<>
inline void perform_skill<freeze>(Field* fd, CardStatus* c, unsigned v)
{
    c->m_frozen = true;
}

template<>
inline void perform_skill<heal>(Field* fd, CardStatus* c, unsigned v)
{
    add_hp(fd, c, v);
}

template<>
inline void perform_skill<infuse>(Field* fd, CardStatus* c, unsigned v)
{
    c->m_faction = bloodthirsty;
    c->m_infused = true;
}

template<>
inline void perform_skill<jam>(Field* fd, CardStatus* c, unsigned v)
{
    c->m_jammed = true;
}

template<>
inline void perform_skill<protect>(Field* fd, CardStatus* c, unsigned v)
{
    c->m_protected += v;
}

template<>
inline void perform_skill<rally>(Field* fd, CardStatus* c, unsigned v)
{
    c->m_rallied += v;
}

template<>
inline void perform_skill<repair>(Field* fd, CardStatus* c, unsigned v)
{
    add_hp(fd, c, v);
}

template<>
inline void perform_skill<rush>(Field* fd, CardStatus* c, unsigned v)
{
    c->m_delay = safe_minus(c->m_delay, v);
}

template<>
inline void perform_skill<shock>(Field* fd, CardStatus* c, unsigned v)
{
    // shock damage counts in ARD. (if attacker ever has the skill)
    remove_commander_hp(fd, *c, v, true);
}

template<>
inline void perform_skill<siege>(Field* fd, CardStatus* c, unsigned v)
{
    remove_hp(fd, *c, v);
}

template<>
inline void perform_skill<strike>(Field* fd, CardStatus* c, unsigned v)
{
    remove_hp(fd, *c, strike_damage(c, v));
}

template<>
inline void perform_skill<supply>(Field* fd, CardStatus* c, unsigned v)
{
    add_hp(fd, c, v);
}

template<>
inline void perform_skill<weaken>(Field* fd, CardStatus* c, unsigned v)
{
    c->m_weakened += v;
}

template<unsigned skill_id>
inline unsigned select_fast(Field* fd, CardStatus* src_status, const std::vector<CardStatus*>& cards, const SkillSpec& s, bool is_helpful_skill)
{
    if(std::get<2>(s) == allfactions)
    {
        return(fd->make_selection_array(cards.begin(), cards.end(),
                [fd, src_status, s, is_helpful_skill](CardStatus* c) {
                    return(!(is_helpful_skill && c->m_phased) && skill_predicate<skill_id>(fd, src_status, c, s));
                }));
    }
    else
    {
        return(fd->make_selection_array(cards.begin(), cards.end(), [fd, src_status, s, is_helpful_skill](CardStatus* c){return(c->m_faction == std::get<2>(s) && !(is_helpful_skill && c->m_phased) && skill_predicate<skill_id>(fd, src_status, c, s));}));
    }
}

template<>
inline unsigned select_fast<supply>(Field* fd, CardStatus* src_status, const std::vector<CardStatus*>& cards, const SkillSpec& s, bool is_helpful_skill)
{
    // mimiced supply by a structure, etc ?
    if(!(src_status->m_card->m_type == CardType::assault)) { return(0); }
    const unsigned min_index(src_status->m_index - (src_status->m_index == 0 ? 0 : 1));
    const unsigned max_index(src_status->m_index + (src_status->m_index == cards.size() - 1 ? 0 : 1));
    return(fd->make_selection_array(cards.begin() + min_index, cards.begin() + max_index + 1, [fd, src_status, s, is_helpful_skill](CardStatus* c){return(!(is_helpful_skill && c->m_phased) && skill_predicate<supply>(fd, src_status, c, s));}));
}

inline std::vector<CardStatus*>& skill_targets_hostile_assault(Field* fd, CardStatus* src_status)
{
    return(fd->players[src_status->m_chaosed ? src_status->m_player : opponent(src_status->m_player)]->assaults.m_indirect);
}

inline std::vector<CardStatus*>& skill_targets_allied_assault(Field* fd, CardStatus* src_status)
{
    return(fd->players[src_status->m_player]->assaults.m_indirect);
}

inline std::vector<CardStatus*>& skill_targets_hostile_structure(Field* fd, CardStatus* src_status)
{
    return(fd->players[src_status->m_chaosed ? src_status->m_player : opponent(src_status->m_player)]->structures.m_indirect);
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

template<> inline std::vector<CardStatus*>& skill_targets<augment>(Field* fd, CardStatus* src_status)
{ return(skill_targets_allied_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<chaos>(Field* fd, CardStatus* src_status)
{ return(skill_targets_hostile_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<cleanse>(Field* fd, CardStatus* src_status)
{ return(skill_targets_allied_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<enfeeble>(Field* fd, CardStatus* src_status)
{ return(skill_targets_hostile_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<freeze>(Field* fd, CardStatus* src_status)
{ return(skill_targets_hostile_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<heal>(Field* fd, CardStatus* src_status)
{ return(skill_targets_allied_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<jam>(Field* fd, CardStatus* src_status)
{ return(skill_targets_hostile_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<mimic>(Field* fd, CardStatus* src_status)
{
    if(fd->effect == Effect::copycat)
    { return(skill_targets_allied_assault(fd, src_status)); }
    else
    { return(skill_targets_hostile_assault(fd, src_status)); }
}

template<> std::vector<CardStatus*>& skill_targets<protect>(Field* fd, CardStatus* src_status)
{ return(skill_targets_allied_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<rally>(Field* fd, CardStatus* src_status)
{ return(skill_targets_allied_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<repair>(Field* fd, CardStatus* src_status)
{ return(skill_targets_allied_structure(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<rush>(Field* fd, CardStatus* src_status)
{ return(skill_targets_allied_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<strike>(Field* fd, CardStatus* src_status)
{ return(skill_targets_hostile_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<supply>(Field* fd, CardStatus* src_status)
{ return(skill_targets_allied_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<weaken>(Field* fd, CardStatus* src_status)
{ return(skill_targets_hostile_assault(fd, src_status)); }

template<> std::vector<CardStatus*>& skill_targets<siege>(Field* fd, CardStatus* src_status)
{ return(skill_targets_hostile_structure(fd, src_status)); }

template<typename T>
void maybeTriggerRegen(Field* fd)
{
}

template<>
void maybeTriggerRegen<true_>(Field* fd)
{
    fd->skill_queue.emplace_front(nullptr, std::make_tuple(trigger_regen, 0, allfactions, false, SkillMod::on_activate));
}

CardStatus* select_interceptable(Field* fd, CardStatus* src_status, unsigned index)
{
    CardStatus* status(fd->selection_array[index]);
    // do not intercept skills from allied units (Chaosed / Infuse)
    if(src_status->m_player == status->m_player)
    {
        return(fd->selection_array[index]);
    }
    if(index > 0)
    {
        CardStatus* left_status(fd->selection_array[index - 1]);
        if(left_status->m_card->m_intercept && left_status->m_index == status->m_index - 1 && left_status->m_player == status->m_player && skill_check<intercept>(fd, left_status, status))
        {
            count_achievement<intercept>(fd, left_status);
            _DEBUG_MSG(1, "%s intercepts for %s\n", status_description(left_status).c_str(), status_description(status).c_str());
            return(fd->selection_array[index - 1]);
        }
    }
    if(index + 1 < fd->selection_array.size())
    {
        CardStatus* right_status(fd->selection_array[index + 1]);
        if(right_status->m_card->m_intercept && right_status->m_index == status->m_index + 1 && right_status->m_player == status->m_player && skill_check<intercept>(fd, right_status, status))
        {
            count_achievement<intercept>(fd, right_status);
            _DEBUG_MSG(1, "%s intercepts for %s\n", status_description(right_status).c_str(), status_description(status).c_str());
            return(fd->selection_array[index + 1]);
        }
    }
    return(fd->selection_array[index]);
}

template<Skill skill_id>
bool check_and_perform_skill(Field* fd, CardStatus* src_status, CardStatus* dst_status, const SkillSpec& s, bool is_evadable, bool is_count_achievement)
{
    if(skill_check<skill_id>(fd, src_status, dst_status))
    {
        if(is_evadable && (dst_status->m_card->m_evade || (fd->effect == Effect::quicksilver && dst_status->m_card->m_type == CardType::assault)) && fd->flip() && skill_check<evade>(fd, dst_status, src_status))
        {
            count_achievement<evade>(fd, dst_status);
            _DEBUG_MSG(1, "%s %s (%u) on %s but it evades\n", status_description(src_status).c_str(), skill_names[skill_id].c_str(), std::get<1>(s), status_description(dst_status).c_str());
            return(false);
        }
        if(is_count_achievement)
        {
            count_achievement<skill_id>(fd, src_status);
        }
        _DEBUG_MSG(1, "%s %s (%u) on %s\n", status_description(src_status).c_str(), skill_names[skill_id].c_str(), std::get<1>(s), status_description(dst_status).c_str());
        perform_skill<skill_id>(fd, dst_status, std::get<1>(s));
        return(true);
    }
    return(false);
}

bool check_and_perform_blitz(Field* fd, CardStatus* src_status)
{
    if(skill_check<blitz>(fd, src_status, nullptr))
    {
        count_achievement<blitz>(fd, src_status);
        _DEBUG_MSG(1, "%s activates Blitz opposing %s\n", status_description(src_status).c_str(), status_description(&fd->tip->assaults[src_status->m_index]).c_str());
        src_status->m_blitzing = true;
        return(true);
    }
    return(false);
}

bool check_and_perform_recharge(Field* fd, CardStatus* src_status)
{
    if(fd->flip() && skill_check<recharge>(fd, src_status, nullptr))
    {
        count_achievement<recharge>(fd, src_status);
        _DEBUG_MSG(1, "%s activates Recharge\n", status_description(src_status).c_str());
        fd->tap->deck->place_at_bottom(src_status->m_card);
        return(true);
    }
    return(false);
}

bool check_and_perform_refresh(Field* fd, CardStatus* src_status)
{
    if(skill_check<refresh>(fd, src_status, nullptr))
    {
        count_achievement<refresh>(fd, src_status);
        _DEBUG_MSG(1, "%s refreshes, hp -> %u\n", status_description(src_status).c_str(), src_status->m_card->m_health);
        add_hp(fd, src_status, src_status->m_card->m_health);
        return(true);
    }
    return(false);
}

template<Skill skill_id>
void perform_targetted_hostile_fast(Field* fd, CardStatus* src_status, const SkillSpec& s)
{
    std::vector<CardStatus*>& cards(skill_targets<skill_id>(fd, src_status));
    if(select_fast<skill_id>(fd, src_status, cards, s, false) == 0)
    {
        return;
    }
    _DEBUG_SELECTION("%s", skill_names[skill_id].c_str());
    unsigned index_start, index_end;
    if(std::get<3>(s)) // target all
    {
        index_start = 0;
        index_end = fd->selection_array.size() - 1;
    }
    else
    {
        index_start = index_end = fd->rand(0, fd->selection_array.size() - 1);
    }
    bool is_count_achievement(true);
    for(unsigned s_index(index_start); s_index <= index_end; ++s_index)
    {
        if(!skill_roll<skill_id>(fd))
        {
            _DEBUG_MSG(2, "%s misses the 50%% chance to activate %s (%u) on %s\n", status_description(src_status).c_str(), skill_names[skill_id].c_str(), std::get<1>(s), status_description(fd->selection_array[s_index]).c_str());
            continue;
        }
        CardStatus* c(std::get<3>(s) ? fd->selection_array[s_index] : select_interceptable(fd, src_status, s_index));
        if(check_and_perform_skill<skill_id>(fd, src_status, c, s, true, is_count_achievement))
        {
            // Count at most once even targeting "All"
            is_count_achievement = false;
            // Payback
            if(c->m_card->m_payback && skill_predicate<skill_id>(fd, src_status, src_status, s) && fd->flip() && skill_check<payback>(fd, c, src_status) && skill_check<skill_id>(fd, src_status, c))
            {
                count_achievement<payback>(fd, c);
                _DEBUG_MSG(1, "%s paybacks (%s %u) on %s\n", status_description(c).c_str(), skill_names[skill_id].c_str(), std::get<1>(s), status_description(src_status).c_str());
                perform_skill<skill_id>(fd, src_status, std::get<1>(s));
            }
        }
    }
    maybeTriggerRegen<typename skillTriggersRegen<skill_id>::T>(fd);
    prepend_on_death(fd);
}

template<Skill skill_id>
inline void check_and_perform_emulate(Field* fd, CardStatus* src_status, CardStatus* opposite_status, const SkillSpec& s)
{
    Hand* hand = fd->players[opponent(opposite_status->m_player)];
    if(hand->assaults.size() > opposite_status->m_index)
    {
        CardStatus& emulator = hand->assaults[opposite_status->m_index];
        if(emulator.m_card->m_emulate && skill_predicate<skill_id>(fd, src_status, &emulator, s) && skill_check<emulate>(fd, &emulator, nullptr))
        {
            count_achievement<emulate>(fd, &emulator);
            _DEBUG_MSG(1, "Emulate (%s %u) on %s\n", skill_names[skill_id].c_str(), std::get<1>(s), status_description(&emulator).c_str());
            perform_skill<skill_id>(fd, &emulator, std::get<1>(s));
        }
    }
}

template<Skill skill_id>
void perform_targetted_allied_fast(Field* fd, CardStatus* src_status, const SkillSpec& s)
{
    std::vector<CardStatus*>& cards(skill_targets<skill_id>(fd, src_status));
    if(select_fast<skill_id>(fd, src_status, cards, s, true) == 0)
    {
        return;
    }
    _DEBUG_SELECTION("%s", skill_names[skill_id].c_str());
    unsigned index_start, index_end;
    if(std::get<3>(s) || skill_id == supply) // target all or supply
    {
        index_start = 0;
        index_end = fd->selection_array.size() - 1;
    }
    else
    {
        index_start = index_end = fd->rand(0, fd->selection_array.size() - 1);
    }
    bool is_count_achievement(true);
    for(unsigned s_index(index_start); s_index <= index_end; ++s_index)
    {
        // So far no friendly activation skill needs to roll 50% but check it for completeness.
        if(!skill_roll<skill_id>(fd))
        {
            _DEBUG_MSG(2, "%s misses the 50%% chance to %s (%u) on %s\n", status_description(src_status).c_str(), skill_names[skill_id].c_str(), std::get<1>(s), status_description(fd->selection_array[s_index]).c_str());
            continue;
        }
        CardStatus* c(fd->selection_array[s_index]);
        if(check_and_perform_skill<skill_id>(fd, src_status, c, s, false, is_count_achievement))
        {
            // Count at most once even targeting "All"
            is_count_achievement = false;
            // Tribute
            if(c->m_card->m_tribute && skill_predicate<skill_id>(fd, src_status, src_status, s) && fd->flip() && skill_check<tribute>(fd, c, src_status))
            {
                count_achievement<tribute>(fd, c);
                _DEBUG_MSG(1, "Tribute (%s %u) on %s\n", skill_names[skill_id].c_str(), std::get<1>(s), status_description(src_status).c_str());
                perform_skill<skill_id>(fd, src_status, std::get<1>(s));
                check_and_perform_emulate<skill_id>(fd, src_status, src_status, s);
            }
            check_and_perform_emulate<skill_id>(fd, src_status, c, s);
        }
    }
}

void perform_backfire(Field* fd, CardStatus* src_status, const SkillSpec& s)
{
    check_and_perform_skill<backfire>(fd, src_status, &fd->players[src_status->m_player]->commander, s, false, true);
}

void perform_infuse(Field* fd, CardStatus* src_status, const SkillSpec& s)
{
    const auto &cards = boost::join(fd->tap->assaults.m_indirect, fd->tip->assaults.m_indirect);
    if(fd->make_selection_array(cards.begin(), cards.end(),
                                [fd, s](CardStatus* c) {
                                    return(skill_predicate<infuse>(fd, &fd->tap->commander, c, s));
                                }) > 0)
    {
        _DEBUG_SELECTION("%s", skill_names[infuse].c_str());
        CardStatus* c(select_interceptable(fd, src_status, fd->rand(0, fd->selection_array.size() - 1)));
        check_and_perform_skill<infuse>(fd, src_status, c, s, true, true);
    }
}

inline void perform_recharge(Field* fd, CardStatus* src_status, const SkillSpec& s)
{
    check_and_perform_recharge(fd, src_status);
}

// a summoned card's on play skills seem to be evaluated before any other skills on the skill queue.
inline void prepend_skills(Field* fd, CardStatus* status)
{
    for(auto& skill: boost::adaptors::reverse(status->m_card->m_skills_on_play))
    {
        fd->skill_queue.emplace_front(status, skill);
    }
}

template<Skill skill_id>
void perform_summon(Field* fd, CardStatus* src_status, const SkillSpec& s);

void perform_split(Field* fd, CardStatus* src_status, const SkillSpec& s)
{
    perform_summon<split>(fd, src_status, SkillSpec(summon, src_status->m_card->m_id, std::get<2>(s), std::get<3>(s), std::get<4>(s)));
}

template<Skill skill_id>
void perform_summon(Field* fd, CardStatus* src_status, const SkillSpec& s)
{
    unsigned player = src_status->m_player;
    const auto& mod = std::get<4>(s);
    // Split and Summon on Play are not counted towards the Summon Limit.
    if(skill_id == summon && mod != SkillMod::on_play)
    {
        if(fd->players[player]->available_summons == 0)
        {
            return;
        }
        -- fd->players[player]->available_summons;
        if(fd->players[player]->available_summons == 0)
        {
            _DEBUG_MSG(1, "** Reaching summon limit, this is the last summon.\n");
        }
    }
    unsigned summoned_id = std::get<1>(s);
    const Card* summoned = 0;
    if(summoned_id != 0)
    {
        summoned = fd->cards.by_id(summoned_id);
    }
    else
    {
        Faction summond_faction = std::get<2>(s);
        do {
            summoned = fd->random_in_vector(fd->cards.player_assaults);
        } while(summond_faction != allfactions && summond_faction != summoned->m_faction);
    }
    assert(summoned->m_type == CardType::assault || summoned->m_type == CardType::structure);
    Hand* hand{fd->players[player]};
    count_achievement<summon>(fd, src_status);
    Storage<CardStatus>* storage{summoned->m_type == CardType::assault ? &hand->assaults : &hand->structures};
    CardStatus& card_status(storage->add_back());
    card_status.set(summoned);
    card_status.m_index = storage->size() - 1;
    card_status.m_player = player;
    if(summoned->m_type == CardType::assault && fd->effect == Effect::harsh_conditions)
    {
        ++card_status.m_delay;
    }
    card_status.m_is_summoned = true;
    _DEBUG_MSG(1, "%s %s %s %u [%s]\n", status_description(src_status).c_str(), skill_names[skill_id].c_str(), cardtype_names[summoned->m_type].c_str(), card_status.m_index, card_description(fd->cards, summoned).c_str());
    prepend_skills(fd, &card_status);
    // Summon X (Genesis effect) does not activate Blitz for X
    if(std::get<1>(s) != 0 && card_status.m_card->m_blitz)
    {
        check_and_perform_blitz(fd, &card_status);
    }
    if(summoned->m_type == CardType::assault)
    {
        if(fd->effect == Effect::toxic)
        {
            card_status.m_poisoned = 1;
        }
        else if(fd->effect == Effect::decay)
        {
            card_status.m_poisoned = 1;
            card_status.m_diseased = true;
        }
    }
}

void perform_trigger_regen(Field* fd, CardStatus* src_status, const SkillSpec& s)
{
    check_regeneration(fd);
}

void perform_shock(Field* fd, CardStatus* src_status, const SkillSpec& s)
{
    check_and_perform_skill<shock>(fd, src_status, &fd->tip->commander, s, false, true);
}

// Special rules for mimic :
// cannot mimic mimic,
// structures cannot mimic supply,
// and is not affected by payback.
void perform_mimic(Field* fd, CardStatus* src_status, const SkillSpec& s)
{
    // mimic cannot be triggered by anything. So it should be the only skill in the unresolved skill table.
    // so we can probably clear it safely. This is necessary, because mimic calls resolve_skill as well (infinite loop).
    fd->skill_queue.clear();
    std::vector<CardStatus*>& cards(skill_targets<mimic>(fd, src_status));
    if(select_fast<mimic>(fd, src_status, cards, s, false) == 0)
    {
        return;
    }
    _DEBUG_SELECTION("%s", skill_names[mimic].c_str());
    CardStatus* c(select_interceptable(fd, src_status, fd->rand(0, fd->selection_array.size() - 1)));
    // evade check for mimic
    // individual skills are subject to evade checks too,
    // but resolve_skill will handle those.
    if((c->m_card->m_evade || (fd->effect == Effect::quicksilver && c->m_card->m_type == CardType::assault)) && fd->flip() && skill_check<evade>(fd, c, src_status))
    {
        count_achievement<evade>(fd, c);
        _DEBUG_MSG(1, "%s %s on %s but it evades\n", status_description(src_status).c_str(), skill_names[std::get<0>(s)].c_str(), status_description(c).c_str());
        return;
    }
    count_achievement<mimic>(fd, src_status);
    _DEBUG_MSG(1, "%s %s on %s\n", status_description(src_status).c_str(), skill_names[std::get<0>(s)].c_str(), status_description(c).c_str());
    auto mod = SkillMod::on_activate;
    bool need_add_skill = may_change_skill(fd, c, mod);
    for(auto& skill: c->m_card->m_skills)
    {
        if(src_status->m_card->m_type != CardType::action && src_status->m_hp == 0)
        { break; }
        if(std::get<0>(skill) == mimic || std::get<0>(skill) == split ||
                (std::get<0>(skill) == supply && src_status->m_card->m_type != CardType::assault))
        { continue; }
        auto& battleground_s = need_add_skill ? apply_battleground_effect(fd, c, skill, mod, need_add_skill) : skill;
        SkillSpec mimic_s(std::get<0>(battleground_s), std::get<1>(battleground_s), allfactions, std::get<3>(battleground_s), mod);
        _DEBUG_MSG(2, "Evaluating %s mimiced skill %s\n", status_description(c).c_str(), skill_description(fd->cards, mimic_s).c_str());
        fd->skill_queue.emplace_back(src_status, mimic_s);
        resolve_skill(fd);
        if(__builtin_expect(fd->end, false)) { break; }
        check_regeneration(fd);
    }
    if(need_add_skill)
    {
        auto battleground_s = apply_battleground_effect(fd, c, SkillSpec(new_skill, 0, allfactions, false, mod), mod, need_add_skill);
        assert(std::get<0>(battleground_s) != new_skill);
        SkillSpec mimic_s(std::get<0>(battleground_s), std::get<1>(battleground_s), allfactions, std::get<3>(battleground_s), mod);
        _DEBUG_MSG(2, "Evaluating %s mimiced skill %s\n", status_description(c).c_str(), skill_description(fd->cards, mimic_s).c_str());
        fd->skill_queue.emplace_back(src_status, mimic_s);
        resolve_skill(fd);
    }
}
//------------------------------------------------------------------------------
void fill_skill_table()
{
    skill_table[augment] = perform_targetted_allied_fast<augment>;
    skill_table[backfire] = perform_backfire;
    skill_table[chaos] = perform_targetted_hostile_fast<chaos>;
    skill_table[cleanse] = perform_targetted_allied_fast<cleanse>;
    skill_table[enfeeble] = perform_targetted_hostile_fast<enfeeble>;
    skill_table[freeze] = perform_targetted_hostile_fast<freeze>;
    skill_table[heal] = perform_targetted_allied_fast<heal>;
    skill_table[infuse] = perform_infuse;
    skill_table[jam] = perform_targetted_hostile_fast<jam>;
    skill_table[mimic] = perform_mimic;
    skill_table[protect] = perform_targetted_allied_fast<protect>;
    skill_table[rally] = perform_targetted_allied_fast<rally>;
    skill_table[recharge] = perform_recharge;
    skill_table[repair] = perform_targetted_allied_fast<repair>;
    skill_table[rush] = perform_targetted_allied_fast<rush>;
    skill_table[shock] = perform_shock;
    skill_table[siege] = perform_targetted_hostile_fast<siege>;
    skill_table[supply] = perform_targetted_allied_fast<supply>;
    skill_table[split] = perform_split;
    skill_table[strike] = perform_targetted_hostile_fast<strike>;
    skill_table[summon] = perform_summon<summon>;
    skill_table[trigger_regen] = perform_trigger_regen;
    skill_table[weaken] = perform_targetted_hostile_fast<weaken>;
}
