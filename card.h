#ifndef CARD_H_INCLUDED
#define CARD_H_INCLUDED

#include <cstring>
#include <string>
#include <vector>
#include <map>
#include "tyrant.h"

class Card
{
public:
    unsigned m_attack;
    unsigned m_base_id;  // The id of the original card if a card is unique and alt/upgraded. The own id of the card otherwise.
    unsigned m_delay;
    Faction m_faction;
    unsigned m_health;
    unsigned m_id;
    unsigned m_level;
    unsigned m_fusion_level;
    std::string m_name;
    unsigned m_rarity;
    unsigned m_set;
    std::vector<SkillSpec> m_skills;
    std::vector<SkillSpec> m_skills_on_play;
    //APN
    std::vector<SkillSpec> m_skills_on_attacked;
    std::vector<SkillSpec> m_skills_on_death;
    unsigned m_skill_value[Skill::num_skills];
    Skill::Trigger m_skill_trigger[Skill::num_skills];
    CardType::CardType m_type;
    CardCategory::CardCategory m_category;
    const Card* m_top_level_card; // [TU] corresponding full-level card
    unsigned m_recipe_cost;
    std::map<const Card*, unsigned> m_recipe_cards;
    std::map<const Card*, unsigned> m_used_for_cards;

public:
    Card() :
        m_attack(0),
        m_base_id(0),
        m_delay(0),
        m_faction(imperial),
        m_health(0),
        m_id(0),
        m_level(1),
        m_fusion_level(0),
        m_name(""),
        m_rarity(1),
        m_set(0),
        m_skills(),
        m_skills_on_play(),
	      //APN
	      m_skills_on_attacked(),
        m_skills_on_death(),
        m_type(CardType::assault),
        m_category(CardCategory::normal),
        m_top_level_card(this),
        m_recipe_cost(0),
        m_recipe_cards(),
        m_used_for_cards()
    {
        std::memset(m_skill_value, 0, sizeof m_skill_value);
        std::memset(m_skill_trigger, 0, sizeof m_skill_trigger);
    }

    void add_skill(Skill::Trigger trigger, Skill::Skill id, unsigned x, Faction y, unsigned n, unsigned c, Skill::Skill s, Skill::Skill s2 = Skill::no_skill, bool all = false, unsigned card_id = 0);
    bool is_top_level_card() const { return (this == m_top_level_card); }
    bool is_low_level_card() const { return (m_base_id == m_id); }
    const Card* upgraded() const { return is_top_level_card() ? this : m_used_for_cards.begin()->first; }
    const Card* downgraded() const { return is_low_level_card() ? this : m_recipe_cards.begin()->first; }
};

#endif
