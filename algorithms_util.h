#include "deck.h"
#include "tyrant_optimize.h"

void copy_deck(Deck* src,Deck* dst);
std::string card_id_name(const Card* card);
std::string card_slot_id_names(const std::vector<std::pair<signed, const Card *>> card_list);

bool adjust_deck(Deck * deck, const signed from_slot, const signed to_slot, const Card * card, unsigned fund, std::mt19937 & re, unsigned & deck_cost,
		std::vector<std::pair<signed, const Card *>> & cards_out, std::vector<std::pair<signed, const Card *>> & cards_in);


unsigned check_requirement(const Deck* deck, const Requirement & requirement
#ifndef NQUEST
		, const Quest & quest
#endif
);
