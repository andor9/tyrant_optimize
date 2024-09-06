#include "xml.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include "rapidxml.hpp"
#include "card.h"
#include "cards.h"
#include "deck.h"
#include "tyrant.h"
#include "tyrant_optimize.h"
//---------------------- $20 cards.xml parsing ---------------------------------
// Sets: 1 enclave; 2 nexus; 3 blight; 4 purity; 5 homeworld;
// 6 phobos; 7 phobos aftermath; 8 awakening
// 1000 standard; 5000 rewards; 5001 promotional; 9000 exclusive
// mission only and test cards have no set
using namespace rapidxml;

Skill::Skill skill_name_to_id(const std::string & name)
{
    static std::map<std::string, int> skill_map;
    if (skill_map.empty())
    {
        for (unsigned i(0); i < Skill::num_skills; ++i)
        {
            std::string skill_id = boost::to_lower_copy(skill_names[i]);
            skill_map[skill_id] = i;
        }
        skill_map["armored"] = skill_map["armor"];  // Special case for Armor: id and name differ
        skill_map["besiege"] = skill_map["mortar"]; // Special case for Mortar: id and name differ
    }
    auto x = skill_map.find(boost::to_lower_copy(name));
    if (x == skill_map.end())
    {
        return Skill::no_skill;
    }
    else
    {
        return (Skill::Skill)x->second;
    }
}

Faction skill_faction(xml_node<>* skill)
{
    xml_attribute<>* y(skill->first_attribute("y"));
    if (y)
    {
        return static_cast<Faction>(atoi(y->value()));
    }
    return allfactions;
}

Skill::Trigger skill_trigger(xml_node<>* skill)
{
    xml_attribute<>* trigger(skill->first_attribute("trigger"));
    if (trigger)
    {
        for (unsigned t(Skill::Trigger::activate); t < Skill::num_triggers; ++t)
        {
            if (skill_trigger_names[t] == boost::to_lower_copy(std::string(trigger->value())))
            {
                return static_cast<Skill::Trigger>(t);
            }
        }
        std::cerr << "WARNING: unknown skill trigger: " << trigger->value() << std::endl;
    }
    return Skill::Trigger::activate;
}

unsigned node_value(xml_node<>* skill, const char* attribute, unsigned default_value = 0)
{
    xml_attribute<>* value_node(skill->first_attribute(attribute));
    return value_node ? atoi(value_node->value()) : default_value;
}

double node_value_float(xml_node<>* skill, const char* attribute, double default_value = 0)
{
    xml_attribute<>* value_node(skill->first_attribute(attribute));
    return value_node ? atof(value_node->value()) : default_value;
}

Skill::Skill skill_target_skill(xml_node<>* skill, const char* attribute)
{
    Skill::Skill s(Skill::no_skill);
    xml_attribute<>* x(skill->first_attribute(attribute));
    if (x)
    {
       s = skill_name_to_id(x->value());
    }
    return(s);
}

//------------------------------------------------------------------------------
void load_decks_xml(Decks& decks, const Cards& all_cards, const std::string & mission_filename, const std::string & raid_filename, bool do_warn_on_missing=true)
{
    try
    {
        read_missions(decks, all_cards, mission_filename, do_warn_on_missing);
    }
    catch (const rapidxml::parse_error& e)
    {
        std::cerr << "\nFailed to parse file [" << mission_filename << "]. Skip it.\n";
    }
    try
    {
        read_raids(decks, all_cards, raid_filename, do_warn_on_missing);
    }
    catch(const rapidxml::parse_error& e)
    {
        std::cerr << "\nFailed to parse file [" << raid_filename << "]. Skip it.\n";
    }
}

//------------------------------------------------------------------------------
bool parse_file(const std::string & filename, std::vector<char>& buffer, xml_document<>& doc, bool do_warn_on_missing=true)
{
    std::ifstream cards_stream(filename, std::ios::binary);
    if (!cards_stream.good())
    {
        if (do_warn_on_missing)
        {
            std::cerr << "WARNING: The file '" << filename << "' does not exist. Proceeding without reading from this file.\n";
        }
        buffer.resize(1);
        buffer[0] = 0;
        doc.parse<0>(&buffer[0]);
        return false;
    }
    // Get the size of the file
    cards_stream.seekg(0,std::ios::end);
    std::streampos length = cards_stream.tellg();
    cards_stream.seekg(0,std::ios::beg);
    buffer.resize(length + std::streampos(1));
    cards_stream.read(&buffer[0],length);
    // zero-terminate
    buffer[length] = '\0';
    try
    {
        doc.parse<0>(&buffer[0]);
        return true;
    }
    catch(rapidxml::parse_error& e)
    {
        std::cerr << "Parse error exception.\n";
        std::cout << e.what();
        throw(e);
    }
}
//------------------------------------------------------------------------------
void parse_card_node(Cards& all_cards, Card* card, xml_node<>* card_node)
{
    double eps = 1e-4;
    xml_node<>* id_node(card_node->first_node("id"));
    xml_node<>* card_id_node = card_node->first_node("card_id");
    assert(id_node || card_id_node);
    xml_node<>* name_node(card_node->first_node("name"));
    xml_node<>* attack_node(card_node->first_node("attack"));
    xml_node<>* health_node(card_node->first_node("health"));
    xml_node<>* cost_node(card_node->first_node("cost"));
    xml_node<>* rarity_node(card_node->first_node("rarity"));
    xml_node<>* type_node(card_node->first_node("type"));
    xml_node<>* fortress_type_node(card_node->first_node("fortress_type"));
    xml_node<>* set_node(card_node->first_node("set"));
    int set(set_node ? atoi(set_node->value()) : card->m_set);
    xml_node<>* level_node(card_node->first_node("level"));
    xml_node<>* fusion_level_node(card_node->first_node("fusion_level"));
    if (id_node) { card->m_base_id = card->m_id = atoi(id_node->value()); }
    else if (card_id_node) { card->m_id = atoi(card_id_node->value()); }
    if (name_node) { card->m_name = name_node->value(); }
    if (level_node) { card->m_level = atoi(level_node->value()); }
    if (fusion_level_node) { card->m_fusion_level = atoi(fusion_level_node->value()); }
    if (attack_node) { card->m_attack = atoi(attack_node->value()); 
	    if(abs(1-atk_scale)>eps)
		card->m_attack = ceil(card->m_attack/(atk_scale));
}
    if (health_node) { card->m_health = atoi(health_node->value()); 
	    if(abs(1-hp_scale)>eps)
		card->m_health = ceil(card->m_health/(hp_scale));

    }
    if (cost_node) { card->m_delay = atoi(cost_node->value()); }
    if (id_node)
    {
        // [0 .. 999]
        if (card->m_id < 1000)
        {
            card->m_type = CardType::assault;
        }

        // [1000 .. 1999]
        else if (card->m_id < 2000)
        {
            card->m_type = CardType::commander;
        }

        // [2000 .. 2999]
        else if (card->m_id < 3000)
        {
            card->m_type = CardType::structure;

            // fortress
            if ((card->m_id >= 2700) && (card->m_id < 2997))
            {
                if (fortress_type_node) {
                    unsigned fort_type_value = atoi(fortress_type_node->value());
                    switch (fort_type_value) {
                    case 1:
                        card->m_category = CardCategory::fortress_defense;
                        break;
                    case 2:
                        card->m_category = CardCategory::fortress_siege;
                        break;
                    default:
                        std::cerr << "WARNING: parsing card [" << card->m_id << "]: unsupported fortress_type=" << fort_type_value << std::endl;
                    }
                }
                else if ((card->m_id < 2748) || (card->m_id >= 2754)) // except Sky Fortress
                {
                    std::cerr << "WARNING: parsing card [" << card->m_id << "]: expected fortress_type node" << std::endl;
                }
            }
        }

        // [3000 ... 7999]
        else if (card->m_id < 8000)
        {
            card->m_type = CardType::assault;
        }

        // [8000 .. 9999]
        else if (card->m_id < 10000)
        {
            card->m_type = CardType::structure;
        }

        // [10000 .. 16999]
        else if (card->m_id < 17000)
        {
            card->m_type = CardType::assault;
        }

        // [17000 .. 24999]
        else if (card->m_id < 25000)
        {
            card->m_type = CardType::structure;
        }

        // [25000 .. 29999]
        else if (card->m_id < 30000)
        {
            card->m_type = CardType::commander;
        }

        // [30000 .. 50000]
        else if (card->m_id < 50001)
        {
            card->m_type = CardType::assault;
            if ((card->m_id == 43451) || (card->m_id == 43452))
            {
                card->m_category = CardCategory::dominion_material;
            }
        }

        // [50001 .. 55000]
        else if (card->m_id < 55001)
        {
            card->m_type = CardType::structure;
            card->m_category = CardCategory::dominion_alpha;
        }

        // [55001+]
        else
        {
            card->m_type = CardType::assault;
        }
    }
    if (rarity_node) { card->m_rarity = atoi(rarity_node->value()); }
    if (type_node) { card->m_faction = static_cast<Faction>(atoi(type_node->value())); }
    card->m_set = set;

    // fortresses
    if (set == 8000)
    {
        if (card->m_type != CardType::structure)
        {
            std::cerr << "WARNING: parsing card [" << card->m_id << "]: set 8000 supposes fortresses card that implies type Structure"
                << ", but card has type " << cardtype_names[card->m_type] << std::endl;
        }

        // assume all other fortresses as conquest towers
        if (card->m_category == CardCategory::normal)
        {
            card->m_category = CardCategory::fortress_conquest;
        }
    }

    if (card_node->first_node("skill"))
    { // inherit no skill if there is skill node
        card->m_skills.clear();
        card->m_skills_on_play.clear();
	//APN
	card->m_skills_on_attacked.clear();
        card->m_skills_on_death.clear();
        memset(card->m_skill_value, 0, sizeof card->m_skill_value);
        memset(card->m_skill_trigger, 0, sizeof card->m_skill_trigger);
    }
    for (xml_node<>* skill_node = card_node->first_node("skill");
            skill_node;
            skill_node = skill_node->next_sibling("skill"))
    {
        Skill::Skill skill_id = skill_name_to_id(skill_node->first_attribute("id")->value());
        if (skill_id == Skill::no_skill) { continue; }
        auto trig = skill_trigger(skill_node);
        auto x = node_value(skill_node, "x", 0);
        auto y = skill_faction(skill_node);
        auto n = node_value(skill_node, "n", 0);
        auto c = node_value(skill_node, "c", skill_id==Skill::flurry?1:0);
        auto s = skill_target_skill(skill_node, "s");
        auto s2 = skill_target_skill(skill_node, "s2");
        bool all = skill_node->first_attribute("all");
        auto card_id = node_value(skill_node, "card_id", 0);
	//scaling
	if(abs(1-x_skill_scale[Skill::no_skill]*x_skill_scale[skill_id==Skill::enhance?s:skill_id])>eps)
		x = ceil(x/(x_skill_scale[Skill::no_skill]*x_skill_scale[skill_id==Skill::enhance?s:skill_id]));
	if(abs(1-n_skill_scale[Skill::no_skill]*n_skill_scale[skill_id])>eps)
		n = ceil(n/(n_skill_scale[Skill::no_skill]*n_skill_scale[skill_id]));
	if(abs(1-c_skill_scale[Skill::no_skill]*c_skill_scale[skill_id])>eps)
		c = ceil(c/(c_skill_scale[Skill::no_skill]*c_skill_scale[skill_id]));



        card->add_skill(trig, skill_id, x, y, n, c, s, s2, all, card_id);
    }
    all_cards.all_cards.push_back(card);
    Card* top_card = card;
    for (xml_node<>* upgrade_node = card_node->first_node("upgrade");
            upgrade_node;
            upgrade_node = upgrade_node->next_sibling("upgrade"))
    {
        Card* pre_upgraded_card = top_card;
        top_card = new Card(*top_card);
        parse_card_node(all_cards, top_card, upgrade_node);
        if (top_card->m_type == CardType::commander)
        {
            // Commanders cost twice
            top_card->m_recipe_cost = 2 * upgrade_cost[pre_upgraded_card->m_level];
        }
        else
        {
            top_card->m_recipe_cost = upgrade_cost[pre_upgraded_card->m_level];
        }
        top_card->m_recipe_cards.clear();
        top_card->m_recipe_cards[pre_upgraded_card] = 1;
        pre_upgraded_card->m_used_for_cards[top_card] = 1;
    }
    card->m_top_level_card = top_card;
}

bool load_cards_xml(Cards & all_cards, const std::string & filename, bool do_warn_on_missing)
{
    std::vector<char> buffer;
    xml_document<> doc;
    if (!parse_file(filename, buffer, doc, do_warn_on_missing)) { return false; }
    xml_node<>* root = doc.first_node();
    if (!root) { return false; }

    for (xml_node<>* card_node = root->first_node("unit");
        card_node;
        card_node = card_node->next_sibling("unit"))
    {
        auto card = new Card();
        parse_card_node(all_cards, card, card_node);
    }

    return true;
}

void load_skills_set_xml(Cards & all_cards, const std::string & filename, bool do_warn_on_missing)
{
    std::vector<char> buffer;
    xml_document<> doc;
    parse_file(filename, buffer, doc, do_warn_on_missing);
    xml_node<>* root = doc.first_node();
    if (!root) { return; }

    for (xml_node<>* set_node = root->first_node("cardSet");
            set_node;
            set_node = set_node->next_sibling("cardSet"))
    {
        xml_node<>* id_node(set_node->first_node("id"));
        xml_node<>* visible_node = set_node->first_node("visible");
        if (id_node && visible_node && atoi(visible_node->value()))
        {
            all_cards.visible_cardset.insert(atoi(id_node->value()));
        }
    }
}

void load_levels_xml(Cards& all_cards, const std::string& filename, bool do_warn_on_missing)
{
    std::vector<char> buffer;
    xml_document<> doc;
    parse_file(filename, buffer, doc, do_warn_on_missing);
    xml_node<>* root = doc.first_node();
    if (!root) { return; }

    for (xml_node<>* dfl_node = root->first_node("dominion_fusion_level");
            dfl_node;
            dfl_node = dfl_node->next_sibling("dominion_fusion_level"))
    {
        xml_node<>* fl_node = dfl_node->first_node("fusion_level");
        if (!fl_node)
        {
            std::cerr << "WARNING: levels.xml/dominion_fusion_level: no fusion_level" << std::endl;
            continue;
        }
        unsigned fusion_lvl = atoi(fl_node->value());
        if (fusion_lvl > 2)
        {
            std::cerr << "WARNING: levels.xml/dominion_fusion_level: unsupported fusion level: " << fusion_lvl << std::endl;
            continue;
        }
        for (xml_node<>* lvl_node = dfl_node->first_node("level");
                lvl_node;
                lvl_node = lvl_node->next_sibling("level"))
        {
            xml_node<>* id_node(lvl_node->first_node("id"));
            unsigned card_lvl = id_node ? atoi(id_node->value()) : 0;
            if (!(card_lvl >= 1 && card_lvl <= 6))
            {
                std::cerr << "WARNING: levels.xml/dominion_fusion_level: unsupported card level: " << card_lvl << std::endl;
                continue;
            }

            // dominion cost in shards (dominion material)
            // each level have cost for next level (using lvl+1), first level cost can be found in fusion recipes
            for (xml_node<>* cost_node = lvl_node->first_node("card_cost");
                    cost_node && (card_lvl < 6);
                    cost_node = cost_node->next_sibling("card_cost"))
            {
                const Card* cost_card = all_cards.by_id(atoi(cost_node->first_attribute("card_id")->value()));
                if (cost_card->m_category != CardCategory::dominion_material)
                {
                    std::cerr << "WARNING: levels.xml/dominion_fusion_level: card_id=" << cost_card->m_id
                        << " (" << cost_card->m_name << ") is not a dominion material card (new shard type?)" << std::endl;
                }
                else
                {
                    dominion_cost[fusion_lvl][card_lvl+1][cost_card] += atoi(cost_node->first_attribute("number")->value());
                }
            }

            // dominion refund in shards (dominion material)
            for (xml_node<>* ref_node = lvl_node->first_node("card_refund");
                    ref_node;
                    ref_node = ref_node->next_sibling("card_refund"))
            {
                const Card* refund_card = all_cards.by_id(atoi(ref_node->first_attribute("card_id")->value()));
                if (refund_card->m_category == CardCategory::dominion_material)
                {
                    dominion_refund[fusion_lvl][card_lvl][refund_card] += atoi(ref_node->first_attribute("number")->value());
                }
            }
        }
    }
}
//------------------------------------------------------------------------------
Deck* read_deck(Decks& decks, const Cards& all_cards, xml_node<>* node, DeckType::DeckType decktype, unsigned id, std::string base_deck_name)
{
    xml_node<>* commander_node(node->first_node("commander"));
    const Card* card = all_cards.by_id(atoi(commander_node->value()));
    const Card* commander_card{card};
    xml_node<>* commander_max_level_node(node->first_node("commander_max_level"));
    unsigned commander_max_level = commander_max_level_node ? atoi(commander_max_level_node->value()) : commander_card->m_top_level_card->m_level;
    unsigned upgrade_opportunities = commander_max_level - card->m_level;
    std::vector<const Card*> always_cards;
    std::vector<std::tuple<unsigned, unsigned, std::vector<const Card*>>> some_forts;
    std::vector<std::tuple<unsigned, unsigned, std::vector<const Card*>>> some_cards;
    xml_node<>* levels_node(node->first_node("levels"));
    xml_node<>* effects_node(node->first_node("effects"));
    xml_node<>* deck_node(node->first_node("deck"));
    unsigned max_level = levels_node ? atoi(levels_node->value()) : 10;

    // Effectes (skill based BGEs; assuming that X is a floating point number (multiplier))
    std::vector<SkillSpecXMult> effects;
    if (effects_node)
    {
        for (xml_node<>* skill_node = effects_node->first_node("skill");
                skill_node;
                skill_node = skill_node->next_sibling("skill"))
        {
            auto skill_name = skill_node->first_attribute("id")->value();
            Skill::Skill skill_id = skill_name_to_id(skill_name);
            if (skill_id == Skill::no_skill) { throw std::runtime_error("unknown skill id:" + tuo::to_string(skill_name)); }
            auto x = node_value_float(skill_node, "x", 0.0);
            auto y = skill_faction(skill_node);
            auto n = node_value(skill_node, "n", 0);
            auto c = node_value(skill_node, "c", 0);
            auto s = skill_target_skill(skill_node, "s");
            auto s2 = skill_target_skill(skill_node, "s2");
            bool all(skill_node->first_attribute("all"));
            effects.push_back({skill_id, x, y, n, c, s, s2, all});
        }
    }

    // Fixed fortresses (<fortress_card id="xxx"/>)
    std::vector<const Card*> fortress_cards;
    for (xml_node<>* fortress_card_node = node->first_node("fortress_card");
            fortress_card_node;
            fortress_card_node = fortress_card_node->next_sibling("fortress_card"))
    {
        const Card* card = all_cards.by_id(atoi(fortress_card_node->first_attribute("id")->value()));
        fortress_cards.push_back(card);
        upgrade_opportunities += card->m_top_level_card->m_level - card->m_level;
    }

    // Variable fortresses (<fortress_pool amount="x" replicates="y"> ... </fortress_pool>)
    for (xml_node<>* fortress_pool_node = node->first_node("fortress_pool");
            fortress_pool_node;
            fortress_pool_node = fortress_pool_node->next_sibling("fortress_pool"))
    {
        unsigned num_cards_from_pool(atoi(fortress_pool_node->first_attribute("amount")->value()));
        unsigned pool_replicates(fortress_pool_node->first_attribute("replicates")
            ? atoi(fortress_pool_node->first_attribute("replicates")->value())
            : 1);
        std::vector<const Card*> cards_from_pool;
        unsigned upgrade_points = 0;
        for (xml_node<>* card_node = fortress_pool_node->first_node("card");
                card_node;
                card_node = card_node->next_sibling("card"))
        {
            card = all_cards.by_id(atoi(card_node->value()));
            unsigned card_replicates(card_node->first_attribute("replicates") ? atoi(card_node->first_attribute("replicates")->value()) : 1);
            while (card_replicates --)
            {
                cards_from_pool.push_back(card);
                upgrade_points += card->m_top_level_card->m_level - card->m_level;
            }
        }
        some_forts.push_back(std::make_tuple(num_cards_from_pool, pool_replicates, cards_from_pool));
        upgrade_opportunities += upgrade_points * num_cards_from_pool * pool_replicates / cards_from_pool.size();
    }

    // Fixed cards (<always_include> ... </always_include>)
    xml_node<>* always_node{deck_node->first_node("always_include")};
    for (xml_node<>* card_node = (always_node ? always_node : deck_node)->first_node("card");
            card_node;
            card_node = card_node->next_sibling("card"))
    {
        card = all_cards.by_id(atoi(card_node->value()));
        unsigned replicates(card_node->first_attribute("replicates") ? atoi(card_node->first_attribute("replicates")->value()) : 1);
        while (replicates --)
        {
            always_cards.push_back(card);
            upgrade_opportunities += card->m_top_level_card->m_level - card->m_level;
        }
    }

    // Variable cards (<card_pool amount="x" replicates="y"> ... </card_pool>)
    for (xml_node<>* pool_node = deck_node->first_node("card_pool");
            pool_node;
            pool_node = pool_node->next_sibling("card_pool"))
    {
        unsigned num_cards_from_pool(atoi(pool_node->first_attribute("amount")->value()));
        unsigned pool_replicates(pool_node->first_attribute("replicates") ? atoi(pool_node->first_attribute("replicates")->value()) : 1);
        std::vector<const Card*> cards_from_pool;
        unsigned upgrade_points = 0;
        for (xml_node<>* card_node = pool_node->first_node("card");
                card_node;
                card_node = card_node->next_sibling("card"))
        {
            card = all_cards.by_id(atoi(card_node->value()));
            unsigned card_replicates(card_node->first_attribute("replicates") ? atoi(card_node->first_attribute("replicates")->value()) : 1);
            while (card_replicates --)
            {
                cards_from_pool.push_back(card);
                upgrade_points += card->m_top_level_card->m_level - card->m_level;
            }
        }
        some_cards.push_back(std::make_tuple(num_cards_from_pool, pool_replicates, cards_from_pool));
        upgrade_opportunities += upgrade_points * num_cards_from_pool * pool_replicates / cards_from_pool.size();
    }

    // Mission requirement
    xml_node<>* mission_req_node(node->first_node(decktype == DeckType::mission ? "req" : "mission_req"));
    unsigned mission_req(mission_req_node ? atoi(mission_req_node->value()) : 0);

    for (unsigned level = 1; level < max_level; ++ level)
    {
        std::string deck_name = base_deck_name + "-" + tuo::to_string(level);
        unsigned upgrade_points = ceil(upgrade_opportunities * (level - 1) / (double)(max_level - 1));
        decks.decks.push_back(Deck{all_cards, decktype, id, deck_name, upgrade_points, upgrade_opportunities});
        Deck* deck = &decks.decks.back();
        deck->set(commander_card, commander_max_level, always_cards, some_forts, some_cards, mission_req);
        deck->fortress_cards = fortress_cards;
        deck->effects = effects;
        deck->level = level;
        decks.add_deck(deck, deck_name);
        decks.add_deck(deck, decktype_names[decktype] + " #" + tuo::to_string(id) + "-" + tuo::to_string(level));
    }

    decks.decks.push_back(Deck{all_cards, decktype, id, base_deck_name});
    Deck* deck = &decks.decks.back();
    deck->set(commander_card, commander_max_level, always_cards, some_forts, some_cards, mission_req);
    deck->fortress_cards = fortress_cards;
    deck->effects = effects;
    deck->level = max_level;

    // upgrade cards for full-level missions/raids
    if (max_level > 1)
    {
        while (deck->commander->m_level < commander_max_level)
        { deck->commander = deck->commander->upgraded(); }
        for (auto && card: deck->fortress_cards)
        { card = card->m_top_level_card; }
        for (auto && card: deck->cards)
        { card = card->m_top_level_card; }
        for (auto && pool: deck->variable_forts)
        {
            for (auto && card: std::get<2>(pool))
            { card = card->m_top_level_card; }
        }
        for (auto && pool: deck->variable_cards)
        {
            for (auto && card: std::get<2>(pool))
            { card = card->m_top_level_card; }
        }
    }

    decks.add_deck(deck, base_deck_name);
    decks.add_deck(deck, base_deck_name + "-" + tuo::to_string(max_level));
    decks.add_deck(deck, decktype_names[decktype] + " #" + tuo::to_string(id));
    decks.add_deck(deck, decktype_names[decktype] + " #" + tuo::to_string(id) + "-" + tuo::to_string(max_level));
    decks.by_type_id[{decktype, id}] = deck;
    return deck;
}
//------------------------------------------------------------------------------
void read_missions(Decks& decks, const Cards& all_cards, const std::string & filename, bool do_warn_on_missing=true)
{
    std::vector<char> buffer;
    xml_document<> doc;
    parse_file(filename.c_str(), buffer, doc, do_warn_on_missing);
    xml_node<>* root = doc.first_node();

    if (!root)
    {
        return;
    }

    for (xml_node<>* mission_node = root->first_node("mission");
        mission_node;
        mission_node = mission_node->next_sibling("mission"))
    {
        std::vector<unsigned> card_ids;
        xml_node<>* id_node(mission_node->first_node("id"));
        assert(id_node);
        unsigned id(id_node ? atoi(id_node->value()) : 0);
        xml_node<>* name_node(mission_node->first_node("name"));
        std::string deck_name{name_node->value()};
        try
        {
            read_deck(decks, all_cards, mission_node, DeckType::mission, id, deck_name);
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << "WARNING: Failed to parse mission [" << deck_name << "] in file " << filename << ": [" << e.what() << "]. Skip the mission.\n";
            continue;
        }
    }
}
//------------------------------------------------------------------------------
void read_raids(Decks& decks, const Cards& all_cards, const std::string & filename, bool do_warn_on_missing=true)
{
    std::vector<char> buffer;
    xml_document<> doc;
    parse_file(filename.c_str(), buffer, doc, do_warn_on_missing);
    xml_node<>* root = doc.first_node();

    if (!root)
    {
        return;
    }

    for (xml_node<>* raid_node = root->first_node("raid");
        raid_node;
        raid_node = raid_node->next_sibling("raid"))
    {
        xml_node<>* id_node(raid_node->first_node("id"));
        assert(id_node);
        unsigned id(id_node ? atoi(id_node->value()) : 0);
        xml_node<>* name_node(raid_node->first_node("name"));
        std::string deck_name{name_node->value()};
        try
        {
            read_deck(decks, all_cards, raid_node, DeckType::raid, id, deck_name);
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << "WARNING: Failed to parse raid [" << deck_name << "] in file " << filename << ": [" << e.what() << "]. Skip the raid.\n";
            continue;
        }
    }

    for (xml_node<>* campaign_node = root->first_node("campaign");
        campaign_node;
        campaign_node = campaign_node->next_sibling("campaign"))
    {
        xml_node<>* id_node(campaign_node->first_node("id"));
        unsigned id(id_node ? atoi(id_node->value()) : 0);
        for (auto && name_node = campaign_node->first_node("name");
            name_node;
            name_node = name_node->next_sibling("name"))
        {
            try
            {
                read_deck(decks, all_cards, campaign_node, DeckType::campaign, id, name_node->value());
            }
            catch (const std::runtime_error& e)
            {
                std::cerr << "WARNING: Failed to parse campaign [" << name_node->value() << "] in file " << filename << ": [" << e.what() << "]. Skip the campaign.\n";
                continue;
            }
        }
    }
}

//------------------------------------------------------------------------------
void load_recipes_xml(Cards& all_cards, const std::string & filename, bool do_warn_on_missing=true)
{
    std::vector<char> buffer;
    xml_document<> doc;
    parse_file(filename, buffer, doc, do_warn_on_missing);
    xml_node<>* root = doc.first_node();

    if (!root)
    {
        return;
    }

    for (xml_node<>* recipe_node = root->first_node("fusion_recipe");
        recipe_node;
        recipe_node = recipe_node->next_sibling("fusion_recipe"))
    {
        xml_node<>* card_id_node(recipe_node->first_node("card_id"));
        if (!card_id_node) { continue; }
        unsigned card_id(atoi(card_id_node->value()));
        Card* card = all_cards.cards_by_id[card_id];
        if (!card) {
            std::cerr << "Could not find card by id " << card_id << std::endl;
            continue;
        }

        for (xml_node<>* resource_node = recipe_node->first_node("resource");
                resource_node;
                resource_node = resource_node->next_sibling("resource"))
        {
            unsigned card_id(node_value(resource_node, "card_id"));
            unsigned number(node_value(resource_node, "number"));
            if (card_id == 0 || number == 0) { continue; }
            if (!card->is_low_level_card())
            {
                card->m_base_id = card->m_id;
                card->m_recipe_cards.clear();
            }
            Card* material_card = all_cards.cards_by_id[card_id];
            card->m_recipe_cards[material_card] += number;
            material_card->m_used_for_cards[card] += number;
        }
    }
}

