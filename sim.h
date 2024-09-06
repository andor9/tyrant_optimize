#ifndef SIM_H_INCLUDED
#define SIM_H_INCLUDED

#include <boost/pool/pool.hpp>
#include <string>
#include <array>
#include <deque>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <map>
#include <random>

#include "tyrant.h"

class Card;
class Cards;
class Deck;
class Field;
class Achievement;

extern unsigned turn_limit;

//---------------------- Represent Simulation Results ----------------------------
template<typename result_type>
struct Results
{
    result_type wins = 0;
    result_type draws = 0;
    result_type losses = 0;
    result_type points = 0;
    result_type count = 0; // number of simulations, this is equal to wins + draws + losses in 99% of cases
    template<typename other_result_type>
    Results& operator+=(const Results<other_result_type>& other)
    {
        wins += other.wins;
        draws += other.draws;
        losses += other.losses;
        points += other.points;
        count += other.count;
        return *this;
    }
};

// pair<Results of size enemy_decks.size() * your_dekcs.size(), number of simulations >
typedef std::pair<std::vector<Results<uint64_t>>, unsigned> EvaluatedResults;

template<typename result_type>
struct FinalResults
{
    result_type wins;
    result_type draws;
    result_type losses;
    result_type points;
    result_type points_lower_bound;
    result_type points_upper_bound;
    uint64_t n_sims;
};

typedef std::pair<Deck*,FinalResults<long double>> DeckResults;

template<typename result_type>
bool operator >(const FinalResults<result_type>& x, const FinalResults<result_type>& y) {
    return x.points > y.points;
}

template<typename result_type>
bool operator <(const FinalResults<result_type>& x, const FinalResults<result_type>& y) {
    return x.points < y.points;
}

void fill_skill_table();
Results<uint64_t> play(Field* fd, bool skip_init=false, bool skip_preplay=false , unsigned turns_both=0);
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

    Storage(const Storage &s) :
	m_pool(sizeof(T))
    {
	m_indirect.reserve(s.m_indirect.capacity());
	for(T* t : s.m_indirect)
	{
	     T* c = &add_back();
	     *c=*t;
	}
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
        /*
         *  [a-z]       -   predicate unmatched cards (alive)
         *  [X]         -   predicate matched cards (dead)
         *
         *  m_indirect:     [a][X][b][X][X][c]
         *                   ^ head
         *                   ^ current
         *
         *                   ... (loop logic: shift unmatched cards to left) ...
         *
         *                  [a][b][c][X][X][c]
         *                           |-------> erase
         *                           (allocated memory has been freed for matched cards)
         *
         *                            ^ head
         *                                     ^ current
         *  new m_indirect: [a][b][c]
         */
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

    template<typename Pred>
    unsigned count(Pred p)
    {
        return std::count_if(m_indirect.begin(), m_indirect.end(), p);
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
    unsigned m_action_index;
    unsigned m_player;
    unsigned m_delay;
    unsigned m_hp;
    unsigned m_absorption;
    CardStep m_step;
    unsigned m_perm_health_buff;
    unsigned m_perm_attack_buff;
    signed m_temp_attack_buff;

    unsigned m_corroded_rate;
    unsigned m_corroded_weakened;
    unsigned m_subdued;
    unsigned m_enfeebled;
    unsigned m_evaded;
    unsigned m_inhibited;
    unsigned m_sabotaged;
    unsigned m_paybacked;
    unsigned m_tributed;
    unsigned m_poisoned;
    unsigned m_protected;
    unsigned m_protected_stasis;
    unsigned m_enraged;
    unsigned m_entrapped;
    unsigned m_marked;
    unsigned m_diseased;

    signed m_primary_skill_offset[Skill::num_skills];
    signed m_evolved_skill_offset[Skill::num_skills];
    unsigned m_enhanced_value[Skill::num_skills];
    unsigned m_skill_cd[Skill::num_skills];

    bool m_jammed;
    bool m_overloaded;
    bool m_rush_attempted;
    bool m_sundered;
    bool m_summoned;

    CardStatus() {}

    void set(const Card* card);
    void set(const Card& card);
    std::string description() const;
    inline unsigned skill_base_value(Skill::Skill skill_id) const;
    inline unsigned skill(Skill::Skill skill_id) const;
    inline bool has_skill(Skill::Skill skill_id) const;
    inline unsigned enhanced(Skill::Skill skill) const;
    inline unsigned protected_value() const;
    inline unsigned attack_power() const;
    inline signed calc_attack_power() const;
    inline unsigned max_hp() const;
    inline unsigned add_hp(unsigned value);
    inline unsigned ext_hp(unsigned value);
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
        structures(15),
        stasis_faction_bitmap(0),
        total_cards_destroyed(0),
	total_nonsummon_cards_destroyed(0)
    {
    }

    void reset(std::mt19937& re);

    Deck* deck;
    CardStatus commander;
    Storage<CardStatus> assaults;
    Storage<CardStatus> structures;
    unsigned stasis_faction_bitmap;
    unsigned total_cards_destroyed;
    unsigned total_nonsummon_cards_destroyed;
};

#ifndef NQUEST
struct Quest
{
    QuestType::QuestType quest_type;
    unsigned quest_key;
    unsigned quest_2nd_key;
    unsigned quest_value;
    unsigned quest_score; // score for quest goal
    unsigned win_score;   // score for win regardless quest goal
    bool must_fulfill;  // true: score iff value is reached; false: score proportion to achieved value
    bool must_win;      // true: score only if win
    Quest() :
        quest_type(QuestType::none),
        quest_key(0),
        quest_value(0),
        quest_score(100),
        win_score(0),
        must_fulfill(false),
        must_win(false)
    {}
};
#endif

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
    unsigned flexible_iter = 20;
    unsigned flexible_turn = 20;
    unsigned eval_iter = 10;
    unsigned eval_turn = 20;

    gamemode_t gamemode;
    OptimizationMode optimization_mode;
#ifndef NQUEST
    const Quest quest;
#endif
    std::array<signed short, PassiveBGE::num_passive_bges> bg_effects[2]; // passive BGE
    std::vector<SkillSpec> bg_skills[2]; // active BGE, casted every turn
    // With the introduction of on death skills, a single skill can trigger arbitrary many skills.
    // They are stored in this, and cleared after all have been performed.
    std::deque<std::tuple<CardStatus*, SkillSpec>> skill_queue;
    std::vector<CardStatus*> killed_units;
    std::map<CardStatus*, unsigned> damaged_units_to_times;


    enum phase
    {
        playcard_phase,
        bge_phase,
        commander_phase,
        structures_phase,
        assaults_phase,
        end_phase,
    };
    // the current phase of the turn: starts with playcard_phase, then bge_phase, commander_phase, structures_phase, and assaults_phase
    phase current_phase;
    // the index of the card being evaluated in the current phase.
    // Meaningless in playcard_phase,
    // otherwise is the index of the current card in players->structures or players->assaults
    unsigned current_ci;

    bool assault_bloodlusted;
    unsigned bloodlust_value;
#ifndef NQUEST
    unsigned quest_counter;
#endif

    bool (&fixes)[Fix::num_fixes];// ;

    Field(std::mt19937& re_, const Cards& cards_, Hand& hand1, Hand& hand2, gamemode_t gamemode_, OptimizationMode optimization_mode_,
#ifndef NQUEST
            const Quest & quest_,
#endif
            std::array<signed short, PassiveBGE::num_passive_bges>& your_bg_effects_,
            std::array<signed short, PassiveBGE::num_passive_bges>& enemy_bg_effects_,
            std::vector<SkillSpec>& your_bg_skills_,
            std::vector<SkillSpec>& enemy_bg_skills_,bool (&fixes_)[Fix::num_fixes],
	    unsigned flexible_iter_=20,unsigned flexible_turn_=20,unsigned eval_iter_=10,unsigned eval_turn_=20) :
        end{false},
        re(re_),
        cards(cards_),
        players{{&hand1, &hand2}},
        turn(1),
	    flexible_iter(flexible_iter_),
	    flexible_turn(flexible_turn_),
	    eval_iter(eval_iter_),
	    eval_turn(eval_turn_),

        gamemode(gamemode_),
        optimization_mode(optimization_mode_),
#ifndef NQUEST
        quest(quest_),
#endif
        bg_effects{your_bg_effects_, enemy_bg_effects_},
        bg_skills{your_bg_skills_, enemy_bg_skills_},
        assault_bloodlusted(false),
        bloodlust_value(0),
#ifndef NQUEST
        quest_counter(0),
#endif
		fixes(fixes_)
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
    inline CardStatus * left_assault(const CardStatus * status);
    inline CardStatus * left_assault(const CardStatus * status, const unsigned n);
    inline CardStatus * right_assault(const CardStatus * status);
    inline CardStatus * right_assault(const CardStatus * status, const unsigned n);
    inline void print_selection_array();
    inline void prepare_action();
    inline void finalize_action();

#ifndef NQUEST
    inline void inc_counter(QuestType::QuestType quest_type, unsigned quest_key, unsigned quest_2nd_key = 0, unsigned value = 1)
    {
        if (quest.quest_type == quest_type && quest.quest_key == quest_key && (quest.quest_2nd_key == 0 || quest.quest_2nd_key == quest_2nd_key))
        {
            quest_counter += value;
        }
    }
#endif
};


//------------------------------------------------------------------------------
extern std::string card_name_by_id_safe(const Cards& cards, const unsigned card_id);
//------------------------------------------------------------------------------
template<typename x_type>
inline std::string skill_description(const Cards& cards, const _SkillSpec<x_type>& s, Skill::Trigger trig)
{
    //APN
    return ((trig == Skill::Trigger::play) ? "(On Play)" :
            (trig == Skill::Trigger::attacked) ? "(On Attacked)" :
            (trig == Skill::Trigger::death) ? "(On Death)" : "") +
        skill_names[s.id] +
        (s.card_id == 0 ? "" : " " + card_name_by_id_safe(cards, s.card_id) + " id[" + tuo::to_string(s.card_id) + "]") +
        (s.all ? " all" : s.n == 0 ? "" : std::string(" ") + tuo::to_string(s.n)) +
        (s.y == allfactions ? "" : std::string(" ") + faction_names[s.y]) +
        (s.s == Skill::no_skill ? "" : std::string(" ") + skill_names[s.s]) +
        (s.s2 == Skill::no_skill ? "" : std::string(" ") + skill_names[s.s2]) +
        (s.x == 0 ? "" : std::string(" ") + tuo::to_string(s.x)) +
        (s.c == 0 ? "" : std::string(" every ") + tuo::to_string(s.c));
}

template<typename x_type>
inline std::string skill_description(const Cards& cards, const _SkillSpec<x_type>& s)
{ return skill_description<x_type>(cards, s, Skill::Trigger::activate); }

template<typename x_type>
inline std::string skill_short_description(const Cards& cards, const _SkillSpec<x_type>& s)
{
    return skill_names[s.id] +
        (s.card_id == 0 ? "" : " " + card_name_by_id_safe(cards, s.card_id)) +
        (s.s == Skill::no_skill ? "" : std::string(" ") + skill_names[s.s]) +
        (s.s2 == Skill::no_skill ? "" : std::string(" ") + skill_names[s.s2]) +
        (s.x == 0 ? "" : " " + tuo::to_string(s.x));
}

#endif
