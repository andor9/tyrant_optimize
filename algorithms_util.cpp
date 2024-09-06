#include "algorithms_util.h"
void copy_deck(Deck* src,Deck* dst)
{
	dst->commander = src->commander;
	dst->alpha_dominion = src->alpha_dominion;
	dst->cards = src->cards;
}
std::string card_id_name(const Card* card)
{
	std::stringstream ios;
	if (card)
	{
		ios << "[" << card->m_id << "] " << card->m_name;
	}
	else
	{
		ios << "-void-";
	}
	return ios.str();
}
std::string card_slot_id_names(const std::vector<std::pair<signed, const Card *>> card_list)
{
	if (card_list.empty())
	{
		return "-void-";
	}
	std::stringstream ios;
	std::string separator = "";
	for (const auto & card_it : card_list)
	{
		ios << separator;
		separator = ", ";
		if (card_it.first >= 0)
		{ ios << card_it.first << " "; }
		ios << "[" << card_it.second->m_id << "] " << card_it.second->m_name;
	}
	return ios.str();
}
// remove val from oppo if found, otherwise append val to self
	template <typename C>
void append_unless_remove(C & self, C & oppo, typename C::const_reference val)
{
	for (auto it = oppo.begin(); it != oppo.end(); ++ it)
	{
		if (*it == val)
		{
			oppo.erase(it);
			return;
		}
	}
	self.push_back(val);
}

// insert card at to_slot into deck limited by fund; store deck_cost
// return true if affordable
bool adjust_deck(Deck * deck, const signed from_slot, const signed to_slot, const Card * card, unsigned fund, std::mt19937 & re, unsigned & deck_cost,
		std::vector<std::pair<signed, const Card *>> & cards_out, std::vector<std::pair<signed, const Card *>> & cards_in)
{
	bool is_random = (deck->strategy == DeckStrategy::random) || (deck->strategy == DeckStrategy::flexible);
	cards_out.clear();
	cards_in.clear();
	if (from_slot < 0)
	{
		if (card->m_category == CardCategory::dominion_alpha)
		{ // change alpha dominion
			cards_out.emplace_back(-1, deck->alpha_dominion);
			deck->alpha_dominion = card;
			cards_in.emplace_back(-1, deck->alpha_dominion);
			deck_cost = get_deck_cost(deck);
			return true;
		}

		// change commander
		cards_out.emplace_back(-1, deck->commander);
		deck->commander = card;
		cards_in.emplace_back(-1, deck->commander);
		deck_cost = get_deck_cost(deck);
		return (deck_cost <= fund);
	}
	if (from_slot < (signed)deck->cards.size())
	{ // remove card from the deck
		cards_out.emplace_back(is_random ? -1 : from_slot, deck->cards[from_slot]);
		deck->cards.erase(deck->cards.begin() + from_slot);
	}
	if (card == nullptr)
	{ // remove card (no new replacement for removed card)
		deck_cost = get_deck_cost(deck);
		return (deck_cost <= fund);
	}

	// backup deck cards
	const Card * old_commander = deck->commander;
	std::vector<const Card *> cards = deck->cards;

	// try to add new card into the deck, downgrade it if necessary
	{
		const Card * candidate_card = card;
		deck->commander = nullptr;
		deck->cards.clear();
		deck->cards.emplace_back(card);
		deck_cost = get_deck_cost(deck);
		if (!use_top_level_card && (deck_cost > fund))
		{
			while ((deck_cost > fund) && !candidate_card->is_low_level_card())
			{
				candidate_card = candidate_card->downgraded();
				deck->cards[0] = candidate_card;
				deck_cost = get_deck_cost(deck);
			}
		}
		if (deck_cost > fund)
		{ return false; }
		cards_in.emplace_back(is_random ? -1 : to_slot, deck->cards[0]);
	}

	// try to add commander into the deck, downgrade it if necessary
	{
		const Card * candidate_card = old_commander;
		deck->commander = candidate_card;
		deck_cost = get_deck_cost(deck);
		if (!use_top_level_commander && (deck_cost > fund))
		{
			while ((deck_cost > fund) && !candidate_card->is_low_level_card())
			{
				candidate_card = candidate_card->downgraded();
				deck->commander = candidate_card;
				deck_cost = get_deck_cost(deck);
			}
		}
		if (deck_cost > fund)
		{ return false; }
		if (deck->commander != old_commander)
		{
			append_unless_remove(cards_out, cards_in, {-1, old_commander});
			append_unless_remove(cards_in, cards_out, {-1, deck->commander});
		}
	}

	// added backuped deck cards back (place cards strictly before/after card inserted above according to slot index)
	for (signed i = 0; i < (signed)cards.size(); ++ i)
	{
		// try to add cards[i] into the deck, downgrade it if necessary
		const Card * candidate_card = cards[i];
		auto in_it = deck->cards.end() - (i < to_slot); // (before/after according to slot index)
		in_it = deck->cards.insert(in_it, candidate_card);
		deck_cost = get_deck_cost(deck);
		if (!use_top_level_card && (deck_cost > fund))
		{
			while ((deck_cost > fund) && !candidate_card->is_low_level_card())
			{
				candidate_card = candidate_card->downgraded();
				*in_it = candidate_card;
				deck_cost = get_deck_cost(deck);
			}
		}
		if (deck_cost > fund)
		{ return false; }
		if (*in_it != cards[i])
		{
			append_unless_remove(cards_out, cards_in, {is_random ? -1 : i + (i >= from_slot), cards[i]});
			append_unless_remove(cards_in, cards_out, {is_random ? -1 : i + (i >= to_slot), *in_it});
		}
	}
	return !cards_in.empty() || !cards_out.empty();
}

unsigned check_requirement(const Deck* deck, const Requirement & requirement
#ifndef NQUEST
		, const Quest & quest
#endif
		)
{
	unsigned gap = safe_minus(min_deck_len, deck->cards.size());
	if (!requirement.num_cards.empty())
	{
		std::unordered_map<const Card*, unsigned> num_cards;
		num_cards[deck->commander] = 1;
		for (auto card: deck->cards)
		{
			++ num_cards[card];
		}
		for (auto it: requirement.num_cards)
		{
			gap += safe_minus(it.second, num_cards[it.first]);
		}
	}
#ifndef NQUEST
	if (quest.quest_type != QuestType::none)
	{
		unsigned potential_value = 0;
		switch (quest.quest_type)
		{
			case QuestType::skill_use:
			case QuestType::skill_damage:
				for (const auto & ss: deck->commander->m_skills)
				{
					if (quest.quest_key == ss.id)
					{
						potential_value = quest.quest_value;
						break;
					}
				}
				break;
			case QuestType::faction_assault_card_kill:
			case QuestType::type_card_kill:
				potential_value = quest.quest_value;
				break;
			default:
				break;
		}
		for (auto card: deck->cards)
		{
			switch (quest.quest_type)
			{
				case QuestType::skill_use:
				case QuestType::skill_damage:
					for (const auto & ss: card->m_skills)
					{
						if (quest.quest_key == ss.id)
						{
							potential_value = quest.quest_value;
							break;
						}
					}
					break;
				case QuestType::faction_assault_card_use:
					potential_value += (quest.quest_key == card->m_faction);
					break;
				case QuestType::type_card_use:
					potential_value += (quest.quest_key == card->m_type);
					break;
				default:
					break;
			}
			if (potential_value >= (quest.must_fulfill ? quest.quest_value : 1))
			{
				break;
			}
		}
		gap += safe_minus(quest.must_fulfill ? quest.quest_value : 1, potential_value);
	}
#endif
	return gap;
}
