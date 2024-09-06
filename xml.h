#ifndef XML_H_INCLUDED
#define XML_H_INCLUDED

#include <string>
#include "tyrant.h"

class Cards;
class Decks;
class Achievement;

Skill::Skill skill_name_to_id(const std::string & name);
bool load_cards_xml(Cards & all_cards, const std::string & filename, bool do_warn_on_missing);
void load_skills_set_xml(Cards & all_cards, const std::string & filename, bool do_warn_on_missing);
void load_levels_xml(Cards& all_cards, const std::string& filename, bool do_warn_on_missing);
void load_decks_xml(Decks& decks, const Cards& all_cards, const std::string & mission_filename, const std::string & raid_filename, bool do_warn_on_missing);
void load_recipes_xml(Cards& all_cards, const std::string & filename, bool do_warn_on_missing);
void read_missions(Decks& decks, const Cards& all_cards, const std::string & filename, bool do_warn_on_missing);
void read_raids(Decks& decks, const Cards& all_cards, const std::string & filename, bool do_warn_on_missing);

#endif
