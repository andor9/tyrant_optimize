#include "deck.h"
#include <boost/thread/barrier.hpp>
#include <boost/thread/mutex.hpp>
#include <iostream>
#include <boost/thread/thread.hpp>
#include <chrono>
#include <array>
#include <vector>
#include "hPMML.h"

// OpenMP Header
#ifdef _OPENMP
#include <omp.h>
#endif
// Android Headers
#if defined(ANDROID) || defined(__ANDROID__)
#include <jni.h>
#include <android/log.h>
#endif
//------------------------------------------------------------------------------

struct Requirement
{
	std::unordered_map<const Card*, unsigned> num_cards;
};
#ifdef DEFINE_GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

namespace tuo {
	EXTERN Cards all_cards;
	EXTERN unsigned opt_num_threads;
	EXTERN gamemode_t gamemode;
	EXTERN OptimizationMode optimization_mode;
	EXTERN std::unordered_map<unsigned, unsigned> owned_cards;
	EXTERN const Card* owned_alpha_dominion;
	EXTERN bool use_owned_cards;
	EXTERN bool opt_skip_unclaimed_decks;
	//bool opt_trim_unclaimed_decks{false};
	EXTERN unsigned min_deck_len;
	EXTERN unsigned max_deck_len;
	EXTERN unsigned opt_freezed_cards;
	EXTERN unsigned freezed_cards;
	EXTERN unsigned fund;
	EXTERN long double target_score;
	EXTERN long double min_increment_of_score;
	EXTERN long double confidence_level;
	EXTERN bool use_top_level_card;
	EXTERN bool use_top_level_commander;
	EXTERN bool mode_open_the_deck;
	EXTERN bool use_owned_dominions;
	EXTERN bool use_maxed_dominions;
	EXTERN unsigned use_fused_card_level;
	EXTERN unsigned use_fused_commander_level;
	EXTERN bool print_upgraded;
	EXTERN bool print_values;
	EXTERN int vc_x;
	EXTERN bool simplify_output;
	EXTERN bool show_ci;
	EXTERN bool use_harmonic_mean;
	EXTERN unsigned iterations_multiplier;
	EXTERN unsigned sim_seed;
	EXTERN unsigned flexible_iter;
	EXTERN unsigned flexible_turn;
	EXTERN unsigned eval_iter;
	EXTERN unsigned eval_turn;
	EXTERN Requirement requirement;
	EXTERN double hp_scale;
	EXTERN double atk_scale;
	EXTERN std::map<Skill::Skill,double> x_skill_scale;
	EXTERN std::map<Skill::Skill,double> n_skill_scale;
	EXTERN std::map<Skill::Skill,double> c_skill_scale;
#ifndef NQUEST
	EXTERN Quest quest;
#endif
	EXTERN std::unordered_set<unsigned> allowed_candidates;
	EXTERN std::unordered_set<unsigned> disallowed_candidates;
	EXTERN std::unordered_set<unsigned> disallowed_recipes;
	EXTERN std::chrono::time_point<std::chrono::system_clock> start_time;
	EXTERN long double maximum_time;
	//anneal
	EXTERN long double temperature;
	EXTERN long double coolingRate;
	//genetic
	EXTERN unsigned generations;
	EXTERN unsigned pool_size;
	EXTERN unsigned min_pool_size;
	EXTERN double opt_pool_keep;
	EXTERN double opt_pool_mutate;
	EXTERN double opt_pool_cross;
	//beam
	EXTERN unsigned min_beam_size;
	//fort_climb
	EXTERN unsigned yfpool;
	EXTERN unsigned efpool;
	EXTERN std::vector<Faction> factions;
	EXTERN bool invert_factions;
	EXTERN bool only_recent;
	EXTERN bool prefered_recent;
	EXTERN unsigned recent_percent;
	EXTERN std::vector<Skill::Skill> skills;
	EXTERN bool invert_skills;
	EXTERN std::vector<Skill::Skill> prefered_skills;
	EXTERN unsigned prefered_factor;

	//fixes
	EXTERN bool fixes[Fix::num_fixes];

    // TUO5 db of results
    // map<hash of proc,ydeck edeck decks but also more,result>
    //EXTERN std::map<std::string, Results<uint64_t>> database;
	EXTERN std::map<std::string,std::map<std::string,std::map<std::string,Results<uint64_t>>>> database;
    EXTERN hpmml::Model win_model, loss_model, stall_model, points_model;

    EXTERN long int db_limit;
    EXTERN bool use_strict_db;
    EXTERN bool use_db_load;
    EXTERN bool use_db_write;

    EXTERN bool use_ml;
    EXTERN bool use_only_ml;
    EXTERN double ml_precision;

#if defined(ANDROID) || defined(__ANDROID__)
	EXTERN JNIEnv *envv;
	EXTERN jobject objv;
#endif
}

namespace proc {
  EXTERN volatile unsigned thread_num_iterations; // written by threads
  EXTERN EvaluatedResults *thread_results; // written by threads
  EXTERN volatile const FinalResults<long double> *thread_best_results;
  EXTERN volatile bool thread_compare;
  EXTERN volatile bool thread_compare_stop; // written by threads
  EXTERN volatile bool destroy_threads;
}

using namespace tuo;
using namespace proc;

struct SimulationData;
class Process;
// some shared functions
FinalResults<long double> compute_score(const EvaluatedResults& results, std::vector<long double>& factors);
unsigned get_deck_cost(const Deck* deck);
void thread_evaluate(boost::barrier& main_barrier,
		boost::mutex& shared_mutex,
		SimulationData& sim,
		const Process& p,
		unsigned thread_id);
#if !defined(TEST)
int main(int argc, const char** argv);
#endif
DeckResults run(int argc, const char** argv);
void init();
bool is_timeout_reached();
bool valid_deck(Deck* your_deck);
std::vector<std::vector<const Card*>> get_candidate_lists(Process& proc);
std::string alpha_dominion_cost(const Card* dom_card);
#ifdef _OPENMP
std::vector<Results<uint64_t>> merge(std::vector<Results<uint64_t>> out, std::vector<Results<uint64_t>> in);
#endif
// some print functions
void print_score_info(const EvaluatedResults& results, std::vector<long double>& factors);
void print_results(const EvaluatedResults& results, std::vector<long double>& factors);
void print_cards_inline(std::vector<const Card*> cards,std::ostream& =std::cout, Deck* =nullptr);
void print_score_inline(const FinalResults<long double> score);
void print_sim_card_values(Deck* original_deck, Process& p, unsigned iter); // run_deck == p.your_decks[0]
void print_deck_inline(const unsigned deck_cost, const FinalResults<long double> score, Deck * deck,bool print_locked=false);
void print_upgraded_cards(Deck* deck);

struct SimulationData
{
	std::mt19937 re;
	const Cards& cards;
	const Decks& decks;
	std::vector<std::shared_ptr<Deck>> your_decks;
	std::vector<Hand*> your_hands;
	std::vector<std::shared_ptr<Deck>> enemy_decks;
	std::vector<Hand*> enemy_hands;
	std::vector<long double> factors;
	gamemode_t gamemode;
#ifndef NQUEST
	Quest quest;
#endif
	std::array<signed short, PassiveBGE::num_passive_bges> your_bg_effects, enemy_bg_effects;
	std::vector<SkillSpec> your_bg_skills, enemy_bg_skills;

	SimulationData(unsigned seed, const Cards& cards_, const Decks& decks_, unsigned num_your_decks_,unsigned num_enemy_decks_, std::vector<long double> factors_, gamemode_t gamemode_,
#ifndef NQUEST
			Quest & quest_,
#endif
			std::array<signed short, PassiveBGE::num_passive_bges>& your_bg_effects_,
			std::array<signed short, PassiveBGE::num_passive_bges>& enemy_bg_effects_,
			std::vector<SkillSpec>& your_bg_skills_,
			std::vector<SkillSpec>& enemy_bg_skills_) :
		re(seed),
		cards(cards_),
		decks(decks_),
		your_decks(num_your_decks_),
		enemy_decks(num_enemy_decks_),
		factors(factors_),
		gamemode(gamemode_),
#ifndef NQUEST
		quest(quest_),
#endif
		your_bg_effects(your_bg_effects_),
		enemy_bg_effects(enemy_bg_effects_),
		your_bg_skills(your_bg_skills_),
		enemy_bg_skills(enemy_bg_skills_)
		{
			for (size_t i = 0; i < num_your_decks_; ++i)
			{
				your_hands.emplace_back(new Hand(nullptr));
			}
			for (size_t i = 0; i < num_enemy_decks_; ++i)
			{
				enemy_hands.emplace_back(new Hand(nullptr));
			}
		}

	~SimulationData()
	{
		for (auto hand: enemy_hands) { delete(hand); }
		for (auto hand: your_hands) { delete(hand); }
	}

  void set_decks(std::vector<Deck*> const your_decks_, std::vector<Deck*> const & enemy_decks_);
  inline std::vector<Results<uint64_t>> evaluate(const  std::vector<bool> & deck_mask);
  inline std::vector<Results<uint64_t>> evaluate() ;


};
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
    const std::vector<Deck*> your_decks;
    const std::vector<Deck*> enemy_decks;
    std::vector<long double> factors;
    gamemode_t gamemode;
    #ifndef NQUEST
    Quest quest;
    #endif
    std::array<signed short, PassiveBGE::num_passive_bges> your_bg_effects, enemy_bg_effects;
    std::vector<SkillSpec> your_bg_skills, enemy_bg_skills;
    std::vector<bool> mask;
  public:
    Process(unsigned num_threads_, const Cards& cards_, const Decks& decks_, std::vector<Deck*> your_decks_, std::vector<Deck*> enemy_decks_, std::vector<long double> factors_, gamemode_t gamemode_,
#ifndef NQUEST
				Quest & quest_,
#endif
				std::array<signed short, PassiveBGE::num_passive_bges>& your_bg_effects_,
				std::array<signed short, PassiveBGE::num_passive_bges>& enemy_bg_effects_,
				std::vector<SkillSpec>& your_bg_skills_, std::vector<SkillSpec>& enemy_bg_skills_) :
			num_threads(num_threads_),
			main_barrier(num_threads+1),
			cards(cards_),
			decks(decks_),
			your_decks(your_decks_),
			enemy_decks(enemy_decks_),
			factors(factors_),
			gamemode(gamemode_),
#ifndef NQUEST
			quest(quest_),
#endif
			your_bg_effects(your_bg_effects_),
			enemy_bg_effects(enemy_bg_effects_),
			your_bg_skills(your_bg_skills_),
			enemy_bg_skills(enemy_bg_skills_)
			{
                mask = std::vector<bool>(your_decks.size() * enemy_decks.size(), true);
				destroy_threads = false;
				unsigned seed(sim_seed ? sim_seed : std::chrono::system_clock::now().time_since_epoch().count() * 2654435761);  // Knuth multiplicative hash
				if (num_threads_ == 1)
				{
					std::cout << "RNG seed " << seed << std::endl;
				}
				for (unsigned i(0); i < num_threads; ++i)
				{
					threads_data.push_back(new SimulationData(seed + i, cards, decks,your_decks.size(), enemy_decks.size(), factors, gamemode,
#ifndef NQUEST
								quest,
#endif
								your_bg_effects, enemy_bg_effects, your_bg_skills, enemy_bg_skills));
#ifndef _OPENMP
					threads.push_back(new boost::thread(thread_evaluate, std::ref(main_barrier), std::ref(shared_mutex), std::ref(*threads_data.back()), std::ref(*this), i));
#endif
				}
			}

		~Process()
		{
			destroy_threads = true;
#ifndef _OPENMP
			main_barrier.wait();
#endif
			for (auto thread: threads) { thread->join(); }
			for (auto data: threads_data) { delete(data); }
		}
  // all hashed except the array of decks
  std::string partial_hash();
  std::vector<unsigned int> partial_ids();
  // all hashed
  std::vector<std::array<std::string,3>> hashes();
  std::vector<std::unordered_map<std::string,std::string>> samples();

   bool eval_ml(unsigned num_iterations, EvaluatedResults & evaluated_results);
   bool check_db(std::vector<std::array<std::string,3>> const & vhashes,unsigned num_iterations, EvaluatedResults & evaluated_results);
   void save_db(std::vector<std::array<std::string,3>>  const & vhashes,EvaluatedResults & evaluated_results);

    EvaluatedResults & evaluate(unsigned num_iterations, EvaluatedResults & evaluated_results);
    EvaluatedResults & compare(unsigned num_iterations, EvaluatedResults & evaluated_results, const FinalResults<long double> & best_results);

#ifdef _OPENMP
		void openmp_evaluate_reduction(EvaluatedResults & evaluated_results);
    void openmp_compare_reduction(EvaluatedResults & evaluated_results);
#endif
};
