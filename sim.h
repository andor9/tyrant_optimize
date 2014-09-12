#ifndef SIM_H_INCLUDED
#define SIM_H_INCLUDED

#include <boost/pool/pool.hpp>
#include <string>
#include <array>
#include <deque>
#include <tuple>
#include <vector>
#include <map>
#include <random>

#include "tyrant.h"

class Card;
class Cards;
class Deck;
class Field;
class Achievement;

extern unsigned turn_limit;

inline unsigned safe_minus(unsigned x, unsigned y)
{
    return(x - std::min(x, y));
}

//---------------------- Represent Simulation Results ----------------------------
template<typename result_type>
struct Results
{
    result_type wins;
    result_type draws;
    result_type losses;
    result_type points;
    result_type sq_points;
    template<typename other_result_type>
    Results& operator+=(const Results<other_result_type>& other)
    {
        wins += other.wins;
        draws += other.draws;
        losses += other.losses;
        points += other.points;
        sq_points += other.points * other.points;
        return *this;
    }
};

typedef std::pair<std::vector<Results<uint64_t>>, unsigned> EvaluatedResults;

template<typename result_type>
struct FinalResults
{
    result_type wins;
    result_type draws;
    result_type losses;
    result_type points;
    result_type sq_points;
    result_type points_lower_bound;
    result_type points_upper_bound;
    uint64_t n_sims;
};

void fill_skill_table();
Results<uint64_t> play(Field* fd);
void modify_cards(Cards& cards, enum Effect effect);
// Pool-based indexed storage.
//---------------------- Pool-based indexed storage ----------------------------
template<typename T>
class Storage
{
public:
    typedef typename std::vector<T*>::size_type size_type;
    typedef T value_type;
    Storage(size_type size) :
        m_pool(sizeof(T))
    {
        m_indirect.reserve(size);
    }

    inline T& operator[](size_type i)
    {
        return(*m_indirect[i]);
    }

    inline T& add_back()
    {
        m_indirect.emplace_back((T*) m_pool.malloc());
        return(*m_indirect.back());
    }

    template<typename Pred>
    void remove(Pred p)
    {
        size_type head(0);
        for(size_type current(0); current < m_indirect.size(); ++current)
        {
            if(p((*this)[current]))
            {
                m_pool.free(m_indirect[current]);
            }
            else
            {
                if(current != head)
                {
                    m_indirect[head] = m_indirect[current];
                }
                ++head;
            }
        }
        m_indirect.erase(m_indirect.begin() + head, m_indirect.end());
    }

    void reset()
    {
        for(auto index: m_indirect)
        {
            m_pool.free(index);
        }
        m_indirect.clear();
    }

    inline size_type size() const
    {
        return(m_indirect.size());
    }

    std::vector<T*> m_indirect;
    boost::pool<> m_pool;
};
//------------------------------------------------------------------------------
enum class CardStep
{
    none,
    attacking,
    attacked,
};
//------------------------------------------------------------------------------
struct CardStatus
{
    const Card* m_card;
    unsigned m_index;
    unsigned m_player;
    unsigned m_berserk;
    unsigned m_corroded_rate;
    unsigned m_corroded_weakened;
    unsigned m_delay;
    unsigned m_evaded;
    unsigned m_enfeebled;
    Faction m_faction;
    unsigned m_hp;
    unsigned m_inhibited;
    bool m_jammed;
    bool m_overloaded;
    unsigned m_poisoned;
    unsigned m_protected;
    unsigned m_rallied;
    unsigned m_weakened;
    unsigned m_valor;
    CardStep m_step;
    unsigned m_enhanced_value[num_skills];
    unsigned m_skill_cd[num_skills];

    CardStatus() {}

    void set(const Card* card);
    void set(const Card& card);
    std::string description();
    bool has_skill(Skill skill_id) const;
    template<Skill skill_id> bool has_skill() const;
    template<Skill skill_id> unsigned skill() const;
    unsigned enhanced(Skill skill) const;
    unsigned protected_value() const;
};
//------------------------------------------------------------------------------
// Represents a particular draw from a deck.
// Persistent object: call reset to get a new draw.
class Hand
{
public:

    Hand(Deck* deck_) :
        deck(deck_),
        assaults(15),
        structures(15)
    {
    }

    void reset(std::mt19937& re);

    Deck* deck;
    CardStatus commander;
    Storage<CardStatus> assaults;
    Storage<CardStatus> structures;
    unsigned available_summons;
};
//------------------------------------------------------------------------------
// struct Field is the data model of a battle:
// an attacker and a defender deck, list of assaults and structures, etc.
class Field
{
public:
    bool end;
    std::mt19937& re;
    const Cards& cards;
    // players[0]: the attacker, players[1]: the defender
    std::array<Hand*, 2> players;
    unsigned tapi; // current turn's active player index
    unsigned tipi; // and inactive
    Hand* tap;
    Hand* tip;
    std::vector<CardStatus*> selection_array;
    unsigned turn;
    gamemode_t gamemode;
    OptimizationMode optimization_mode;
    const Effect effect;
    SkillSpec bg_skill;
    // With the introduction of on death skills, a single skill can trigger arbitrary many skills.
    // They are stored in this, and cleared after all have been performed.
    std::deque<std::tuple<CardStatus*, SkillSpec>> skill_queue;
    unsigned n_player_kills;
    enum phase
    {
        playcard_phase,
        commander_phase,
        structures_phase,
        assaults_phase,
        end_phase,
    };
    // the current phase of the turn: starts with playcard_phase, then commander_phase, structures_phase, and assaults_phase
    phase current_phase;
    // the index of the card being evaluated in the current phase.
    // Meaningless in playcard_phase,
    // otherwise is the index of the current card in players->structures or players->assaults
    unsigned current_ci;

    Field(std::mt19937& re_, const Cards& cards_, Hand& hand1, Hand& hand2, gamemode_t gamemode_, OptimizationMode optimization_mode_,
            Effect effect_, SkillSpec bg_skill_) :
        end{false},
        re(re_),
        cards(cards_),
        players{{&hand1, &hand2}},
        turn(1),
        gamemode(gamemode_),
        optimization_mode(optimization_mode_),
        effect(effect_),
        bg_skill(bg_skill_),
        n_player_kills(0)
    {
    }

    inline unsigned rand(unsigned x, unsigned y)
    {
        return(std::uniform_int_distribution<unsigned>(x, y)(re));
    }

    inline unsigned flip()
    {
        return(this->rand(0,1));
    }

    template <typename T>
    inline T random_in_vector(const std::vector<T>& v)
    {
        assert(v.size() > 0);
        return(v[this->rand(0, v.size() - 1)]);
    }

    template <typename CardsIter, typename Functor>
    inline unsigned make_selection_array(CardsIter first, CardsIter last, Functor f);
    inline void print_selection_array();
};

#endif
