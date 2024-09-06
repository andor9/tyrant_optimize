#ifndef DECK_H_INCLUDED
#define DECK_H_INCLUDED

#include <deque>
#include <list>
#include <map>
#include <random>
#include <set>
#include <vector>

#include "tyrant.h"
#include "card.h"
#include "sim.h"

class Cards;

//---------------------- $30 Deck: a commander + a sequence of cards -----------
// Can be shuffled.
// Implementations: random player and raid decks, ordered player decks.
//------------------------------------------------------------------------------
namespace DeckStrategy
{
enum DeckStrategy
{
    random,
    ordered,
    exact_ordered,
    flexible,
    evaluate,
    evaluate_twice,
    num_deckstrategies
};
}
typedef void (*DeckDecoder)(const char* hash, std::vector<unsigned>& ids);
typedef void (*DeckEncoder)(std::stringstream &ios, std::vector<const Card*> cards);
void hash_to_ids_wmt_b64(const char* hash, std::vector<unsigned>& ids);
void encode_deck_wmt_b64(std::stringstream &ios, std::vector<const Card*> cards);
void hash_to_ids_ext_b64(const char* hash, std::vector<unsigned>& ids);
void encode_deck_ext_b64(std::stringstream &ios, std::vector<const Card*> cards);
void hash_to_ids_ddd_b64(const char* hash, std::vector<unsigned>& ids);
void encode_deck_ddd_b64(std::stringstream &ios, std::vector<const Card*> cards);

void encode_id_ext_b64(std::stringstream &ios, unsigned card_id);
extern DeckDecoder hash_to_ids;
extern DeckEncoder encode_deck;

//------------------------------------------------------------------------------
// No support for ordered raid decks
class Deck
{
public:
    const Cards& all_cards;
    DeckType::DeckType decktype;
    unsigned id;
    std::string name;
    unsigned upgrade_points;
    unsigned upgrade_opportunities;
    DeckStrategy::DeckStrategy strategy;

    const Card* commander;
    const Card* alpha_dominion;
    unsigned commander_max_level;
    std::vector<const Card*> cards;
    std::map<signed, char> card_marks;  // <positions of card, prefix mark>: -1 indicating the commander. E.g, used as a mark to be kept in attacking deck when optimizing.

    const Card* shuffled_commander;
    std::deque<const Card*> shuffled_forts;
    std::deque<const Card*> shuffled_cards;

    // card id -> card order
    std::map<unsigned, std::list<unsigned>> order;
    std::vector<std::tuple<unsigned, unsigned, std::vector<const Card*>>> variable_forts;  // amount, replicates, card pool
    std::vector<std::tuple<unsigned, unsigned, std::vector<const Card*>>> variable_cards;  // amount, replicates, card pool
    unsigned deck_size;
    unsigned mission_req;
    unsigned level;

    std::string deck_string;
    std::unordered_set<unsigned> vip_cards;
    std::vector<unsigned> given_hand;
    std::vector<const Card*> fortress_cards;
    std::vector<SkillSpecXMult> effects;

    Deck(
        const Cards& all_cards_,
        DeckType::DeckType decktype_ = DeckType::deck,
        unsigned id_ = 0,
        std::string name_ = "",
        unsigned upgrade_points_ = 0,
        unsigned upgrade_opportunities_ = 0,
        DeckStrategy::DeckStrategy strategy_ = DeckStrategy::random) :
        all_cards(all_cards_),
        decktype(decktype_),
        id(id_),
        name(name_),
        upgrade_points(upgrade_points_),
        upgrade_opportunities(upgrade_opportunities_),
        strategy(strategy_),
        commander(nullptr),
        alpha_dominion(nullptr),
        shuffled_commander(nullptr),
        deck_size(0),
        mission_req(0)
    {
    }


    ~Deck() {}

    void set(
        const Card* commander_,
        unsigned commander_max_level_,
        const std::vector<const Card*>& cards_,
        std::vector<std::tuple<unsigned, unsigned, std::vector<const Card*>>> variable_forts_ = {},
        std::vector<std::tuple<unsigned, unsigned, std::vector<const Card*>>> variable_cards_ = {},
        unsigned mission_req_ = 0)
    {
        commander = commander_;
        commander_max_level = commander_max_level_;
        cards = std::vector<const Card*>(std::begin(cards_), std::end(cards_));
        variable_forts = variable_forts_;
        variable_cards = variable_cards_;
        deck_size = cards.size();
        for (const auto & pool: variable_cards)
        {
            deck_size += std::get<0>(pool) * std::get<1>(pool);
        }
        mission_req = mission_req_;
    }

    void set(const std::vector<unsigned>& ids, const std::map<signed, char> &marks);
    void set(const std::vector<unsigned>& ids)
    {
        std::map<signed, char> empty;
        set(ids, empty);
    }
    void set(const std::string& deck_string_);
    void resolve();
    void shrink(const unsigned deck_len);
    void set_vip_cards(const std::string& deck_string_);
    void set_given_hand(const std::string& deck_string_);
    void add_forts(const std::string& deck_string_);
    void add_pool_forts(const std::string& deck_string_,unsigned amount);
    void add_dominions(const std::string& deck_string_, bool override_dom);
    void add_dominion(const Card* dom_card, bool override_dom);

    Deck* clone() const;
    std::vector<const Card*> sorted_ids() const;
    std::string hash() const;
    std::string short_description() const;
    std::string medium_description() const;
    std::string long_description() const;
    void show_upgrades(std::stringstream &ios, const Card* card, unsigned card_max_level, const char * leading_chars) const;
    const Card* next(Field* f);
    const Card* upgrade_card(const Card* card, unsigned card_max_level, std::mt19937& re, unsigned &remaining_upgrade_points, unsigned &remaining_upgrade_opportunities);
    void shuffle(std::mt19937& re);
    void place_at_bottom(const Card* card);
};

typedef std::map<std::string, long double> DeckList;
class Decks
{
public:
    void add_deck(Deck* deck, const std::string& deck_name);
    Deck* find_deck_by_name(const std::string& deck_name);
    std::list<Deck> decks;
    std::map<std::pair<DeckType::DeckType, unsigned>, Deck*> by_type_id;
    std::map<std::string, Deck*> by_name;
};

#endif
