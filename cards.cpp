#include "cards.h"

#include <boost/tokenizer.hpp>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <list>
#include <iostream>

#include "tyrant.h"
#include "card.h"

std::string simplify_name(const std::string& card_name)
{
    std::string simple_name;
    for(auto c : card_name)
    {
        if(!strchr(";:,\"'! ", c))
        {
            simple_name += ::tolower(c);
        }
    }
    return(simple_name);
}

std::list<std::string> get_abbreviations(const std::string& name)
{
    std::list<std::string> abbr_list;
    boost::tokenizer<boost::char_delimiters_separator<char>> word_token{name, boost::char_delimiters_separator<char>{false, " -", ""}};
    std::string initial;
    auto token_iter = word_token.begin();
    for(; token_iter != word_token.end(); ++token_iter)
    {
        abbr_list.push_back(simplify_name(std::string{token_iter->begin(), token_iter->end()}));
        initial += *token_iter->begin();
    }
    abbr_list.push_back(simplify_name(initial));
    return(abbr_list);
}

//------------------------------------------------------------------------------
Cards::~Cards()
{
    for (Card* c: all_cards) { delete(c); }
}

const Card* Cards::by_id(unsigned id) const
{
    const auto cardIter = cards_by_id.find(id);
    if (cardIter == cards_by_id.end())
    {
        throw std::runtime_error("No card with id " + tuo::to_string(id));
    }
    else
    {
        return(cardIter->second);
    }
}
//------------------------------------------------------------------------------
void Cards::organize()
{
    cards_by_id.clear();
    player_cards.clear();
    cards_by_name.clear();
    player_commanders.clear();
    player_assaults.clear();
    player_structures.clear();
    // Round 1: set cards_by_id
    for (Card* card: all_cards)
    {
        cards_by_id[card->m_id] = card;
    }

    // Round 2: depend on cards_by_id / by_id(); update m_name, [TU] m_top_level_card etc.; set cards_by_name; 
    for (Card* card: all_cards)
    {
        // Remove delimiters from card names
        size_t pos;
        while((pos = card->m_name.find_first_of(";:,")) != std::string::npos)
        {
            card->m_name.erase(pos, 1);
        }
        // set m_top_level_card for non base cards
        card->m_top_level_card = by_id(card->m_base_id)->m_top_level_card;
        // Cards available ("visible") to players have priority
        std::string base_name = card->m_name;
        if (card == card->m_top_level_card)
        {
            add_card(card, card->m_name + "-" + tuo::to_string(card->m_level));
        }
        else
        {
            card->m_name += "-" + tuo::to_string(card->m_level);
        }
        add_card(card, card->m_name);
    }

    // Round 3: depend on summon skill card_id check that card_id
    for (Card* card: all_cards)
    {
        unsigned summon_card_id(card->m_skill_value[Skill::summon]);
        if (!summon_card_id) { continue; }
        if (!cards_by_id[summon_card_id])
        {
            std::cerr << "WARNING: Card [" << card->m_id << "] (" << card->m_name
                << ") summons an unknown card [" << summon_card_id << "] (removing invalid skill Summon)" << std::endl;
            std::remove_if(card->m_skills.begin(), card->m_skills.end(), [](const SkillSpec& ss) {return ss.id == Skill::summon;});
            card->m_skill_value[Skill::summon] = 0;
        }
    }
    //Test
    //Round 4: sort cards by id
    struct { bool operator()(Card* a, Card* b) const {return a->m_id < b->m_id;}} idsort;
    std::sort(all_cards.begin(),all_cards.end(),idsort);


}
//------------------------------------------------------------------------------
void Cards::fix_dominion_recipes()
{
    for (Card* card: all_cards)
    {
        if (card->m_category != CardCategory::dominion_alpha)
        { continue; }
        std::map<const Card*, unsigned> dom_cost = dominion_cost[card->m_fusion_level][card->m_level];
        for (auto recipe_it : dom_cost)
        {
            // except basic Alpha Dominion (id 50001 & 50002 for lvl 1 & 2 respectively)
            if ((card->m_id != 50001) && (card->m_id != 50002))
            {
                card->m_recipe_cards[recipe_it.first] += recipe_it.second;
            }
        }
        card->m_recipe_cost = 0; // no SP required
    }
}
//------------------------------------------------------------------------------
void Cards::add_card(Card * card, const std::string & name)
{
    std::string simple_name{simplify_name(name)};
    auto card_itr = cards_by_name.find(simple_name);
    signed old_visible = card_itr == cards_by_name.end() ? -1 : visible_cardset.count(card_itr->second->m_set);
    signed new_visible = visible_cardset.count(card->m_set);
    if (card_itr != cards_by_name.end())
    {
        if (old_visible == new_visible)
        {
            ambiguous_names.insert(simple_name);
        }
        _DEBUG_MSG(2, "Duplicated card name \"%s\" [%u] set=%u (visible=%u) : [%u] set=%u (visible=%u)\n", name.c_str(), card_itr->second->m_id, card_itr->second->m_set, old_visible, card->m_id, card->m_set, new_visible);
    }
    else if (old_visible < new_visible)
    {
        ambiguous_names.erase(simple_name);
        cards_by_name[simple_name] = card;
        if (new_visible && !player_cards.count(card))
        {
            player_cards.emplace(card);
            switch(card->m_type)
            {
                case CardType::commander: {
                    player_commanders.push_back(card);
                    break;
                }
                case CardType::assault: {
                    player_assaults.push_back(card);
                    break;
                }
                case CardType::structure: {
                    player_structures.push_back(card);
                    break;
                }
                case CardType::num_cardtypes: {
                    throw card->m_type;
                    break;
                }
            }
        }
    }
}

void Cards::erase_fusion_recipe(unsigned id)
{
    cards_by_id[by_id(id)->m_base_id]->m_recipe_cards.clear();
}

// class Card
void Card::add_skill(Skill::Trigger trigger, Skill::Skill id, unsigned x, Faction y, unsigned n, unsigned c, Skill::Skill s, Skill::Skill s2, bool all, unsigned card_id)
{
    std::vector<SkillSpec>* storage(nullptr);
    switch (trigger)
    {
    case Skill::Trigger::activate:
        storage = &m_skills;
        break;
    case Skill::Trigger::play:
        storage = &m_skills_on_play;
        break;
    //APN
    case Skill::Trigger::attacked:
        storage = &m_skills_on_attacked;
        break;
    case Skill::Trigger::death:
        storage = &m_skills_on_death;
        break;
    default:
        _DEBUG_ASSERT(false);
        __builtin_unreachable();
    }
    assert(storage);
    // remove previous copy of such skill.id
    {
        auto pred = [id](const SkillSpec& ss) { return ss.id == id; };
        m_skills.erase(std::remove_if(m_skills.begin(), m_skills.end(), pred), m_skills.end());
        m_skills_on_play.erase(std::remove_if(m_skills_on_play.begin(), m_skills_on_play.end(), pred), m_skills_on_play.end());
        m_skills_on_death.erase(std::remove_if(m_skills_on_death.begin(), m_skills_on_death.end(), pred), m_skills_on_death.end());
        //APN
        m_skills_on_attacked.erase(std::remove_if(m_skills_on_attacked.begin(), m_skills_on_attacked.end(), pred), m_skills_on_attacked.end());
    }
    // add a new one
    storage->push_back({id, x, y, n, c, s, s2, all, card_id});
    // setup value
    m_skill_value[id] = x ? x : n ? n : card_id ? card_id : 1;
    m_skill_trigger[id] = trigger;
}
