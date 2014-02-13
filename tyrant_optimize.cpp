// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//------------------------------------------------------------------------------
//#define NDEBUG
#define BOOST_THREAD_USE_LIB
#include <cassert>
#include <cstring>
#include <ctime>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <tuple>
#include <boost/range/join.hpp>
#include <boost/optional.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/math/distributions/binomial.hpp>
#include "card.h"
#include "cards.h"
#include "deck.h"
#include "achievement.h"
#include "read.h"
#include "sim.h"
#include "tyrant.h"
#include "xml.h"
//#include "timer.hpp"

namespace {
    gamemode_t gamemode{fight};
    OptimizationMode optimization_mode{OptimizationMode::winrate};
    std::map<unsigned, unsigned> owned_cards;
    std::map<unsigned, unsigned> buyable_cards;
    bool use_owned_cards{false};
    unsigned min_deck_len{1};
    unsigned max_deck_len{10};
    unsigned fund{0};
    bool auto_upgrade_cards{true};
    long double target_score{100};
    bool show_stdev{false};
    bool use_harmonic_mean{false};
}

using namespace std::placeholders;
//------------------------------------------------------------------------------
std::string card_id_name(const Card* card)
{
    std::stringstream ios;
    if(card)
    {
        ios << "[" << card->m_id << "] " << card->m_name;
    }
    else
    {
        ios << "-void-";
    }
    return ios.str();
}
//------------------------------------------------------------------------------
Deck* find_deck(Decks& decks, const Cards& cards, std::string deck_name)
{
    auto it = decks.by_name.find(deck_name);
    if(it != decks.by_name.end())
    {
        it->second->resolve(cards);
        return(it->second);
    }
    decks.decks.push_back(Deck{});
    Deck* deck = &decks.decks.back();
    deck->set(cards, deck_name);
    deck->resolve(cards);
    return(deck);
}
//---------------------- $80 deck optimization ---------------------------------

// @claim_all: true = claim all cards; false = claim only non-buyable cards.
// @is_reward: not claim a card if there is upgraded version (assuming the reward card has been upgraded).
void claim_cards(const std::vector<const Card*> & card_list, const Cards & cards, bool claim_all, bool is_reward)
{
    std::map<const Card *, unsigned> num_cards;
    for(const Card * card: card_list)
    {
        if(card->m_proto_id > 0 && auto_upgrade_cards && owned_cards[card->m_id] <= num_cards[card])
        {
            const Card * proto_card = cards.by_id(card->m_proto_id);
            num_cards[proto_card] += proto_card->m_upgrade_consumables;
        }
        else
        {
            num_cards[card] += 1;
        }
    }
    for(auto it: num_cards)
    {
        const Card * card = it.first;
        if(claim_all || buyable_cards.find(card->m_id) == buyable_cards.end())
        {
            unsigned num_to_claim = safe_minus(it.second, owned_cards[card->m_id] + (is_reward ? card->m_upgrade_consumables * owned_cards[card->m_upgraded_id] : 0));
            if(num_to_claim > 0)
            {
                owned_cards[card->m_id] += num_to_claim;
                if(debug_print)
                {
                    std::cout << "Claim " << card->m_name << " (" << num_to_claim << ")" << std::endl;
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
bool suitable_non_commander(const Deck& deck, unsigned slot, const Card* card)
{
    assert(card->m_type != CardType::commander);
    if(card->m_rarity == 4) // legendary - 1 per deck
    {
        for(unsigned i(0); i < deck.cards.size(); ++i)
        {
            if(i != slot && deck.cards[i]->m_rarity == 4)
            {
                return false;
            }
        }
    }
    if(card->m_unique) // unique - 1 card with same id per deck
    {
        for(unsigned i(0); i < deck.cards.size(); ++i)
        {
            if(i != slot && deck.cards[i]->m_base_id == card->m_base_id)
            {
                return false;
            }
        }
    }
    return true;
}

unsigned get_deck_cost(const Deck * deck, const Cards & cards)
{
    if(!use_owned_cards)
    {
        return 0;
    }
    std::map<unsigned, unsigned> num_in_deck;
    unsigned deck_cost = 0;
    const Card * card = deck->commander;
    if(card->m_proto_id > 0 && auto_upgrade_cards && owned_cards[card->m_id] == 0)
    {
        const Card * proto_card = cards.by_id(card->m_proto_id);
        num_in_deck[proto_card->m_id] += proto_card->m_upgrade_consumables;
        deck_cost += proto_card->m_upgrade_gold_cost;
    }
    else
    {
        num_in_deck[card->m_id] += 1;
    }
    for(const Card * card: deck->cards)
    {
        if(card->m_proto_id > 0 && auto_upgrade_cards && owned_cards[card->m_id] <= num_in_deck[card->m_id])
        {
            const Card * proto_card = cards.by_id(card->m_proto_id);
            num_in_deck[proto_card->m_id] += proto_card->m_upgrade_consumables;
            deck_cost += proto_card->m_upgrade_gold_cost;
        }
        else
        {
            num_in_deck[card->m_id] += 1;
        }
    }
    for(auto it: num_in_deck)
    {
        unsigned card_id = it.first;
        unsigned num_to_buy = safe_minus(it.second, owned_cards[card_id]);
        if(num_to_buy > 0)
        {
            auto buyable_iter = buyable_cards.find(card_id);
            if(buyable_iter == buyable_cards.end()) { return UINT_MAX; }
            deck_cost += num_to_buy * buyable_iter->second;
        }
    }
    return deck_cost;
}

//------------------------------------------------------------------------------
Results<long double> compute_score(const std::pair<std::vector<Results<uint64_t>> , unsigned>& results, std::vector<long double>& factors)
{
    Results<long double> final{0, 0, 0, 0, 0};
    for(unsigned index(0); index < results.first.size(); ++index)
    {
        final.wins += results.first[index].wins * factors[index];
        final.draws += results.first[index].draws * factors[index];
        final.losses += results.first[index].losses * factors[index];
        if(use_harmonic_mean)
        { final.points += factors[index] / results.first[index].points; }
        else
        { final.points += results.first[index].points * factors[index]; }
        final.sq_points += results.first[index].sq_points * factors[index] * factors[index];
    }
    long double factor_sum = std::accumulate(factors.begin(), factors.end(), 0.);
    final.wins /= factor_sum * (long double)results.second;
    final.draws /= factor_sum * (long double)results.second;
    final.losses /= factor_sum * (long double)results.second;
    if(use_harmonic_mean)
    { final.points = factor_sum / ((long double)results.second * final.points); }
    else
    { final.points /= factor_sum * (long double)results.second; }
    final.sq_points /= factor_sum * factor_sum * (long double)results.second;
    return final;
}
//------------------------------------------------------------------------------
volatile unsigned thread_num_iterations{0}; // written by threads
std::vector<Results<uint64_t>> thread_results; // written by threads
volatile unsigned thread_total{0}; // written by threads
volatile long double thread_prev_score{0.0};
volatile bool thread_compare{false};
volatile bool thread_compare_stop{false}; // written by threads
volatile bool destroy_threads;
//------------------------------------------------------------------------------
// Per thread data.
// seed should be unique for each thread.
// d1 and d2 are intended to point to read-only process-wide data.
struct SimulationData
{
    std::mt19937 re;
    const Cards& cards;
    const Decks& decks;
    std::shared_ptr<Deck> att_deck;
    Hand att_hand;
    std::vector<std::shared_ptr<Deck>> def_decks;
    std::vector<Hand*> def_hands;
    std::vector<long double> factors;
    gamemode_t gamemode;
    enum Effect effect;
    const Achievement& achievement;

    SimulationData(unsigned seed, const Cards& cards_, const Decks& decks_, unsigned num_def_decks_, std::vector<long double> factors_, gamemode_t gamemode_, enum Effect effect_, const Achievement& achievement_) :
        re(seed),
        cards(cards_),
        decks(decks_),
        att_deck(),
        att_hand(nullptr),
        def_decks(num_def_decks_),
        factors(factors_),
        gamemode(gamemode_),
        effect(effect_),
        achievement(achievement_)
    {
        for(auto def_deck: def_decks)
        {
            def_hands.emplace_back(new Hand(nullptr));
        }
    }

    ~SimulationData()
    {
        for(auto hand: def_hands) { delete(hand); }
    }

    void set_decks(const Deck* const att_deck_, std::vector<Deck*> const & def_decks_)
    {
        att_deck.reset(att_deck_->clone());
        att_hand.deck = att_deck.get();
        for(unsigned i(0); i < def_decks_.size(); ++i)
        {
            def_decks[i].reset(def_decks_[i]->clone());
            def_hands[i]->deck = def_decks[i].get();
        }
    }

    inline std::vector<Results<uint64_t>> evaluate()
    {
        std::vector<Results<uint64_t>> res;
        for(Hand* def_hand: def_hands)
        {
            att_hand.reset(re);
            def_hand->reset(re);
            Field fd(re, cards, att_hand, *def_hand, gamemode, optimization_mode, effect != Effect::none ? effect : def_hand->deck->effect, achievement);
            Results<uint64_t> result(play(&fd));
            res.emplace_back(result);
        }
        return(res);
    }
};
//------------------------------------------------------------------------------
class Process;
void thread_evaluate(boost::barrier& main_barrier,
                     boost::mutex& shared_mutex,
                     SimulationData& sim,
                     const Process& p,
                     unsigned thread_id);
//------------------------------------------------------------------------------
class Process
{
public:
    unsigned num_threads;
    std::vector<boost::thread*> threads;
    std::vector<SimulationData*> threads_data;
    boost::barrier main_barrier;
    boost::mutex shared_mutex;
    const Cards& cards;
    const Decks& decks;
    Deck* att_deck;
    const std::vector<Deck*> def_decks;
    std::vector<long double> factors;
    gamemode_t gamemode;
    enum Effect effect;
    Achievement achievement;

    Process(unsigned _num_threads, const Cards& cards_, const Decks& decks_, Deck* att_deck_, std::vector<Deck*> _def_decks, std::vector<long double> _factors, gamemode_t _gamemode, enum Effect _effect, const Achievement& achievement_) :
        num_threads(_num_threads),
        main_barrier(num_threads+1),
        cards(cards_),
        decks(decks_),
        att_deck(att_deck_),
        def_decks(_def_decks),
        factors(_factors),
        gamemode(_gamemode),
        effect(_effect),
        achievement(achievement_)
    {
        destroy_threads = false;
        unsigned seed(time(0));
        for(unsigned i(0); i < num_threads; ++i)
        {
            threads_data.push_back(new SimulationData(seed + i, cards, decks, def_decks.size(), factors, gamemode, effect, achievement));
            threads.push_back(new boost::thread(thread_evaluate, std::ref(main_barrier), std::ref(shared_mutex), std::ref(*threads_data.back()), std::ref(*this), i));
        }
    }

    ~Process()
    {
        destroy_threads = true;
        main_barrier.wait();
        for(auto thread: threads) { thread->join(); }
        for(auto data: threads_data) { delete(data); }
    }

    std::pair<std::vector<Results<uint64_t>> , unsigned> evaluate(unsigned num_iterations)
    {
        thread_num_iterations = num_iterations;
        thread_results = std::vector<Results<uint64_t>>(def_decks.size());
        thread_total = 0;
        thread_compare = false;
        // unlock all the threads
        main_barrier.wait();
        // wait for the threads
        main_barrier.wait();
        return(std::make_pair(thread_results, thread_total));
    }

    std::pair<std::vector<Results<uint64_t>> , unsigned> compare(unsigned num_iterations, long double prev_score)
    {
        thread_num_iterations = num_iterations;
        thread_results = std::vector<Results<uint64_t>>(def_decks.size());
        thread_total = 0;
        thread_prev_score = prev_score;
        thread_compare = true;
        thread_compare_stop = false;
        // unlock all the threads
        main_barrier.wait();
        // wait for the threads
        main_barrier.wait();
        return(std::make_pair(thread_results, thread_total));
    }
};
//------------------------------------------------------------------------------
void thread_evaluate(boost::barrier& main_barrier,
                     boost::mutex& shared_mutex,
                     SimulationData& sim,
                     const Process& p,
                     unsigned thread_id)
{
    while(true)
    {
        main_barrier.wait();
        sim.set_decks(p.att_deck, p.def_decks);
        if(destroy_threads) { return; }
        while(true)
        {
            shared_mutex.lock(); //<<<<
            if(thread_num_iterations == 0 || (thread_compare && thread_compare_stop)) //!
            {
                shared_mutex.unlock(); //>>>>
                main_barrier.wait();
                break;
            }
            else
            {
                --thread_num_iterations; //!
                shared_mutex.unlock(); //>>>>
                std::vector<Results<uint64_t>> result{sim.evaluate()};
                shared_mutex.lock(); //<<<<
                std::vector<unsigned> thread_score_local(thread_results.size(), 0u); //!
                for(unsigned index(0); index < result.size(); ++index)
                {
                    thread_results[index] += result[index]; //!
                    thread_score_local[index] = thread_results[index].points; //!
                }
                ++thread_total; //!
                unsigned thread_total_local{thread_total}; //!
                shared_mutex.unlock(); //>>>>
                if(thread_compare && thread_id == 0 && thread_total_local > 1)
                {
                    unsigned score_accum = 0;
                    // Multiple defense decks case: scaling by factors and approximation of a "discrete" number of events.
                    if(result.size() > 1)
                    {
                        long double score_accum_d = 0.0;
                        for(unsigned i = 0; i < thread_score_local.size(); ++i)
                        {
                            score_accum_d += thread_score_local[i] * sim.factors[i];
                        }
                        score_accum_d /= std::accumulate(sim.factors.begin(), sim.factors.end(), .0d);
                        score_accum = score_accum_d;
                    }
                    else
                    {
                        score_accum = thread_score_local[0];
                    }
                    bool compare_stop(false);
                    long double best_possible = (optimization_mode == OptimizationMode::raid ? 250 : 100);
                    // Get a loose (better than no) upper bound. TODO: Improve it.
                    compare_stop = (boost::math::binomial_distribution<>::find_upper_bound_on_p(thread_total_local, score_accum / best_possible, 0.01) * best_possible < thread_prev_score);
                    if(compare_stop) {
                        shared_mutex.lock(); //<<<<
                        //std::cout << thread_total_local << "\n";
                        thread_compare_stop = true; //!
                        shared_mutex.unlock(); //>>>>
                    }
                }
            }
        }
    }
}
//------------------------------------------------------------------------------
void print_score_info(const std::pair<std::vector<Results<uint64_t>> , unsigned>& results, std::vector<long double>& factors)
{
    auto final = compute_score(results, factors);
    std::cout << final.points << " (";
    for(auto val: results.first)
    {
        switch(optimization_mode)
        {
            case OptimizationMode::raid:
                std::cout << val.points << " ";
                break;
            default:
                std::cout << val.points / 100 << " ";
                break;
        }
    }
    std::cout << "/ " << results.second << ")" << std::endl;
}
//------------------------------------------------------------------------------
void print_results(const std::pair<std::vector<Results<uint64_t>> , unsigned>& results, std::vector<long double>& factors)
{
    auto final = compute_score(results, factors);

    if(optimization_mode == OptimizationMode::raid)
    {
        std::cout <<  "win%: " << (final.wins + final.draws) * 100.0 << " (";
        for(auto val: results.first)
        {
            std::cout << val.wins + val.draws << " ";
        }
        std::cout << "/ " << results.second << ")" << std::endl;
    }

    std::cout << (optimization_mode == OptimizationMode::raid ? "slay%: " : "win%: ") << final.wins * 100.0 << " (";
    for(auto val: results.first)
    {
        std::cout << val.wins << " ";
    }
    std::cout << "/ " << results.second << ")" << std::endl;

    if(optimization_mode != OptimizationMode::raid)
    {
        std::cout << "stall%: " << final.draws * 100.0 << " (";
        for(auto val: results.first)
        {
            std::cout << val.draws << " ";
        }
        std::cout << "/ " << results.second << ")" << std::endl;
    }

    std::cout << "loss%: " << final.losses * 100.0 << " (";
    for(auto val: results.first)
    {
        std::cout << val.losses << " ";
    }
    std::cout << "/ " << results.second << ")" << std::endl;

    switch(optimization_mode)
    {
        case OptimizationMode::raid:
            std::cout << "ard: " << final.points << " (";
            for(auto val: results.first)
            {
                std::cout << val.points << " ";
            }
            std::cout << "/ " << results.second << ")" << std::endl;
            if (show_stdev)
            {
                std::cout << "stdev: " << sqrt(final.sq_points - final.points * final.points) << std::endl;
            }
            break;
        case OptimizationMode::achievement:
            std::cout << "achievement%: " << final.points << " (";
            for(auto val: results.first)
            {
                std::cout << val.points / 100 << " ";
            }
            std::cout << "/ " << results.second << ")" << std::endl;
            break;
        default:
            break;
    }
}
//------------------------------------------------------------------------------
void print_deck_inline(const unsigned deck_cost, const Results<long double> score, const Card *commander, std::vector<const Card*> cards, bool is_ordered)
{
    if(fund > 0)
    {
        std::cout << "$" << deck_cost << " ";
    }
    switch(optimization_mode)
    {
        case OptimizationMode::raid:
            std::cout << "(" << (score.wins + score.draws) * 100 << "% win, " << score.wins * 100.0 << "% slay";
            if (show_stdev) {
                std::cout << ", " << sqrt(score.sq_points - score.points * score.points) << " stdev";
            }
            std::cout << ") ";
            break;
        case OptimizationMode::achievement:
            std::cout << "(" << score.wins * 100.0 << "% win) ";
            break;
        case OptimizationMode::defense:
            std::cout << "(" << score.draws * 100.0 << "% stall) ";
            break;
        default:
            break;
    }
    std::cout << score.points << ": " << commander->m_name;
    if(!is_ordered)
    {
        std::sort(cards.begin(), cards.end(), [](const Card* a, const Card* b) { return a->m_id < b->m_id; });
    }
    std::string last_name;
    unsigned num_repeat(0);
    for(const Card* card: cards)
    {
        if(card->m_name == last_name)
        {
            ++ num_repeat;
        }
        else
        {
            if(num_repeat > 1)
            {
                std::cout << " #" << num_repeat;
            }
            std::cout << ", " << card->m_name;
            last_name = card->m_name;
            num_repeat = 1;
        }
    }
    if(num_repeat > 1)
    {
        std::cout << " #" << num_repeat;
    }
    std::cout << std::endl;
}
//------------------------------------------------------------------------------
void hill_climbing(unsigned num_iterations, Deck* d1, Process& proc, std::map<signed, char> card_marks)
{
    auto results = proc.evaluate(num_iterations);
    print_score_info(results, proc.factors);
    auto current_score = compute_score(results, proc.factors);
    auto best_score = current_score;
    std::map<std::multiset<unsigned>, unsigned> evaluated_decks{{d1->card_ids<std::multiset<unsigned>>(),  num_iterations}};
    // Non-commander cards
    auto non_commander_cards = proc.cards.player_assaults;
    non_commander_cards.insert(non_commander_cards.end(), proc.cards.player_structures.begin(), proc.cards.player_structures.end());
    non_commander_cards.insert(non_commander_cards.end(), proc.cards.player_actions.begin(), proc.cards.player_actions.end());
    non_commander_cards.insert(non_commander_cards.end(), std::initializer_list<Card *>{NULL,});
    const Card* best_commander = d1->commander;
    std::vector<const Card*> best_cards = d1->cards;
    unsigned deck_cost = get_deck_cost(d1, proc.cards);
    fund = std::max(fund, deck_cost);
    print_deck_inline(deck_cost, best_score, best_commander, best_cards, false);
    std::mt19937 re(time(NULL));
    bool deck_has_been_improved = true;
    unsigned long skipped_simulations = 0;
    for(unsigned slot_i(0), dead_slot(0); (deck_has_been_improved || slot_i != dead_slot) && best_score.points - target_score < -1e-9; slot_i = (slot_i + 1) % std::min<unsigned>(max_deck_len, best_cards.size() + 1))
    {
        if(card_marks.count(slot_i)) { continue; }
        if(deck_has_been_improved)
        {
            dead_slot = slot_i;
            deck_has_been_improved = false;
        }
        if(!card_marks.count(-1))
        {
            /* Evaluate all commanders for this deck, first. */
            for(const Card* commander_candidate: proc.cards.player_commanders) {
                // Various checks to check if the card is accepted
                assert(commander_candidate->m_type == CardType::commander);
                if(commander_candidate->m_name == best_commander->m_name) { continue; }
                // Place it in the deck
                d1->commander = commander_candidate;
                auto &&cur_deck = d1->card_ids<std::multiset<unsigned>>();
                if(evaluated_decks.count(cur_deck) == 0) {
                    deck_cost = get_deck_cost(d1, proc.cards);

                    if(deck_cost > fund) { continue; }

                    // Evaluate new deck
                    auto compare_results = proc.compare(num_iterations, best_score.points);
                    current_score = compute_score(compare_results, proc.factors);
                    evaluated_decks[cur_deck] = compare_results.second;

                    // Is it better ?
                    if(current_score.points > best_score.points) {
                        // Then update best score/commander, print stuff
                        best_score = current_score;
                        best_commander = commander_candidate;
                        deck_has_been_improved = true;
                        std::cout << "Deck improved: " << deck_hash(commander_candidate, best_cards, false) << " commander -> " << card_id_name(commander_candidate) << ": ";
                        print_score_info(compare_results, proc.factors);
                        print_deck_inline(deck_cost, best_score, best_commander, best_cards, false);
                    }
                } else {
                    skipped_simulations += evaluated_decks[cur_deck];
                }
            }
            // Now that all commanders are evaluated, take the best one
            d1->commander = best_commander;
        }
        std::shuffle(non_commander_cards.begin(), non_commander_cards.end(), re);
        for(const Card* card_candidate: non_commander_cards)
        {
            d1->cards = best_cards;
            if(card_candidate)
            {
                // Various checks to check if the card is accepted
                assert(card_candidate->m_type != CardType::commander);
                if(slot_i < best_cards.size() && card_candidate->m_name == best_cards[slot_i]->m_name) { continue; }
                if(!suitable_non_commander(*d1, slot_i, card_candidate)) { continue; }
                // Place it in the deck
                if(slot_i == d1->cards.size())
                {
                    d1->cards.emplace_back(card_candidate);
                }
                else
                {
                    d1->cards[slot_i] = card_candidate;
                }
            }
            else
            {
                if(best_cards.size() <= min_deck_len || slot_i == best_cards.size()) { continue; }
                // Remove it from the deck
                d1->cards.erase(d1->cards.begin() + slot_i);
            }
            auto &&cur_deck = d1->card_ids<std::multiset<unsigned>>();
            if(evaluated_decks.count(cur_deck) == 0)
            {
                deck_cost = get_deck_cost(d1, proc.cards);
                if(deck_cost > fund) { continue; }
                // Evaluate new deck
                auto compare_results = proc.compare(num_iterations, best_score.points);
                current_score = compute_score(compare_results, proc.factors);
                evaluated_decks[cur_deck] = compare_results.second;
                // Is it better ?
                if(current_score.points > best_score.points)
                {
                    std::cout << "Deck improved: " << deck_hash(best_commander, d1->cards, false) << " " << card_id_name(slot_i < best_cards.size() ? best_cards[slot_i] : NULL) <<
                        " -> " << card_id_name(card_candidate) << ": ";
                    // Then update best score/slot, print stuff
                    best_score = current_score;
                    best_cards = d1->cards;
                    deck_has_been_improved = true;
                    print_score_info(compare_results, proc.factors);
                    print_deck_inline(deck_cost, best_score, best_commander, best_cards, false);
                }
            }
            else
            {
                skipped_simulations += evaluated_decks[cur_deck];
            }
            if(best_score.points - target_score > -1e-9) { break; }
        }
        d1->cards = best_cards;
    }
    unsigned simulations = 0;
    for(auto evaluation: evaluated_decks)
    { simulations += evaluation.second; }
    std::cout << "Evaluated " << evaluated_decks.size() << " decks (" << simulations << " + " << skipped_simulations << " simulations)." << std::endl;
    std::cout << "Optimized Deck: ";
    print_deck_inline(get_deck_cost(d1, proc.cards), best_score, best_commander, best_cards, false);
}
//------------------------------------------------------------------------------
void hill_climbing_ordered(unsigned num_iterations, Deck* d1, Process& proc, std::map<signed, char> card_marks)
{
    auto results = proc.evaluate(num_iterations);
    print_score_info(results, proc.factors);
    auto current_score = compute_score(results, proc.factors);
    auto best_score = current_score;
    std::map<std::vector<unsigned>, unsigned> evaluated_decks{{d1->card_ids<std::vector<unsigned>>(), num_iterations}};
    // Non-commander cards
    auto non_commander_cards = proc.cards.player_assaults;
    non_commander_cards.insert(non_commander_cards.end(), proc.cards.player_structures.begin(), proc.cards.player_structures.end());
    non_commander_cards.insert(non_commander_cards.end(), proc.cards.player_actions.begin(), proc.cards.player_actions.end());
    non_commander_cards.insert(non_commander_cards.end(), std::initializer_list<Card *>{NULL,});
    const Card* best_commander = d1->commander;
    std::vector<const Card*> best_cards = d1->cards;
    unsigned deck_cost = get_deck_cost(d1, proc.cards);
    fund = std::max(fund, deck_cost);
    print_deck_inline(deck_cost, best_score, best_commander, best_cards, true);
    std::mt19937 re(time(NULL));
    bool deck_has_been_improved = true;
    unsigned long skipped_simulations = 0;
    for(unsigned from_slot(0), dead_slot(0); (deck_has_been_improved || from_slot != dead_slot) && best_score.points - target_score < -1e-9; from_slot = (from_slot + 1) % std::min<unsigned>(max_deck_len, d1->cards.size() + 1))
    {
        if(deck_has_been_improved)
        {
            dead_slot = from_slot;
            deck_has_been_improved = false;
        }
        if(!card_marks.count(-1))
        {
            for(const Card* commander_candidate: proc.cards.player_commanders)
            {
                if(best_score.points - target_score > -1e-9) { break; }
                // Various checks to check if the card is accepted
                assert(commander_candidate->m_type == CardType::commander);
                if(commander_candidate->m_name == best_commander->m_name) { continue; }
                // Place it in the deck
                d1->commander = commander_candidate;
                auto &&cur_deck = d1->card_ids<std::vector<unsigned>>();
                if(evaluated_decks.count(cur_deck) == 0)
                {
                    deck_cost = get_deck_cost(d1, proc.cards);
                    if(deck_cost > fund) { continue; }
                    // Evaluate new deck
                    auto compare_results = proc.compare(num_iterations, best_score.points);
                    current_score = compute_score(compare_results, proc.factors);
                    evaluated_decks[cur_deck] = compare_results.second;
                    // Is it better ?
                    if(current_score.points > best_score.points)
                    {
                        // Then update best score/commander, print stuff
                        best_score = current_score;
                        best_commander = commander_candidate;
                        deck_has_been_improved = true;
                        std::cout << "Deck improved: " << deck_hash(commander_candidate, best_cards, true) << " commander -> " << card_id_name(commander_candidate) << ": ";
                        print_score_info(compare_results, proc.factors);
                        print_deck_inline(deck_cost, best_score, best_commander, best_cards, true);
                    }
                }
                else
                {
                    skipped_simulations += evaluated_decks[cur_deck];
                }
            }
            // Now that all commanders are evaluated, take the best one
            d1->commander = best_commander;
        }
        std::shuffle(non_commander_cards.begin(), non_commander_cards.end(), re);
        for(const Card* card_candidate: non_commander_cards)
        {
            // Various checks to check if the card is accepted
            assert(!card_candidate || card_candidate->m_type != CardType::commander);
            for(unsigned to_slot(card_candidate ? 0 : best_cards.size() - 1); to_slot < best_cards.size() + (from_slot < best_cards.size() ? 0 : 1); ++to_slot)
            {
                if(card_marks.count(from_slot) && card_candidate != best_cards[from_slot]) { break; }
                d1->cards = best_cards;
                if(card_candidate)
                {
                    // Various checks to check if the card is accepted
                    if(!suitable_non_commander(*d1, from_slot, card_candidate)) { continue; }
                    // Place it in the deck
                    if(from_slot < best_cards.size())
                    {
                        if(from_slot == to_slot && card_candidate->m_name == best_cards[to_slot]->m_name) { continue; }
                        d1->cards.erase(d1->cards.begin() + from_slot);
                    }
                    d1->cards.insert(d1->cards.begin() + to_slot, card_candidate);
                }
                else
                {
                    if(best_cards.size() <= min_deck_len || from_slot == best_cards.size()) { continue; }
                    // Remove it from the deck
                    d1->cards.erase(d1->cards.begin() + from_slot);
                }
                auto &&cur_deck = d1->card_ids<std::vector<unsigned>>();
                if(evaluated_decks.count(cur_deck) == 0)
                {
                    deck_cost = get_deck_cost(d1, proc.cards);
                    if(deck_cost > fund) { continue; }
                    // Evaluate new deck
                    auto compare_results = proc.compare(num_iterations, best_score.points);
                    current_score = compute_score(compare_results, proc.factors);
                    evaluated_decks[cur_deck] = compare_results.second;
                    // Is it better ?
                    if(current_score.points > best_score.points)
                    {
                        // Then update best score/slot, print stuff
                        std::cout << "Deck improved: " << deck_hash(best_commander, d1->cards, true) << " " << from_slot << " " << card_id_name(from_slot < best_cards.size() ? best_cards[from_slot] : NULL) <<
                            " -> " << to_slot << " " << card_id_name(card_candidate) << ": ";
                        best_score = current_score;
                        best_cards = d1->cards;
                        deck_has_been_improved = true;
                        print_score_info(compare_results, proc.factors);
                        print_deck_inline(deck_cost, best_score, best_commander, best_cards, true);
                        std::map<signed, char> new_card_marks;
                        for(auto it: card_marks)
                        {
                            signed pos = it.first;
                            char mark = it.second;
                            if(pos < 0) {}
                            else if(static_cast<unsigned>(pos) == from_slot)
                            {
                                pos = to_slot;
                            }
                            else
                            {
                                if(static_cast<unsigned>(pos) > from_slot) { -- pos; }
                                if(static_cast<unsigned>(pos) >= to_slot) { ++ pos; }
                            }
                            new_card_marks[pos] = mark;
                        }
                        card_marks = new_card_marks;
                    }
                }
                else
                {
                    skipped_simulations += evaluated_decks[cur_deck];
                }
            }
            if(best_score.points - target_score > -1e-9) { break; }
        }
        d1->cards = best_cards;
    }
    unsigned simulations = 0;
    for(auto evaluation: evaluated_decks)
    { simulations += evaluation.second; }
    std::cout << "Evaluated " << evaluated_decks.size() << " decks (" << simulations << " + " << skipped_simulations << " simulations)." << std::endl;
    std::cout << "Optimized Deck: ";
    print_deck_inline(get_deck_cost(d1, proc.cards), best_score, best_commander, best_cards, true);
}
//------------------------------------------------------------------------------
// Implements iteration over all combination of k elements from n elements.
// parameter firstIndexLimit: this is a ugly hack used to implement the special condition that
// a deck could be expected to contain at least 1 assault card. Thus the first element
// will be chosen among the assault cards only, instead of all cards.
// It works on the condition that the assault cards are sorted first in the list of cards,
// thus have indices 0..firstIndexLimit-1.
class Combination
{
public:
    Combination(unsigned all_, unsigned choose_, unsigned firstIndexLimit_ = 0) :
        all(all_),
        choose(choose_),
        firstIndexLimit(firstIndexLimit_ == 0 ? all_ - choose_ : firstIndexLimit_),
        indices(choose_, 0),
        indicesLimits(choose_, 0)
    {
        assert(choose > 0);
        assert(choose <= all);
        assert(firstIndexLimit <= all);
        indicesLimits[0] = firstIndexLimit;
        for(unsigned i(1); i < choose; ++i)
        {
            indices[i] = i;
            indicesLimits[i] =  all - choose + i;
        }
    }

    const std::vector<unsigned>& getIndices()
    {
        return(indices);
    }

    bool next()
    {
        // The end condition's a bit odd here, but index is unsigned.
        // The last iteration is when index = 0.
        // After that, index = max int, which is clearly >= choose.
        for(index = choose - 1; index < choose; --index)
        {
            if(indices[index] < indicesLimits[index])
            {
                ++indices[index];
                for(nextIndex = index + 1; nextIndex < choose; nextIndex++)
                {
                    indices[nextIndex] = indices[index] - index + nextIndex;
                }
                return(false);
            }
        }
        return(true);
    }

private:
    unsigned all;
    unsigned choose;
    unsigned firstIndexLimit;
    std::vector<unsigned> indices;
    std::vector<unsigned> indicesLimits;
    unsigned index;
    unsigned nextIndex;
};
//------------------------------------------------------------------------------
static unsigned total_num_combinations_test(0);
inline void try_all_ratio_combinations(unsigned deck_size, unsigned var_k, unsigned num_iterations, const std::vector<unsigned>& card_indices, std::vector<const Card*>& cards, const Card* commander, Process& proc, Results<long double>& best_score, boost::optional<Deck>& best_deck)
{
    assert(card_indices.size() > 0);
    assert(card_indices.size() <= deck_size);
    unsigned num_cards_to_combine(deck_size);
    std::vector<const Card*> unique_cards;
    std::vector<const Card*> cards_to_combine;
    bool legendary_found(false);
    for(unsigned card_index: card_indices)
    {
        const Card* card(cards[card_index]);
        if(card->m_unique || card->m_rarity == 4)
        {
            if(card->m_rarity == 4)
            {
                if(legendary_found) { return; }
                legendary_found = true;
            }
            --num_cards_to_combine;
            unique_cards.push_back(card);
        }
        else
        {
            cards_to_combine.push_back(card);
        }
    }
    // all unique or legendaries, quit
    if(cards_to_combine.size() == 0) { return; }
    if(cards_to_combine.size() == 1)
    {
        std::vector<const Card*> deck_cards = unique_cards;
        std::vector<const Card*> combined_cards(num_cards_to_combine, cards_to_combine[0]);
        deck_cards.insert(deck_cards.end(), combined_cards.begin(), combined_cards.end());
        Deck deck{};
        deck.set(commander, deck_cards);
        (*dynamic_cast<Deck*>(proc.att_deck)) = deck;
        auto new_results = proc.compare(num_iterations, best_score.points);
        auto new_score = compute_score(new_results, proc.factors);
        if(new_score.points > best_score.points)
        {
            best_score = new_score;
            best_deck = deck;
            print_score_info(new_results, proc.factors);
            print_deck_inline(0, best_score, commander, deck_cards, false);
        }
        //++num;
        // num_cards = num_cards_to_combine ...
    }
    else
    {
        var_k = cards_to_combine.size() - 1;
        Combination cardAmounts(num_cards_to_combine-1, var_k);
        bool finished(false);
        while(!finished)
        {
            const std::vector<unsigned>& indices = cardAmounts.getIndices();
            std::vector<unsigned> num_cards(var_k+1, 0);
            num_cards[0] = indices[0] + 1;
            for(unsigned i(1); i < var_k; ++i)
            {
                num_cards[i] = indices[i] - indices[i-1];
            }
            num_cards[var_k] = num_cards_to_combine - (indices[var_k-1] + 1);
            std::vector<const Card*> deck_cards = unique_cards;
            //std::cout << "num cards: ";
            for(unsigned num_index(0); num_index < num_cards.size(); ++num_index)
            {
                //std::cout << num_cards[num_index] << " ";
                for(unsigned i(0); i < num_cards[num_index]; ++i) { deck_cards.push_back(cards[card_indices[num_index]]); }
            }
            //std::cout << "\n" << std::flush;
            //std::cout << std::flush;
            assert(deck_cards.size() == deck_size);
            Deck deck{};
            deck.set(commander, deck_cards);
            *proc.att_deck = deck;
            auto new_results = proc.compare(num_iterations, best_score.points);
            auto new_score = compute_score(new_results, proc.factors);
            if(new_score.points > best_score.points)
            {
                best_score = new_score;
                best_deck = deck;
                print_score_info(new_results, proc.factors);
                print_deck_inline(0, best_score, commander, deck_cards, false);
            }
            ++total_num_combinations_test;
            finished = cardAmounts.next();
        }
    }
}
//------------------------------------------------------------------------------
enum Operation {
    simulate,
    climb,
    reorder,
    debug,
    debuguntil
};
//------------------------------------------------------------------------------
void print_available_decks(const Decks& decks, bool allow_card_pool)
{
    std::cout << "Available decks: (use double-quoted name)" << std::endl;
    std::cout << "(All missions, omitted because the list is too long.)" << std::endl;
    for(auto& deck: decks.decks)
    {
        if(deck.decktype != DeckType::mission && (allow_card_pool || deck.raid_cards.empty()))
        {
            std::cout << deck.short_description() << "\n";
        }
    }
}
//------------------------------------------------------------------------------
void print_available_effects()
{
    std::cout << "Available effects:" << std::endl;
    for(int i(1); i < Effect::num_effects; ++ i)
    {
        std::cout << i << " \"" << effect_names[i] << "\"" << std::endl;
    }
}

void usage(int argc, char** argv)
{
    std::cout << "usage: " << argv[0] << " Your_Deck Enemy_Deck [Flags] [Operations]\n"
        "\n"
        "Your_Deck:\n"
        "  the name/hash/cards of a custom deck.\n"
        "\n"
        "Enemy_Deck:\n"
        "  semicolon separated list of defense decks, syntax:\n"
        "  deck1[:factor1];deck2[:factor2];...\n"
        "  where deck is the name/hash/cards of a mission, raid, quest or custom deck, and factor is optional. The default factor is 1.\n"
        "  example: \'fear:0.2;slowroll:0.8\' means fear is the defense deck 20% of the time, while slowroll is the defense deck 80% of the time.\n"
        "\n"
        "Flags:\n"
        "  -A <achievement>: optimize for the achievement specified by either id or name.\n"
        "  -e <effect>: set the battleground effect. effect is automatically set when applicable.\n"
        "  -r: the attack deck is played in order instead of randomly (respects the 3 cards drawn limit).\n"
        "  -s: use surge (default is fight).\n"
        "  -t <num>: set the number of threads, default is 4.\n"
        "  -turnlimit <num>: set the number of turns in a battle, default is 50 (can be used for speedy achievements).\n"
        "  win:     simulate/optimize for win rate. default for non-raids.\n"
        "  defense: simulate/optimize for win rate + stall rate. can be used for defending deck or win rate oriented raid simulations.\n"
        "  raid:    simulate/optimize for average raid damage (ARD). default for raids.\n"
        "Flags for climb:\n"
        "  -c: don't try to optimize the commander.\n"
        "  -L <min> <max>: restrict deck size between <min> and <max>.\n"
        "  -o: restrict to the owned cards listed in \"ownedcards.txt\".\n"
        "  -o=<filename>: restrict to the owned cards listed in <filename>.\n"
        "  fund <num>: fund <num> gold to buy/upgrade cards. prices are specified in ownedcards file.\n"
        "  target <num>: stop as soon as the score reaches <num>.\n"
        "  -u: don't upgrade owned cards. (by default, upgrade owned cards when needed)\n"
        "\n"
        "Operations:\n"
        "  sim <num>: simulate <num> battles to evaluate a deck.\n"
        "  climb <num>: perform hill-climbing starting from the given attack deck, using up to <num> battles to evaluate a deck.\n"
        "  reorder <num>: optimize the order for given attack deck, using up to <num> battles to evaluate an order.\n"
#ifndef NDEBUG
        "  debug: testing purpose only. very verbose output. only one battle.\n"
        "  debuguntil <min> <max>: testing purpose only. fight until the last fight results in range [<min>, <max>]. recommend to redirect output.\n"
#endif
        ;
}

int main(int argc, char** argv)
{
    if(argc == 1) { usage(argc, argv); return(0); }
    if(argc <= 2 && strcmp(argv[1], "-version") == 0)
    {
        std::cout << "Tyrant Optimizer " << TYRANT_OPTIMIZER_VERSION << std::endl;
        return(0);
    }
    unsigned num_threads = 4;
    DeckStrategy::DeckStrategy att_strategy(DeckStrategy::random);
    DeckStrategy::DeckStrategy def_strategy(DeckStrategy::random);
    Cards cards;
    read_cards(cards);
    read_card_abbrs(cards, "cardabbrs.txt");
    Decks decks;
    Achievement achievement;
    load_decks_xml(decks, cards);
    load_decks(decks, cards);
    fill_skill_table();

    if(argc <= 2)
    {
        print_available_decks(decks, true);
        return(4);
    }
    std::string att_deck_name{argv[1]};
    auto deck_list_parsed = parse_deck_list(argv[2]);

    Deck* att_deck{nullptr};
    std::vector<Deck*> def_decks;
    std::vector<long double> def_decks_factors;
    enum Effect effect(Effect::none);
    bool keep_commander{false};
    bool fixed_len{false};
    std::vector<std::tuple<unsigned, unsigned, Operation>> todo;

    try
    {
        att_deck = find_deck(decks, cards, att_deck_name);
    }
    catch(const std::runtime_error& e)
    {
        std::cerr << "Error: Deck " << att_deck_name << ": " << e.what() << std::endl;
        return(5);
    }
    if(att_deck == nullptr)
    {
        std::cerr << "Error: Invalid attack deck name/hash " << att_deck_name << ".\n";
    }
    else if(!att_deck->raid_cards.empty())
    {
        std::cerr << "Error: Invalid attack deck " << att_deck_name << ": has optional cards.\n";
        att_deck = nullptr;
    }
    if(att_deck == nullptr)
    {
        print_available_decks(decks, false);
        return(5);
    }

    for(auto deck_parsed: deck_list_parsed)
    {
        Deck* def_deck{nullptr};
        try
        {
            def_deck = find_deck(decks, cards, deck_parsed.first);
        }
        catch(const std::runtime_error& e)
        {
            std::cerr << "Error: Deck " << deck_parsed.first << ": " << e.what() << std::endl;
            return(5);
        }
        if(def_deck == nullptr)
        {
            std::cerr << "Error: Invalid defense deck name/hash " << deck_parsed.first << ".\n";
            print_available_decks(decks, true);
            return(5);
        }
        if(def_deck->decktype == DeckType::raid)
        {
            optimization_mode = OptimizationMode::raid;
            turn_limit = 30;
            target_score = 250;
        }
        def_decks.push_back(def_deck);
        def_decks_factors.push_back(deck_parsed.second);
    }

    std::map<std::string, int> effect_map;
    for(unsigned i(0); i < Effect::num_effects; ++i)
    {
        effect_map[effect_names[i]] = i;
        std::stringstream ss;
        ss << i;
        effect_map[ss.str()] = i;
    }

    for(int argIndex(3); argIndex < argc; ++argIndex)
    {
        if(strcmp(argv[argIndex], "win") == 0)
        {
            optimization_mode = OptimizationMode::winrate;
        }
        else if(strcmp(argv[argIndex], "raid") == 0)
        {
            optimization_mode = OptimizationMode::raid;
            turn_limit = 30;
            target_score = 250;
        }
        else if(strcmp(argv[argIndex], "defense") == 0)
        {
            optimization_mode = OptimizationMode::defense;
        }
        else if(strcmp(argv[argIndex], "-A") == 0)
        {
            try
            {
                read_achievement(decks, cards, achievement, argv[argIndex + 1]);
                optimization_mode = OptimizationMode::achievement;
            }
            catch(const std::runtime_error& e)
            {
                std::cerr << "Error: Achievement " << argv[argIndex + 1] << ": " << e.what() << std::endl;
                return(1);
            }
            for(auto def_deck: def_decks)
            {
                if(def_deck->decktype != DeckType::mission)
                {
                    std::cerr << "Error: Enemy's deck must be mission for achievement." << std::endl;
                    return(1);
                }
                if(!achievement.mission_condition.check(def_deck->id))
                {
                    std::cerr << "Error: Wrong mission [" << def_deck->name << "] for achievement." << std::endl;
                    return(1);
                }
            }
            argIndex += 1;
        }
        else if(strcmp(argv[argIndex], "-c") == 0)
        {
            keep_commander = true;
        }
        else if(strcmp(argv[argIndex], "-e") == 0)
        {
            std::string arg_effect(argv[argIndex + 1]);
            auto x = effect_map.find(arg_effect);
            if(x == effect_map.end())
            {
                std::cout << "The effect '" << arg_effect << "' was not found. ";
                print_available_effects();
                return(6);
            }
            effect = static_cast<enum Effect>(x->second);
            argIndex += 1;
        }
        else if(strcmp(argv[argIndex], "-fixedlen") == 0)
        {
            fixed_len = true;
        }
        else if(strcmp(argv[argIndex], "-L") == 0)
        {
            min_deck_len = atoi(argv[argIndex + 1]);
            max_deck_len = atoi(argv[argIndex + 2]);
            argIndex += 2;
        }
        else if(strcmp(argv[argIndex], "-o") == 0)
        {
            read_owned_cards(cards, owned_cards, buyable_cards, "ownedcards.txt");
            use_owned_cards = true;
        }
        else if(strncmp(argv[argIndex], "-o=", 3) == 0)
        {
            read_owned_cards(cards, owned_cards, buyable_cards, argv[argIndex] + 3);
            use_owned_cards = true;
        }
        else if(strncmp(argv[argIndex], "-o+", 3) == 0)
        {
            unsigned completed_mission_id = atoi(argv[argIndex] + 3);
            // claim reward cards as owned; use filter file if you want to exclude
            for(auto def_deck: def_decks)
            {
                unsigned prev_mission_id = def_deck->mission_req;
                while(prev_mission_id > completed_mission_id)
                {
                    auto prev_deck = decks.by_type_id[{DeckType::mission, prev_mission_id}];
                    prev_mission_id = prev_deck->mission_req;
                    claim_cards(prev_deck->reward_cards, cards, true, true);
                }
            }
        }
        else if(strcmp(argv[argIndex], "fund") == 0)
        {
            fund = atoi(argv[argIndex+1]);
            argIndex += 1;
        }
        else if(strcmp(argv[argIndex], "-r") == 0 || strcmp(argv[argIndex], "ordered") == 0)
        {
            att_strategy = DeckStrategy::ordered;
        }
        else if(strcmp(argv[argIndex], "exact-ordered") == 0)
        {
            att_strategy = DeckStrategy::exact_ordered;
        }
        else if(strcmp(argv[argIndex], "defender:ordered") == 0)
        {
            def_strategy = DeckStrategy::ordered;
        }
        else if(strcmp(argv[argIndex], "defender:exact-ordered") == 0)
        {
            def_strategy = DeckStrategy::exact_ordered;
        }
        else if(strcmp(argv[argIndex], "-s") == 0 || strcmp(argv[argIndex], "surge") == 0)
        {
            gamemode = surge;
        }
        else if(strcmp(argv[argIndex], "tournament") == 0)
        {
            gamemode = tournament;
        }
        else if(strcmp(argv[argIndex], "-t") == 0)
        {
            num_threads = atoi(argv[argIndex+1]);
            argIndex += 1;
        }
        else if(strcmp(argv[argIndex], "target") == 0)
        {
            target_score = atof(argv[argIndex+1]);
            argIndex += 1;
        }
        else if(strcmp(argv[argIndex], "-turnlimit") == 0)
        {
            turn_limit = atoi(argv[argIndex+1]);
            argIndex += 1;
        }
        else if(strcmp(argv[argIndex], "-u") == 0)
        {
            auto_upgrade_cards = false;
        }
        else if(strcmp(argv[argIndex], "+stdev") == 0)
        {
            show_stdev = true;
        }
        else if(strcmp(argv[argIndex], "+hm") == 0)
        {
            use_harmonic_mean = true;
        }
        else if(strcmp(argv[argIndex], "+v") == 0)
        {
            ++ debug_print;
        }
        else if(strcmp(argv[argIndex], "hand") == 0)  // set initial hand for test
        {
            att_deck->set_given_hand(cards, argv[argIndex + 1]);
            argIndex += 1;
        }
        else if(strcmp(argv[argIndex], "defender:hand") == 0)  // set enemies' initial hand for test
        {
            for(auto def_deck: def_decks)
            {
                def_deck->set_given_hand(cards, argv[argIndex + 1]);
            }
            argIndex += 1;
        }
        else if(strcmp(argv[argIndex], "sim") == 0)
        {
            todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), 0u, simulate));
            argIndex += 1;
        }
        else if(strcmp(argv[argIndex], "climb") == 0)
        {
            todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), 0u, climb));
            argIndex += 1;
        }
        else if(strcmp(argv[argIndex], "reorder") == 0)
        {
            todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), 0u, reorder));
            argIndex += 1;
        }
        else if(strcmp(argv[argIndex], "debug") == 0)
        {
            todo.push_back(std::make_tuple(0u, 0u, debug));
        }
        else if(strcmp(argv[argIndex], "debuguntil") == 0)
        {
            // output the debug info for the first battle that min_score <= score <= max_score.
            // E.g., 0 0: lose; 100 100: win (non-raid); 150 250: at least 150 damage (raid).
            todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 2]), debuguntil));
            argIndex += 2;
        }
        else
        {
            std::cerr << "Error: Unknown option " << argv[argIndex] << std::endl;
            return(1);
        }
    }

    // Force to claim non-buyable cards in your initial deck.
    if(use_owned_cards)
    {
        claim_cards({att_deck->commander}, cards, fund == 0, false);
        claim_cards(att_deck->cards, cards, fund == 0, false);
    }

    att_deck->strategy = att_strategy;
    for(auto def_deck: def_decks)
    {
        def_deck->strategy = def_strategy;
    }

    if(keep_commander)
    {
        att_deck->card_marks[-1] = '!';
    }
    if(fixed_len)
    {
        min_deck_len = max_deck_len = att_deck->cards.size();
    }

    std::cout << "Your Deck: " << (debug_print ? att_deck->long_description(cards) : att_deck->short_description()) << std::endl;
    for(auto def_deck: def_decks)
    {
        std::cout << "Enemy's Deck: " << (debug_print ? def_deck->long_description(cards) : def_deck->short_description()) << std::endl;
    }
    if(effect != Effect::none)
    {
        std::cout << "Effect: " << effect_names[effect] << std::endl;
    }

    Process p(num_threads, cards, decks, att_deck, def_decks, def_decks_factors, gamemode, effect, achievement);

    {
        //ScopeClock timer;
        for(auto op: todo)
        {
            switch(std::get<2>(op))
            {
            case simulate: {
                auto results = p.evaluate(std::get<0>(op));
                print_results(results, p.factors);
                break;
            }
            case climb: {
                if(att_strategy == DeckStrategy::random)
                {
                    hill_climbing(std::get<0>(op), att_deck, p, att_deck->card_marks);
                }
                else
                {
                    hill_climbing_ordered(std::get<0>(op), att_deck, p, att_deck->card_marks);
                }
                break;
            }
            case reorder: {
                att_deck->strategy = DeckStrategy::ordered;
                min_deck_len = max_deck_len = att_deck->cards.size();
                use_owned_cards = true;
                auto_upgrade_cards = false;
                owned_cards.clear();
                claim_cards({att_deck->commander}, cards, true, false);
                claim_cards(att_deck->cards, cards, true, false);
                hill_climbing_ordered(std::get<0>(op), att_deck, p, att_deck->card_marks);
                break;
            }
            case debug: {
                unsigned saved_num_threads = num_threads;
                num_threads = 1;
                ++ debug_print;
                debug_str.clear();
                auto results = p.evaluate(1);
                print_results(results, p.factors);
                -- debug_print;
                num_threads = saved_num_threads;
                break;
            }
            case debuguntil: {
                unsigned saved_num_threads = num_threads;
                num_threads = 1;
                ++ debug_print;
                ++ debug_cached;
                while(1)
                {
                    debug_str.clear();
                    auto results = p.evaluate(1);
                    auto score = compute_score(results, p.factors);
                    if(score.points >= std::get<0>(op) && score.points <= std::get<1>(op)) {
                        std::cout << debug_str << std::flush;
                        print_results(results, p.factors);
                        break;
                    }
                }
                -- debug_cached;
                -- debug_print;
                num_threads = saved_num_threads;
                break;
            }
            }
        }
    }
    return(0);
}
