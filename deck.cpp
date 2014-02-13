#include "deck.h"

#include <boost/range/algorithm_ext/insert.hpp>
#include <boost/tokenizer.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "card.h"
#include "cards.h"
#include "read.h"

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
    for (diff_t i = 0; i < m; ++i) {
        std::swap(first[i], first[D(g, param_t(i, n-1))]);
    }
}

//------------------------------------------------------------------------------
std::string deck_hash(const Card* commander, std::vector<const Card*> cards, bool is_ordered)
{
    std::string base64= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::stringstream ios;
    ios << base64[commander->m_id / 64];
    ios << base64[commander->m_id % 64];
    if(!is_ordered)
    {
        std::sort(cards.begin(), cards.end(), [](const Card* a, const Card* b) { return a->m_id < b->m_id; });
    }
    unsigned last_id = 0;
    unsigned num_repeat = 0;
    for(const Card* card: cards)
    {
        unsigned card_id(card->m_id);
        if(card_id == last_id)
        {
            ++ num_repeat;
        }
        else
        {
            if(num_repeat > 1)
            {
                ios << base64[(num_repeat + 4000) / 64];
                ios << base64[(num_repeat + 4000) % 64];
            }
            last_id = card_id;
            num_repeat = 1;
            if(card_id > 4000)
            {
                ios << '-';
                card_id -= 4000;
            }
            ios << base64[card_id / 64];
            ios << base64[card_id % 64];
        }
    }
    if(num_repeat > 1)
    {
        ios << base64[(num_repeat + 4000) / 64];
        ios << base64[(num_repeat + 4000) % 64];
    }
    return ios.str();
}
//------------------------------------------------------------------------------
namespace {
const char* base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

// Converts cards in `hash' to a deck.
// Stores resulting card IDs in `ids'.
void hash_to_ids(const char* hash, std::vector<unsigned>& ids)
{
    unsigned int last_id = 0;
    const char* pc = hash;

    while(*pc)
    {
        unsigned id_plus = 0;
        if(*pc == '-')
        {
            ++ pc;
            id_plus = 4000;
        }
        if(!*pc || !*(pc + 1))
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

const std::pair<std::vector<unsigned>, std::map<signed, char>> string_to_ids(const Cards& all_cards, const std::string& deck_string, const std::string & description)
{
    std::vector<unsigned> card_ids;
    std::map<signed, char> card_marks;
    if(deck_string.find_first_of(":,") == std::string::npos)
    {
        try
        {
            hash_to_ids(deck_string.c_str(), card_ids);
        }
        catch(std::exception& e)
        {
            std::cerr << "Error while resolving " << description << ": " << e.what() << std::endl;
            throw;
        }
    }
    else
    {
        boost::tokenizer<boost::char_delimiters_separator<char>> deck_tokens{deck_string, boost::char_delimiters_separator<char>{false, ":,", ""}};
        auto token_iter = deck_tokens.begin();
        signed p = -1;
        for(; token_iter != deck_tokens.end(); ++token_iter)
        {
            std::string card_spec(*token_iter);
            unsigned card_id{0};
            unsigned card_num{1};
            char num_sign{0};
            char mark{0};
            try
            {
                parse_card_spec(all_cards, card_spec, card_id, card_num, num_sign, mark);
                assert(num_sign == 0);
                for(unsigned i(0); i < card_num; ++i)
                {
                    card_ids.push_back(card_id);
                    if(mark) { card_marks[p] = mark; }
                    ++ p;
                }
            }
            catch(std::exception& e)
            {
                std::cerr << "Warning while resolving " << description << ": " << e.what() << std::endl;
                continue;
            }
        }
    }
    return {card_ids, card_marks};
}

} // end of namespace

namespace range = boost::range;

void Deck::set(const Cards& all_cards, const std::vector<unsigned>& ids, const std::map<signed, char> marks)
{
    commander = nullptr;
    strategy = DeckStrategy::random;
    for(auto id: ids)
    {
        const Card* card{all_cards.by_id(id)};
        if(card->m_type == CardType::commander)
        {
            if(commander == nullptr)
            {
                commander = card;
            }
            else
            {
                throw std::runtime_error("While constructing a deck: two commanders detected (" + card->m_name + " and " + commander->m_name + ")");
            }
        }
        else
        {
            cards.emplace_back(card);
        }
    }
    if(commander == nullptr)
    {
        throw std::runtime_error("While constructing a deck: no commander found");
    }
    card_marks = marks;
}

void Deck::set(const Cards& all_cards, const std::string& deck_string_)
{
    deck_string = deck_string_;
}

void Deck::resolve(const Cards& all_cards)
{
    if(commander != nullptr)
    {
        return;
    }
    auto && id_marks = string_to_ids(all_cards, deck_string, short_description());
    set(all_cards, id_marks.first, id_marks.second);
    deck_string.clear();
}

void Deck::set_given_hand(const Cards& all_cards, const std::string& hand_string)
{
    auto && id_marks = string_to_ids(all_cards, hand_string, "hand");
    given_hand = id_marks.first;
}

std::string Deck::short_description() const
{
    std::stringstream ios;
    ios << decktype_names[decktype];
    if(id > 0) { ios << " #" << id; }
    if(!name.empty()) { ios << " \"" << name << "\""; }
    if(deck_string.empty())
    {
        if(raid_cards.empty()) { ios << ": " << deck_hash(commander, cards, strategy == DeckStrategy::ordered || strategy == DeckStrategy::exact_ordered); }
    }
    else
    {
        ios << ": " << deck_string;
    }
    return ios.str();
}

extern std::string card_description(const Cards& cards, const Card* c);

std::string Deck::long_description(const Cards& all_cards) const
{
    std::stringstream ios;
    ios << short_description() << std::endl;
    if(effect != Effect::none)
    {
        ios << "Effect: " << effect_names[effect] << "\n";
    }
    if(commander)
    {
        ios << card_description(all_cards, commander) << "\n";
    }
    else
    {
        ios << "No commander\n";
    }
    if(!cards.empty() && !raid_cards.empty())
    {
        ios << "Always include:\n";
    }
    for(const Card* card: cards)
    {
        ios << "  " << card_description(all_cards, card) << "\n";
    }
    for(auto& pool: raid_cards)
    {
        ios << pool.first << " from:\n";
        for(auto& card: pool.second)
        {
            ios << "  " << card_description(all_cards, card) << "\n";
        }
    }
    return ios.str();
}

Deck* Deck::clone() const
{
    return(new Deck(*this));
}


const Card* Deck::get_commander()
{
    return(commander);
}

const Card* Deck::next()
{
    if(shuffled_cards.empty())
    {
        return(nullptr);
    }
    else if(strategy == DeckStrategy::random || strategy == DeckStrategy::exact_ordered)
    {
        const Card* card = shuffled_cards.front();
        shuffled_cards.pop_front();
        return(card);
    }
    else if(strategy == DeckStrategy::ordered)
    {
        auto cardIter = std::min_element(shuffled_cards.begin(), shuffled_cards.begin() + std::min<unsigned>(3u, shuffled_cards.size()), [this](const Card* card1, const Card* card2) -> bool
                                         {
                                             auto card1_order = order.find(card1->m_id);
                                             if(!card1_order->second.empty())
                                             {
                                                 auto card2_order = order.find(card2->m_id);
                                                 if(!card2_order->second.empty())
                                                 {
                                                     return(*card1_order->second.begin() < *card2_order->second.begin());
                                                 }
                                                 else
                                                 {
                                                     return(true);
                                                 }
                                             }
                                             else
                                             {
                                                 return(false);
                                             }
                                         });
        auto card = *cardIter;
        shuffled_cards.erase(cardIter);
        auto card_order = order.find(card->m_id);
        if(!card_order->second.empty())
        {
            card_order->second.erase(card_order->second.begin());
        }
        return(card);
    }
    throw std::runtime_error("Unknown strategy for deck.");
}

void Deck::shuffle(std::mt19937& re)
{
    shuffled_cards.clear();
    boost::insert(shuffled_cards, shuffled_cards.end(), cards);
    if(!raid_cards.empty())
    {
        if(strategy != DeckStrategy::random)
        {
            throw std::runtime_error("Support only random strategy for raid/quest deck.");
        }
        for(auto& card_pool: raid_cards)
        {
            assert(card_pool.first <= card_pool.second.size());
            partial_shuffle(card_pool.second.begin(), card_pool.second.begin() + card_pool.first, card_pool.second.end(), re);
            shuffled_cards.insert(shuffled_cards.end(), card_pool.second.begin(), card_pool.second.begin() + card_pool.first);
        }
    }
    if(strategy == DeckStrategy::ordered)
    {
        unsigned i = 0;
        order.clear();
        for(auto card: cards)
        {
            order[card->m_id].push_back(i);
            ++i;
        }
    }
    if(strategy != DeckStrategy::exact_ordered)
    {
        auto shufflable_iter = shuffled_cards.begin();
        for(auto hand_card_id: given_hand)
        {
            auto it = std::find_if(shufflable_iter, shuffled_cards.end(), [hand_card_id](const Card* card) -> bool { return card->m_id == hand_card_id; });
            if(it != shuffled_cards.end())
            {
                std::swap(*shufflable_iter, *it);
                ++ shufflable_iter;
            }
        }
        std::shuffle(shufflable_iter, shuffled_cards.end(), re);
#if 0
        if(!given_hand.empty())
        {
            for(auto card: cards) std::cout << ", " << card->m_name;
            std::cout << std::endl;
            std::cout << strategy;
            for(auto card: shuffled_cards) std::cout << ", " << card->m_name;
            std::cout << std::endl;
        }
#endif
    }
}

void Deck::place_at_bottom(const Card* card)
{
    shuffled_cards.push_back(card);
}

