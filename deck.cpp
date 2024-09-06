#include "deck.h"

#include <boost/range/algorithm_ext/insert.hpp>
#include <boost/tokenizer.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "card.h"
#include "cards.h"
#include "read.h"
#include "sim.h"

	template<class RandomAccessIterator, class UniformRandomNumberGenerator>
void partial_shuffle(RandomAccessIterator first, RandomAccessIterator middle,
		RandomAccessIterator last,
		UniformRandomNumberGenerator&& g)
{
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type diff_t;
	typedef typename std::make_unsigned<diff_t>::type udiff_t;
	typedef typename std::uniform_int_distribution<udiff_t> distr_t;
	typedef typename distr_t::param_type param_t;

	distr_t D;
	diff_t m = middle - first;
	diff_t n = last - first;
	for (diff_t i = 0; i < m; ++i)
	{
		std::swap(first[i], first[D(g, param_t(i, n-1))]);
	}
}

//------------------------------------------------------------------------------
const char* base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";
const char* wmt_b64_magic_chars = "-.~!*_@#$%^&()[]|\\";

// Converts cards in `hash' to a deck.
// Stores resulting card IDs in `ids'.
void hash_to_ids_wmt_b64(const char* hash, std::vector<unsigned>& ids)
{
	unsigned int last_id = 0;
	const char* pc = hash;

	while (*pc)
	{
		unsigned id_plus = 0;
		const char* pmagic = strchr(wmt_b64_magic_chars, *pc);
		if (pmagic)
		{
			++ pc;
			id_plus = 4000 * (pmagic - wmt_b64_magic_chars + 1);
		}
		if (!*pc || !*(pc + 1))
		{
			throw std::runtime_error("Invalid hash length");
		}
		const char* p0 = strchr(base64_chars, *pc);
		const char* p1 = strchr(base64_chars, *(pc + 1));
		if (!p0 || !p1)
		{
			throw std::runtime_error("Invalid hash character");
		}
		pc += 2;
		size_t index0 = p0 - base64_chars;
		size_t index1 = p1 - base64_chars;
		unsigned int id = (index0 << 6) + index1;

		if (id < 4001)
		{
			id += id_plus;
			ids.push_back(id);
			last_id = id;
		}
		else for (unsigned int j = 0; j < id - 4001; ++j)
		{
			ids.push_back(last_id);
		}
	}
}

void encode_id_wmt_b64(std::stringstream &ios, unsigned card_id)
{
	if (card_id > 4000)
	{
		ios << wmt_b64_magic_chars[(card_id - 1) / 4000 - 1];
		card_id = (card_id - 1) % 4000 + 1;
	}
	ios << base64_chars[card_id / 64];
	ios << base64_chars[card_id % 64];
}

void encode_deck_wmt_b64(std::stringstream &ios, std::vector<const Card*> cards)
{
	unsigned last_id = 0;
	unsigned num_repeat = 0;
	for (const Card* card: cards)
	{
		auto card_id = card->m_id;
		if (card_id == last_id)
		{
			++ num_repeat;
		}
		else
		{
			if (num_repeat > 1)
			{
				ios << base64_chars[(num_repeat + 4000) / 64];
				ios << base64_chars[(num_repeat + 4000) % 64];
			}
			last_id = card_id;
			num_repeat = 1;
			encode_id_wmt_b64(ios, card_id);
		}
	}
	if (num_repeat > 1)
	{
		ios << base64_chars[(num_repeat + 4000) / 64];
		ios << base64_chars[(num_repeat + 4000) % 64];
	}
}

void hash_to_ids_ext_b64(const char* hash, std::vector<unsigned>& ids)
{
	const char* pc = hash;
	while (*pc)
	{
		unsigned id = 0;
		unsigned factor = 1;
		const char* p = strchr(base64_chars, *pc);
		if (!p)
		{ throw std::runtime_error("Invalid hash character"); }
		size_t d = p - base64_chars;
		while (d < 32)
		{
			id += factor * d;
			factor *= 32;
			++ pc;
			p = strchr(base64_chars, *pc);
			if (!p)
			{ throw std::runtime_error("Invalid hash character"); }
			d = p - base64_chars;
		}
		id += factor * (d - 32);
		++ pc;
		ids.push_back(id);
	}
}

void encode_id_ext_b64(std::stringstream &ios, unsigned card_id)
{
	while (card_id >= 32)
	{
		ios << base64_chars[card_id % 32];
		card_id /= 32;
	}
	ios << base64_chars[card_id + 32];
}

void encode_deck_ext_b64(std::stringstream &ios, std::vector<const Card*> cards)
{
	for (const Card* card: cards)
	{
		encode_id_ext_b64(ios, card->m_id);
	}
}

void hash_to_ids_ddd_b64(const char* hash, std::vector<unsigned>& ids)
{
	const char* pc = hash;
	while (*pc)
	{
		if (!*pc || !*(pc + 1) || !*(pc + 2))
		{
			throw std::runtime_error("Invalid hash length");
		}
		const char* p0 = strchr(base64_chars, *pc);
		const char* p1 = strchr(base64_chars, *(pc + 1));
		const char* p2 = strchr(base64_chars, *(pc + 2));
		if (!p0 || !p1 || !p2)
		{
			throw std::runtime_error("Invalid hash character");
		}
		pc += 3;
		size_t index0 = p0 - base64_chars;
		size_t index1 = p1 - base64_chars;
		size_t index2 = p2 - base64_chars;
		unsigned int id = (index0 << 12) + (index1 << 6) + index2;
		ids.push_back(id);
	}
}

void encode_id_ddd_b64(std::stringstream &ios, unsigned card_id)
{
	ios << base64_chars[card_id / 4096];
	ios << base64_chars[card_id % 4096 / 64];
	ios << base64_chars[card_id % 64];
}

void encode_deck_ddd_b64(std::stringstream &ios, std::vector<const Card*> cards)
{
	for (const Card* card: cards)
	{
		encode_id_ddd_b64(ios, card->m_id);
	}
}

DeckDecoder hash_to_ids = hash_to_ids_ext_b64;
DeckEncoder encode_deck = encode_deck_ext_b64;

namespace range = boost::range;

void Deck::set(const std::vector<unsigned>& ids, const std::map<signed, char> &marks)
{
	commander = nullptr;
	strategy = DeckStrategy::random;

	int non_deck_cards_seen = 0;
	for (auto id: ids)
	{
		const Card* card{all_cards.by_id(id)};
		if (card->m_type == CardType::commander)
		{
			if (commander == nullptr)
			{
				commander = card;
				if (marks.find(-1) != marks.end())
					card_marks[-1] = marks.at(-1);
			}
			else
			{
				non_deck_cards_seen++;
				std::cerr << "WARNING: Ignoring additional commander " << card->m_name << " (" << commander->m_name << " already in deck)\n";
			}
		}
		else if (card->m_category == CardCategory::dominion_alpha)
		{
			add_dominion(card, false);
			non_deck_cards_seen++;
		}
		else if (card->m_category == CardCategory::fortress_defense || card->m_category == CardCategory::fortress_siege || card->m_category == CardCategory::fortress_conquest)
		{
			fortress_cards.emplace_back(card);
			non_deck_cards_seen++;
		}
		else
		{
			cards.emplace_back(card);
			int mark_dst = cards.size() - 1;
			int mark_src = mark_dst + non_deck_cards_seen;

			if (marks.find(mark_src) != marks.end())
				card_marks[mark_dst] = marks.at(mark_src);
		}
	}
	if (commander == nullptr)
	{
		throw std::runtime_error("While constructing a deck: no commander found");
	}
	commander_max_level = commander->m_top_level_card->m_level;
	deck_size = cards.size();
}

void Deck::set(const std::string& deck_string_)
{
	deck_string = deck_string_;
}

void Deck::resolve()
{
	if (commander != nullptr)
	{
		return;
	}
	auto && id_marks = string_to_ids(all_cards, deck_string, short_description());
	set(id_marks.first, id_marks.second);
	deck_string.clear();
}

void Deck::shrink(const unsigned deck_len)
{
	if (cards.size() > deck_len)
	{
		cards.resize(deck_len);
	}
}

void Deck::set_vip_cards(const std::string& deck_string)
{
	auto && id_marks = string_to_ids(all_cards, deck_string, "vip");
	for (const auto & cid : id_marks.first)
	{
		vip_cards.insert(cid);
	}
}

void Deck::set_given_hand(const std::string& deck_string)
{
	auto && id_marks = string_to_ids(all_cards, deck_string, "hand");
	given_hand = id_marks.first;
}

void Deck::add_forts(const std::string& deck_string)
{
	auto && id_marks = string_to_ids(all_cards, deck_string, "fortress_cards");
	for (auto id: id_marks.first)
	{
		fortress_cards.push_back(all_cards.by_id(id));
	}
}

void Deck::add_pool_forts(const std::string& deck_string, unsigned amount)
{
	auto && id_marks = string_to_ids(all_cards, deck_string, "fortress_cards");
	unsigned replicates{1};
	std::vector<const Card*> cards;
	if(id_marks.first.size() < amount) {
		std::cerr << "WARNING: fortress pool bigger than fortress cards";
	}
	for (auto id: id_marks.first)
	{
		cards.push_back(all_cards.by_id(id));
	}
	variable_forts.push_back(std::make_tuple(amount,replicates,cards));
}

void Deck::add_dominions(const std::string& deck_string, bool override_dom)
{
	auto && id_marks = string_to_ids(all_cards, deck_string, "dominion_cards");
	for (auto id: id_marks.first)
	{
		add_dominion(all_cards.by_id(id), override_dom);
	}
}

void Deck::add_dominion(const Card* dom_card, bool override_dom)
{
	if (dom_card->m_category == CardCategory::dominion_alpha)
	{
		if (alpha_dominion && !override_dom)
		{
			std::cerr << "WARNING: ";
			if (!name.empty()) { std::cerr << "deck " << name << ": "; }
			std::cerr << "Ignoring additional alpha dominion " << dom_card->m_name
				<< " (" << alpha_dominion->m_name << " already in deck)\n";
		}
		else
		{
			if (alpha_dominion)
			{
				std::cerr << "WARNING: ";
				if (!name.empty()) { std::cerr << "deck " << name << ": "; }
				std::cerr << "Overriding alpha dominion " << alpha_dominion->m_name
					<< " by " << dom_card->m_name << std::endl;
			}
			alpha_dominion = dom_card;
		}
	}
	else
	{
		std::cerr << "WARNING: ";
		if (!name.empty()) { std::cerr << "deck " << name << ": "; }
		std::cerr << "Ignoring non-dominion card " << dom_card->m_name << std::endl;
	}
}

std::vector<const Card*> Deck::sorted_ids() const
{
    std::vector<const Card*> deck_all_cards;
	deck_all_cards.emplace_back(commander);
	if (alpha_dominion) { deck_all_cards.emplace_back(alpha_dominion); }
	deck_all_cards.insert(deck_all_cards.end(), cards.begin(), cards.end());
	if (strategy == DeckStrategy::random || strategy == DeckStrategy::flexible || strategy == DeckStrategy::evaluate || strategy == DeckStrategy::evaluate_twice)
	{
		std::sort(deck_all_cards.end() - cards.size(), deck_all_cards.end(), [](const Card* a, const Card* b) { return a->m_id < b->m_id; });
	}
    return deck_all_cards;
}

std::string Deck::hash() const
{
	std::stringstream ios;
	encode_deck(ios, sorted_ids());
	return ios.str();
}

std::string Deck::short_description() const
{
	std::stringstream ios;
	ios << decktype_names[decktype];
	if (id > 0) { ios << " #" << id; }
	if (!name.empty()) { ios << " \"" << name << "\""; }
	if (deck_string.empty())
	{
		if (variable_cards.empty()) { ios << ": " << hash(); }
	}
	else
	{
		ios << ": " << deck_string;
	}
	return ios.str();
}

std::string Deck::medium_description() const
{
	std::stringstream ios;
	ios << short_description() << std::endl;
	if (commander)
	{
		ios << commander->m_name;
	}
	else
	{
		ios << "No commander";
	}

	// dominions
	if (alpha_dominion)
	{ ios << ", " << alpha_dominion->m_name; }

	// fortresses (fixed)
	for (const Card * card: fortress_cards)
	{ ios << ", " << card->m_name; }

	// normal cards (fixed)
	for (const Card * card: cards)
	{ ios << ", " << card->m_name; }

	// fortress (variable)
	unsigned num_pool_cards = 0;
	for (auto& pool: variable_forts)
	{
		num_pool_cards += std::get<0>(pool) * std::get<1>(pool);
	}
	if (num_pool_cards > 0)
	{
		ios << ", and " << num_pool_cards << " fortresses from pool";
	}

	// normal cards (variable)
	num_pool_cards = 0;
	for (auto& pool: variable_cards)
	{
		num_pool_cards += std::get<0>(pool) * std::get<1>(pool);
	}
	if (num_pool_cards > 0)
	{
		ios << ", and " << num_pool_cards << " cards from pool";
	}

	// upgrade points/opports info
	if (upgrade_points > 0)
	{
		ios << " +" << upgrade_points << "/" << upgrade_opportunities;
	}
	return ios.str();
}

extern std::string card_description(const Cards& all_cards, const Card* c);

std::string Deck::long_description() const
{
	std::stringstream ios;
	ios << medium_description() << "\n";
	if (commander)
	{
		show_upgrades(ios, commander, commander_max_level, "");
	}
	else
	{
		ios << "No commander\n";
	}

	// fixed fortresses
	for (const Card * card: fortress_cards)
	{
		show_upgrades(ios, card, card->m_top_level_card->m_level, "");
	}

	// fixed cards
	for (const Card* card: cards)
	{
		show_upgrades(ios, card, card->m_top_level_card->m_level, "  ");
	}

	// variable fortresses
	for (auto& pool: variable_forts)
	{
		if (std::get<1>(pool) > 1)
		{
			ios << std::get<1>(pool) << " copies of each of ";
		}
		ios << std::get<0>(pool) << " in:\n";
		for (auto& card: std::get<2>(pool))
		{
			show_upgrades(ios, card, card->m_top_level_card->m_level, "  ");
		}
	}

	// variable cards
	for (auto& pool: variable_cards)
	{
		if (std::get<1>(pool) > 1)
		{
			ios << std::get<1>(pool) << " copies of each of ";
		}
		ios << std::get<0>(pool) << " in:\n";
		for (auto& card: std::get<2>(pool))
		{
			show_upgrades(ios, card, card->m_top_level_card->m_level, "  ");
		}
	}

	// return formed string
	return ios.str();
}

void Deck::show_upgrades(std::stringstream &ios, const Card* card, unsigned card_max_level, const char * leading_chars) const
{
	ios << leading_chars << card_description(all_cards, card) << "\n";
	if (upgrade_points == 0 || card->m_level == card_max_level)
	{
		return;
	}
	if (debug_print < 2 && decktype != DeckType::raid)
	{
		while (card->m_level != card_max_level)
		{ card = card->upgraded(); }
		ios << leading_chars << "-> " << card_description(all_cards, card) << "\n";
		return;
	}
	// nCm * p^m / q^(n-m)
	double p = 1.0 * upgrade_points / upgrade_opportunities;
	double q = 1.0 - p;
	unsigned n = card_max_level - card->m_level;
	unsigned m = 0;
	double prob = 100.0 * pow(q, n);
	ios << leading_chars << std::fixed << std::setprecision(2) << std::setw(5) << prob << "% no up\n";
	while (card->m_level != card_max_level)
	{
		card = card->upgraded();
		++m;
		prob = prob * (n + 1 - m) / m * p / q;
		ios << leading_chars << std::setw(5) << prob << "% -> " << card_description(all_cards, card) << "\n";
	}
}

Deck* Deck::clone() const
{
	return(new Deck(*this));
}

const Card* Deck::next(Field* f)
{
	if (shuffled_cards.empty())
	{
		return(nullptr);
	}
	else if (strategy == DeckStrategy::random || strategy == DeckStrategy::exact_ordered)
	{
		const Card* card = shuffled_cards.front();
		shuffled_cards.pop_front();
		return(card);
	}
	else if (strategy == DeckStrategy::ordered)
	{
		auto cardIter = std::min_element(
				shuffled_cards.begin(),
				shuffled_cards.begin() + std::min<unsigned>(3u, shuffled_cards.size()),
				[this](const Card* card1, const Card* card2) -> bool {
				auto card1_order = order.find(card1->m_id);
				if (card1_order->second.empty())
				return false;
				auto card2_order = order.find(card2->m_id);
				if (card2_order->second.empty())
				return true;
				return (*card1_order->second.begin() < *card2_order->second.begin());
				}
				);
		auto card = *cardIter;
		shuffled_cards.erase(cardIter);
		auto card_order = order.find(card->m_id);
		if (!card_order->second.empty())
		{
			card_order->second.erase(card_order->second.begin());
		}
		return(card);
	}
	else if (strategy == DeckStrategy::flexible)
	{
		_DEBUG_MSG(1,">>>>FLEX SIMS>>>>\n");
		std::vector<uint64_t> res(std::min<unsigned>(3u,shuffled_cards.size()));
		unsigned iter = f->flexible_iter;

		bool all_same{true};
		for(unsigned j =1; j < res.size();j++)
		{
			if(shuffled_cards.begin()[0]->m_id!=shuffled_cards.begin()[j]->m_id)
			{
				all_same=false;
				break;
			}
		}
		if(all_same || f->flexible_turn*2<f->turn) //no need for flex here, three same or only one card or flexible_turn reached
		{
			const Card* card = shuffled_cards.front();
			shuffled_cards.pop_front();
			return(card);
		}
		for(unsigned j =0; j < res.size();j++)
		{
			bool repeat{false};
			for(unsigned k=0;k<j;++k) //check previous flex sims
			{
				if(shuffled_cards.begin()[j]->m_id==shuffled_cards.begin()[k]->m_id)
				{
					res[j]=res[k]; //copy prev result
					repeat=true;
					break;
				}
			}
			if(repeat)continue; //skip resim
			for(unsigned i =0; i < iter;i++)
			{
				//copy hand
				Hand hand1(*f->players[0]);
				//hand1.deck=hand1.deck->clone();
				Hand hand2(*f->players[1]);
				//hand2.deck = hand2.deck->clone();
				Deck deck1(*hand1.deck);
				Deck deck2(*hand2.deck);
				hand1.deck = &deck1;
				hand2.deck = &deck2;
				hand1.deck->strategy = DeckStrategy::random;
				hand2.deck->strategy = DeckStrategy::random;

				//copy Field
				Field fd(*f);
				fd.players = {{&hand1,&hand2}};
				fd.tap = fd.players[fd.tapi];
				fd.tip = fd.players[fd.tipi];
				fd.selection_array.clear();
				fd.skill_queue.clear();
				fd.killed_units.clear();
				fd.damaged_units_to_times.clear();

				std::swap(fd.tap->deck->shuffled_cards.begin()[0],fd.tap->deck->shuffled_cards.begin()[j]);
				// randomize all following cards
				std::shuffle(++fd.tap->deck->shuffled_cards.begin(),fd.tap->deck->shuffled_cards.end(),f->re);
				//// randomize 2 remaining + 1 random card // worse results
				//unsigned resplusone = std::min<unsigned>(4u,shuffled_cards.size());
				//std::shuffle(++fd.tap->deck->shuffled_cards.begin(),fd.tap->deck->shuffled_cards.begin()+resplusone,f->re);
				std::shuffle(fd.tip->deck->shuffled_cards.begin(),fd.tip->deck->shuffled_cards.end(),f->re);

				Results<uint64_t> result(play(&fd,true,true));
				res[j]+=result.points;
			}
		}

		_DEBUG_MSG(1,"<<<<FLEX SIMS<<<<\n");
		_DEBUG_MSG(1, "Flexible Order: (%s %llu, %s %llu, %s %llu)\n",shuffled_cards[0]->m_name.c_str(),static_cast<unsigned long long>(res[0]/iter),res.size()>1?shuffled_cards[1]->m_name.c_str():"", static_cast<unsigned long long>(res.size()>1?res[1]/iter:0),res.size()>2?shuffled_cards[2]->m_name.c_str():"", static_cast<unsigned long long>(res.size()>2?res[2]/iter:0));
		unsigned best_j = std::distance(res.begin(), (f->tapi==0)?std::max_element(res.begin(), res.end()):std::min_element(res.begin(), res.end())); //max for own flex. enemy flex should optimize him, so min result is best for him
		std::swap(shuffled_cards.begin()[0],shuffled_cards.begin()[best_j]);
		const Card* card = shuffled_cards.front();
		shuffled_cards.pop_front();
		return(card);
	}
	else if (strategy == DeckStrategy::evaluate || strategy == DeckStrategy::evaluate_twice)
	{
		_DEBUG_MSG(1,">>>>EVAL%i SIMS>>>>\n",strategy);
		std::vector<int> res(std::min<unsigned>(3u,shuffled_cards.size()));
		unsigned iter = f->eval_iter;

		bool all_same{true};
		for(unsigned j =1; j < res.size();j++)
		{
			if(shuffled_cards.begin()[0]->m_id!=shuffled_cards.begin()[j]->m_id)
			{
				all_same=false;
				break;
			}
		}
		if(all_same || f->eval_turn*2<f->turn) //no need for flex here, three same or only one card or eval_turn reached
		{
			const Card* card = shuffled_cards.front();
			shuffled_cards.pop_front();
			return(card);
		}
		_DEBUG_MSG(1, ">>EVAL%i List: (%s , %s , %s )\n",strategy,shuffled_cards[0]->m_name.c_str(),res.size()>1?shuffled_cards[1]->m_name.c_str():"", res.size()>2?shuffled_cards[2]->m_name.c_str():"");
		for(unsigned j =0; j < res.size();j++)
		{
			bool repeat{false};
			for(unsigned k=0;k<j;++k) //check previous flex sims
			{
				if(shuffled_cards.begin()[j]->m_id==shuffled_cards.begin()[k]->m_id)
				{
					res[j]=res[k]; //copy prev result
					repeat=true;
					break;
				}
			}
			if(repeat)continue; //skip resim
			for(unsigned i =0; i < iter;i++)
			{
				//copy hand
				Hand hand1(*f->players[0]);
				//hand1.deck=hand1.deck->clone();
				Hand hand2(*f->players[1]);
				//hand2.deck = hand2.deck->clone();
				Deck deck1(*hand1.deck);
				Deck deck2(*hand2.deck);
				hand1.deck = &deck1;
				hand2.deck = &deck2;
				hand1.deck->strategy = DeckStrategy::random;
				hand2.deck->strategy = DeckStrategy::random;
				

				//copy Field
				Field fd(*f);
				fd.players = {{&hand1,&hand2}};
				fd.tap = fd.players[fd.tapi];
				fd.tip = fd.players[fd.tipi];
				fd.selection_array.clear();
				fd.skill_queue.clear();
				fd.killed_units.clear();
				fd.damaged_units_to_times.clear();
				fd.eval_iter =1;

				std::swap(fd.tap->deck->shuffled_cards.begin()[0],fd.tap->deck->shuffled_cards.begin()[j]);
				// randomize all following cards
				//std::shuffle(++fd.tap->deck->shuffled_cards.begin(),fd.tap->deck->shuffled_cards.end(),f->re);
				// randomize 2 remaining + 1 random card // worse results
				unsigned resplusone = std::min<unsigned>(4u,shuffled_cards.size());
				std::shuffle(++fd.tap->deck->shuffled_cards.begin(),fd.tap->deck->shuffled_cards.begin()+resplusone,f->re);
				std::shuffle(fd.tip->deck->shuffled_cards.begin(),fd.tip->deck->shuffled_cards.end(),f->re);

				Results<uint64_t> result(play(&fd,true,true,1));
				if (result.wins == 0 && result.losses ==0 && strategy == DeckStrategy::evaluate_twice) {
					_DEBUG_MSG(1,">>>>>>EVAL%i SIMS>>>>>>\n",strategy);
					if(f->players[0]->deck->strategy==DeckStrategy::evaluate_twice)hand1.deck->strategy = DeckStrategy::evaluate;
					else hand1.deck->strategy = DeckStrategy::random;
					if(f->players[1]->deck->strategy==DeckStrategy::evaluate_twice)hand2.deck->strategy = DeckStrategy::evaluate;
					else hand2.deck->strategy = DeckStrategy::random;
					result=(play(&fd,true,false,1));
					_DEBUG_MSG(1,"<<<<<<EVAL%i SIMS<<<<<<\n",strategy);
				}
				res[j]+=(1-2*result.draws)*result.points;
			}
		}

		_DEBUG_MSG(1,"<<<<EVAL%i SIMS<<<<\n",strategy);
		_DEBUG_MSG(1, "<<EVAL%i Order: (%s %i, %s %i, %s %i)\n",strategy,shuffled_cards[0]->m_name.c_str(),static_cast<int>(res[0]/iter),res.size()>1?shuffled_cards[1]->m_name.c_str():"", static_cast<int>(res.size()>1?res[1]/iter:0),res.size()>2?shuffled_cards[2]->m_name.c_str():"", static_cast<int>(res.size()>2?res[2]/iter:0));
		unsigned best_j = std::distance(res.begin(), (f->tapi==0)?std::max_element(res.begin(), res.end()):std::min_element(res.begin(), res.end())); //max for own flex. enemy flex should optimize him, so min result is best for him
		std::swap(shuffled_cards.begin()[0],shuffled_cards.begin()[best_j]);
		const Card* card = shuffled_cards.front();
		shuffled_cards.pop_front();
		return(card);

	}
	throw std::runtime_error("Unknown strategy for deck.");
}

const Card* Deck::upgrade_card(const Card* card, unsigned card_max_level, std::mt19937& re, unsigned &remaining_upgrade_points, unsigned &remaining_upgrade_opportunities)
{
	unsigned oppos = card_max_level - card->m_level;
	if (remaining_upgrade_points > 0)
	{
		for (; oppos > 0; -- oppos)
		{
			std::mt19937::result_type rnd = re();
			if (rnd % remaining_upgrade_opportunities < remaining_upgrade_points)
			{
				card = card->upgraded();
				-- remaining_upgrade_points;
			}
			-- remaining_upgrade_opportunities;
		}
	}
	return card;
}

void Deck::shuffle(std::mt19937& re)
{
	shuffled_commander = commander;
	shuffled_forts.clear();
	boost::insert(shuffled_forts, shuffled_forts.end(), fortress_cards);
	shuffled_cards.clear();
	boost::insert(shuffled_cards, shuffled_cards.end(), cards);
	if (!variable_forts.empty())
	{
		if (decktype == DeckType::raid && strategy != DeckStrategy::random)
		{
			throw std::runtime_error("Support only random strategy for raid/quest deck.");
		}
		for (auto& card_pool: variable_forts)
		{
			auto & amount = std::get<0>(card_pool);
			auto & replicates = std::get<1>(card_pool);
			auto & card_list = std::get<2>(card_pool);
			assert(amount <= card_list.size());
			partial_shuffle(card_list.begin(), card_list.begin() + amount, card_list.end(), re);
			for (unsigned rep = 0; rep < replicates; ++ rep)
			{
				shuffled_forts.insert(shuffled_forts.end(), card_list.begin(), card_list.begin() + amount);
			}
		}
	}
	if (!variable_cards.empty())
	{
		if (decktype == DeckType::raid && strategy != DeckStrategy::random)
		{
			throw std::runtime_error("Support only random strategy for raid/quest deck.");
		}
		for (auto& card_pool: variable_cards)
		{
			auto & amount = std::get<0>(card_pool);
			auto & replicates = std::get<1>(card_pool);
			auto & card_list = std::get<2>(card_pool);
			assert(amount <= card_list.size());
			partial_shuffle(card_list.begin(), card_list.begin() + amount, card_list.end(), re);
			for (unsigned rep = 0; rep < replicates; ++ rep)
			{
				shuffled_cards.insert(shuffled_cards.end(), card_list.begin(), card_list.begin() + amount);
			}
		}
	}
	if (upgrade_points > 0)
	{
		unsigned remaining_upgrade_points = upgrade_points;
		std::vector<std::pair<std::deque<const Card*>*, unsigned>> up_cards;
		std::deque<const Card*> commander_storage;
		commander_storage.emplace_back(shuffled_commander);
		up_cards.emplace_back(&commander_storage, 0);
		for (unsigned index(0); index < shuffled_forts.size(); ++ index)
		{ up_cards.emplace_back(&shuffled_forts, index); }
		for (unsigned index(0); index < shuffled_cards.size(); ++ index)
		{ up_cards.emplace_back(&shuffled_cards, index); }

		// distribute upgrade points randomly (no gaussian/poisson distribution)
		while (remaining_upgrade_points && up_cards.size())
		{
			unsigned idx = re() % up_cards.size();
			std::pair<std::deque<const Card*>*, unsigned> x_pair = up_cards.at(idx);
			std::deque<const Card*>* storage_ptr = x_pair.first;
			unsigned storage_idx = x_pair.second;
			const Card* card = storage_ptr->at(storage_idx);
			if (card->is_top_level_card())
			{
				up_cards.erase(up_cards.begin() + idx);
				continue;
			}
			(*storage_ptr)[storage_idx] = card->upgraded();
			-- remaining_upgrade_points;
		}
		shuffled_commander = commander_storage[0];
	}
	if (strategy == DeckStrategy::ordered)
	{
		unsigned i = 0;
		order.clear();
		for (auto card: cards)
		{
			order[card->m_id].push_back(i);
			++i;
		}
	}
	if (strategy != DeckStrategy::exact_ordered)
	{
		std::deque<const Card*>* pools[] = { &shuffled_forts, &shuffled_cards };
		for (std::deque<const Card*>* pool : pools)
		{
			auto shufflable_iter = pool->begin();
			for (auto hand_card_id: given_hand)
			{
				auto it = std::find_if (shufflable_iter, pool->end(),
						[hand_card_id](const Card* card) -> bool { return card->m_id == hand_card_id; });
				if (it != pool->end())
				{
					std::swap(*shufflable_iter, *it);
					++ shufflable_iter;
				}
			}
			std::shuffle(shufflable_iter, pool->end(), re);
		}
#if 0
		if (!given_hand.empty())
		{
			for (auto card: cards) std::cout << ", " << card->m_name;
			std::cout << std::endl;
			std::cout << strategy;
			for (auto card: shuffled_cards) std::cout << ", " << card->m_name;
			std::cout << std::endl;
		}
#endif
	}
#ifndef NDEBUG
	if (upgrade_points > 0)
	{
		_DEBUG_MSG(2, " ** upgraded cards:\n");
		_DEBUG_MSG(2, " >> Commander: %s\n", shuffled_commander->m_name.c_str());
		for (auto * card: shuffled_forts)
		{
			_DEBUG_MSG(2, " >> Fortress: %s\n", card->m_name.c_str());
		}
		for (auto * card: shuffled_cards)
		{
			_DEBUG_MSG(2, " >> Card from pool: %s\n", card->m_name.c_str());
		}
	}
#endif
}

void Deck::place_at_bottom(const Card* card)
{
	shuffled_cards.push_back(card);
}

void Decks::add_deck(Deck* deck, const std::string& deck_name)
{
	by_name[deck_name] = deck;
	by_name[simplify_name(deck_name)] = deck;
}

Deck* Decks::find_deck_by_name(const std::string& deck_name)
{
	auto it = by_name.find(simplify_name(deck_name));
	return it == by_name.end() ? nullptr : it->second;
}
