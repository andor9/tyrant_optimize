#include <iostream>
#include <vector>

#include "algorithms_util.h"

using namespace tuo;

inline bool try_improve_deck(Deck* d1, unsigned from_slot, unsigned to_slot, const Card* card_candidate,
		const Card*& best_commander, const Card*& best_alpha_dominion, std::vector<const Card*>& best_cards,
		FinalResults<long double>& best_score, unsigned& best_gap, std::string& best_deck,
		std::unordered_map<std::string, EvaluatedResults>& evaluated_decks, EvaluatedResults& zero_results,
		unsigned long& skipped_simulations, Process& proc, bool print
#ifndef NQUEST
		, Quest & quest
#endif
        )
{
	unsigned deck_cost(0);
	std::vector<std::pair<signed, const Card *>> cards_out, cards_in;
	std::mt19937& re = proc.threads_data[0]->re;

	// setup best deck
	d1->commander = best_commander;
	d1->alpha_dominion = best_alpha_dominion;
	d1->cards = best_cards;

	// try to adjust the deck
	if (!adjust_deck(d1, from_slot, to_slot, card_candidate, fund, re, deck_cost, cards_out, cards_in))
	{ return false; }

	// check gap
	unsigned new_gap = check_requirement(d1, requirement
#ifndef NQUEST
			, quest
#endif
			);
	if ((new_gap > 0) && (new_gap >= best_gap))
	{ return false; }

	// check previous simulations
	auto && cur_deck = d1->hash();
	auto && emplace_rv = evaluated_decks.insert({cur_deck, zero_results});
	auto & prev_results = emplace_rv.first->second;
	if (!emplace_rv.second)
	{
		skipped_simulations += prev_results.second;
	}

	// Evaluate new deck
	auto compare_results = proc.compare(best_score.n_sims, prev_results, best_score);
	auto current_score = compute_score(compare_results, proc.factors);

	// Is it better ?
	if (new_gap < best_gap || current_score.points > best_score.points + min_increment_of_score)
	{
		// Then update best score/slot, print stuff
		if(print)std::cout << "Deck improved: " << d1->hash() << ": " << card_slot_id_names(cards_out) << " -> " << card_slot_id_names(cards_in) << ": ";
		best_gap = new_gap;
		best_score = current_score;
		best_deck = cur_deck;
		best_commander = d1->commander;
		best_alpha_dominion = d1->alpha_dominion;
		best_cards = d1->cards;
		if(print)print_score_info(compare_results, proc.factors);
		if(print)print_deck_inline(deck_cost, best_score, d1);
		return true;
	}

	return false;
}
//------------------------------------------------------------------------------
/*
 * Calc value of current set deck in d1 (proc.your_decks[0])
 */
inline FinalResults<long double> fitness(Deck* d1,
		FinalResults<long double>& best_score,
		std::unordered_map<std::string, EvaluatedResults>& evaluated_decks, EvaluatedResults& zero_results,
		unsigned long& skipped_simulations, Process& proc, bool compare = false)
{

	// check previous simulations
	auto && cur_deck = d1->hash();
	//std::cout << "Deck hash: " << d1->hash() << " with ";
	auto && emplace_rv = evaluated_decks.insert({cur_deck, zero_results});
	auto & prev_results = emplace_rv.first->second;
	if (!emplace_rv.second)
	{
		skipped_simulations += prev_results.second;
	}

	// Evaluate new deck
	if (compare) {
		auto compare_results= proc.compare(best_score.n_sims, prev_results,best_score);
		auto current_score = compute_score(compare_results, proc.factors);
		return current_score;
	}
	else
	{
		auto compare_results= proc.evaluate(best_score.n_sims, prev_results);
		auto current_score = compute_score(compare_results, proc.factors);

		//best_score = current_score;
		//auto best_deck = d1->clone();
		//print_score_info(compare_results, proc.factors);
		//print_deck_inline(get_deck_cost(best_deck), best_score, best_deck);
		return current_score;
	}
}
//------------------------------------------------------------------------------
Deck* filter_best_deck(std::vector<Deck*> your_decks, Deck* d1,
		FinalResults<long double>& best_score,
		std::unordered_map<std::string, EvaluatedResults>& evaluated_decks, EvaluatedResults& zero_results,
		unsigned long& skipped_simulations, Process& proc) {
	Deck * cur_return = your_decks[0];
	FinalResults<long double> cur_score;
	for(unsigned i=1;i < your_decks.size();++i) //start with 1 since first always is simmed already
	{
		copy_deck(your_decks[i],d1);
		cur_score = fitness(d1, best_score, evaluated_decks, zero_results, skipped_simulations, proc,true);
		if(cur_score.points > best_score.points)
		{
			cur_return = your_decks[i];
			best_score = cur_score;

			std::cout << "Deck improved: " << d1->hash() <<":";
			print_deck_inline(get_deck_cost(d1), best_score,d1);
		}
	}
	return cur_return;
}

//------------------------------------------------------------------------------
DeckResults hill_climbing(unsigned num_min_iterations, unsigned num_iterations, std::vector<Deck*> your_decks , Process& proc, Requirement & requirement
#ifndef NQUEST
		, Quest & quest
#endif
		)
{
	Deck * d1 = proc.your_decks[0];
	EvaluatedResults zero_results = { EvaluatedResults::first_type(proc.enemy_decks.size()), 0 };
	std::string best_deck = d1->hash();
	std::unordered_map<std::string, EvaluatedResults> evaluated_decks{{best_deck, zero_results}};
	EvaluatedResults& results = proc.evaluate(num_min_iterations, evaluated_decks.begin()->second);
	print_score_info(results, proc.factors);
	FinalResults<long double> best_score = compute_score(results, proc.factors);
	unsigned long skipped_simulations = 0;

	// use the best deck from all passed decks
	copy_deck(filter_best_deck(your_decks, d1, best_score, evaluated_decks, zero_results, skipped_simulations, proc),d1);

	// update freezed_cards
	freezed_cards = std::min<unsigned>(opt_freezed_cards, d1->cards.size());
	const Card* best_commander = d1->commander;
	const Card* best_alpha_dominion = d1->alpha_dominion;
	std::vector<const Card*> best_cards = d1->cards;
	unsigned deck_cost = get_deck_cost(d1);
	fund = std::max(fund, deck_cost);
	print_deck_inline(deck_cost, best_score, d1,true);
	std::mt19937& re = proc.threads_data[0]->re;
	unsigned best_gap = check_requirement(d1, requirement
#ifndef NQUEST
			, quest
#endif
			);
	bool is_random = (d1->strategy == DeckStrategy::random) || (d1->strategy == DeckStrategy::flexible);
	bool deck_has_been_improved = true;
	std::vector<const Card*> commander_candidates;
	std::vector<const Card*> alpha_dominion_candidates;
	std::vector<const Card*> card_candidates;

	auto mixed_candidates = get_candidate_lists(proc);
	commander_candidates = mixed_candidates.at(0);
	alpha_dominion_candidates = mixed_candidates.at(1);
	card_candidates = mixed_candidates.at(2);
	// add current alpha dominion to candidates if necessary
	// or setup first candidate into the deck if no alpha dominion defined
	if (use_owned_dominions)
	{
		if (best_alpha_dominion)
		{
			if (!std::count(alpha_dominion_candidates.begin(), alpha_dominion_candidates.end(), best_alpha_dominion))
			{
				alpha_dominion_candidates.emplace_back(best_alpha_dominion);
			}
		}
		else if (!alpha_dominion_candidates.empty())
		{
			best_alpha_dominion = d1->alpha_dominion = alpha_dominion_candidates[0];
		}
		if (debug_print > 0)
		{
			for (const Card* dom_card : alpha_dominion_candidates)
			{
				std::cout << " ** next Alpha Dominion candidate: " << dom_card->m_name
					<< " ($: " << alpha_dominion_cost(dom_card) << ")" << std::endl;
			}
		}
		if (!best_alpha_dominion && owned_alpha_dominion)
		{
			best_alpha_dominion = owned_alpha_dominion;
			std::cout << "Setting up owned Alpha Dominion into a deck: " << best_alpha_dominion->m_name << std::endl;
		}
	}
	//std::reverse(card_candidates.begin(), card_candidates.end());



	// << main climbing loop >>
	for (unsigned from_slot(freezed_cards), dead_slot(freezed_cards); ;
			from_slot = std::max(freezed_cards, (from_slot + 1) % std::min<unsigned>(max_deck_len, best_cards.size() + 1)))
	{
		if(is_timeout_reached()){ break;}
		if (deck_has_been_improved)
		{
			dead_slot = from_slot;
			deck_has_been_improved = false;
		}
		else if (from_slot == dead_slot || best_score.points - target_score > -1e-9)
		{
			if (best_score.n_sims >= num_iterations || best_gap > 0)
			{ break; } // exit main climbing loop
			auto & prev_results = evaluated_decks[best_deck];
			skipped_simulations += prev_results.second;
			// Re-evaluate the best deck
			d1->commander = best_commander;
			d1->alpha_dominion = best_alpha_dominion;
			d1->cards = best_cards;
			auto evaluate_result = proc.evaluate(std::min(prev_results.second * iterations_multiplier, num_iterations), prev_results);
			best_score = compute_score(evaluate_result, proc.factors);
			print_score_info(evaluate_result, proc.factors);
			dead_slot = from_slot;
		}
		if (best_score.points - target_score > -1e-9)
		{ continue; }

		// commander
		if (requirement.num_cards.count(best_commander) == 0)
		{
			// << commander candidate loop >>
			for (const Card* commander_candidate: commander_candidates)
			{
				if (best_score.points - target_score > -1e-9)
				{ break; }
				if (commander_candidate == best_commander)
				{ continue; }
				deck_has_been_improved |= try_improve_deck(d1, -1, -1, commander_candidate,
						best_commander, best_alpha_dominion, best_cards, best_score, best_gap, best_deck,
						evaluated_decks, zero_results, skipped_simulations, proc,true
#ifndef NQUEST
			, quest
#endif
                        );
			}
			// Now that all commanders are evaluated, take the best one
			d1->commander = best_commander;
			d1->alpha_dominion = best_alpha_dominion;
			d1->cards = best_cards;
		}

		// alpha dominion
		if (use_owned_dominions && !alpha_dominion_candidates.empty())
		{
			// << alpha dominion candidate loop >>
			for (const Card* alpha_dominion_candidate: alpha_dominion_candidates)
			{
				if (best_score.points - target_score > -1e-9)
				{ break; }
				if (alpha_dominion_candidate == best_alpha_dominion)
				{ continue; }
				deck_has_been_improved |= try_improve_deck(d1, -1, -1, alpha_dominion_candidate,
						best_commander, best_alpha_dominion, best_cards, best_score, best_gap, best_deck,
						evaluated_decks, zero_results, skipped_simulations, proc,true
#ifndef NQUEST
			, quest
#endif
                        );
			}
			// Now that all alpha dominions are evaluated, take the best one
			d1->commander = best_commander;
			d1->alpha_dominion = best_alpha_dominion;
			d1->cards = best_cards;
		}

		// shuffle candidates
		std::shuffle(card_candidates.begin(), card_candidates.end(), re);

		// << card candidate loop >>
		//for (const Card* card_candidate: card_candidates)
		for (auto it = card_candidates.begin(); it != card_candidates.end();++it)
		{
			const Card* card_candidate = *it;
			for (unsigned to_slot(is_random ? from_slot : card_candidate ? freezed_cards : (best_cards.size() - 1));
					to_slot < (is_random ? (from_slot + 1) : (best_cards.size() + (from_slot < best_cards.size() ? 0 : 1)));
					++ to_slot)
			{
				if (card_candidate ?
						(from_slot < best_cards.size() && (from_slot == to_slot && card_candidate == best_cards[to_slot])) // 2 Omega -> 2 Omega
						:
						(from_slot == best_cards.size())) // void -> void
				{ continue; }
				deck_has_been_improved |= try_improve_deck(d1, from_slot, to_slot, card_candidate,
						best_commander, best_alpha_dominion, best_cards, best_score, best_gap, best_deck,
						evaluated_decks, zero_results, skipped_simulations, proc,true
#ifndef NQUEST
			, quest
#endif
                        );
			}
			if (best_score.points - target_score > -1e-9)
			{ break; }

		}
	}
	d1->commander = best_commander;
	d1->alpha_dominion = best_alpha_dominion;
	d1->cards = best_cards;
	unsigned simulations = 0;
	for (auto evaluation: evaluated_decks)
	{ simulations += evaluation.second.second; }
	std::cout << "Evaluated " << evaluated_decks.size() << " decks (" << simulations << " + " << skipped_simulations << " simulations)." << std::endl;
	print_sim_card_values(d1,proc,num_iterations);
	std::cout << "Optimized Deck: ";
	print_deck_inline(get_deck_cost(d1), best_score, d1,true);
	print_upgraded_cards(d1);
	return std::make_pair(d1->clone(),best_score);
}




inline long double acceptanceProbability(long double old_score, long double new_score, long double temperature)
{
	if(new_score > old_score)
	{
		return 1;
	}
	//100/max_score normalizes the acceptance probability with the used mode/score-range
	//1000 is factor to set the temperature lower (can be changed indirect by setting begin-temperature and its decrease)
	return exp(((new_score-old_score)/temperature*1000*100)/max_possible_score[(size_t)optimization_mode]);
}

DeckResults simulated_annealing(unsigned num_min_iterations, unsigned num_iterations, std::vector<Deck*> your_decks, Process& proc, Requirement & requirement
#ifndef NQUEST
		, Quest & quest
#endif
		)
{
	Deck* cur_deck = proc.your_decks[0];
	EvaluatedResults zero_results = { EvaluatedResults::first_type(proc.enemy_decks.size()), 0 };
	//std::string best_deck = d1->hash();
	std::unordered_map<std::string, EvaluatedResults> evaluated_decks{{cur_deck->hash(), zero_results}};
	EvaluatedResults& results = proc.evaluate(num_min_iterations, evaluated_decks.begin()->second);
	print_score_info(results, proc.factors);
	FinalResults<long double> best_score = compute_score(results, proc.factors);
	//const Card* best_commander = d1->commander;
	//const Card* best_alpha_dominion = cur_deck->alpha_dominion;
	//std::vector<const Card*> best_cards = cur_deck->cards;
	unsigned long skipped_simulations = 0;


	// use the best deck from all passed decks
	copy_deck(filter_best_deck(your_decks, cur_deck, best_score, evaluated_decks, zero_results, skipped_simulations, proc),cur_deck);

	// update freezed_cards
	freezed_cards = std::min<unsigned>(opt_freezed_cards, cur_deck->cards.size());

	unsigned deck_cost = get_deck_cost(cur_deck);
	fund = std::max(fund, deck_cost);
	print_deck_inline(deck_cost, best_score, cur_deck,true);
	std::mt19937& re = proc.threads_data[0]->re;
	unsigned cur_gap = check_requirement(cur_deck, requirement
#ifndef NQUEST
			, quest
#endif
			);

	bool is_random = (cur_deck->strategy == DeckStrategy::random) || (cur_deck->strategy == DeckStrategy::flexible);
	std::vector<const Card*> all_candidates;

	auto mixed_candidates = get_candidate_lists(proc);
	all_candidates.reserve(mixed_candidates.at(0).size()+mixed_candidates.at(1).size()+mixed_candidates.at(2).size());
	all_candidates.insert(all_candidates.end(), std::make_move_iterator(mixed_candidates.at(0).begin()),std::make_move_iterator(mixed_candidates.at(0).end()));
	all_candidates.insert(all_candidates.end(), std::make_move_iterator(mixed_candidates.at(1).begin()),std::make_move_iterator(mixed_candidates.at(1).end()));
	all_candidates.insert(all_candidates.end(), std::make_move_iterator(mixed_candidates.at(2).begin()),std::make_move_iterator(mixed_candidates.at(2).end()));
	//clear
	mixed_candidates.at(0).clear(); mixed_candidates.at(0).shrink_to_fit();
	mixed_candidates.at(1).clear(); mixed_candidates.at(1).shrink_to_fit();
	mixed_candidates.at(2).clear(); mixed_candidates.at(2).shrink_to_fit();
	mixed_candidates.clear();mixed_candidates.shrink_to_fit();

	// add current alpha dominion to candidates if necessary
	// or setup first candidate into the deck if no alpha dominion defined
	if (use_owned_dominions)
	{
		if (cur_deck->alpha_dominion)
		{
			if (!std::count(all_candidates.begin(), all_candidates.end(), cur_deck->alpha_dominion))
			{
				all_candidates.emplace_back(cur_deck->alpha_dominion);
			}
		}
		if (!cur_deck->alpha_dominion && owned_alpha_dominion)
		{
			cur_deck->alpha_dominion = owned_alpha_dominion;
			std::cout << "Setting up owned Alpha Dominion into a deck: " << cur_deck->alpha_dominion->m_name << std::endl;
		}
	}

	Deck* prev_deck = cur_deck->clone();
	Deck* best_deck = cur_deck->clone();

	FinalResults<long double> prev_score = best_score;
	FinalResults<long double> cur_score = best_score;

	unsigned best_gap = cur_gap;

	deck_cost = 0;

	unsigned from_slot(freezed_cards);
	unsigned from_slot_tmp(freezed_cards);
	unsigned to_slot(1);

	if(debug_print >0)std::cout << "Starting Anneal" << std::endl;
	while(temperature > 1 && !(best_score.points - target_score > -1e-9 || is_timeout_reached()))
	{
		cur_deck->commander = prev_deck->commander;
		cur_deck->alpha_dominion = prev_deck->alpha_dominion;
		cur_deck->cards = prev_deck->cards;
		from_slot = std::max(freezed_cards, (from_slot+1) % std::min<unsigned>(max_deck_len, cur_deck->cards.size() +1));
		const Card* candidate = all_candidates.at(std::uniform_int_distribution<unsigned>(0,all_candidates.size()-1)(re));


		if((!candidate || (candidate->m_category == CardCategory::normal && candidate->m_type != CardType::commander && candidate->m_category != CardCategory::dominion_alpha)))
		{

			to_slot = std::uniform_int_distribution<unsigned>(is_random ? from_slot : candidate ? freezed_cards : (cur_deck->cards.size() -1),(is_random ? (from_slot+1) : (cur_deck->cards.size() + ( from_slot < cur_deck->cards.size() ? 0 : 1)))-1)(re);
			if(candidate ?
					(from_slot < cur_deck->cards.size() && (from_slot == to_slot && candidate == cur_deck->cards[to_slot]))
					:
					(from_slot == cur_deck->cards.size()))
			{
				continue;
			}
			from_slot_tmp = from_slot;
		}
		else if(candidate->m_type == CardType::commander && requirement.num_cards.count(cur_deck->commander) == 0)
		{
			cur_deck->commander = candidate;
			from_slot_tmp = -1;
			to_slot = -1;
		}
		else if(candidate->m_category == CardCategory::dominion_alpha && use_owned_dominions)
		{
			cur_deck->alpha_dominion = candidate;
			from_slot_tmp = -1;
			to_slot = -1;
		}
		else{
			continue;
		}

		std::vector<std::pair<signed, const Card * >> cards_out, cards_in;
		if (!adjust_deck(cur_deck, from_slot_tmp, to_slot, candidate, fund, re, deck_cost, cards_out, cards_in))
		{ continue;}
		cur_gap = check_requirement(cur_deck, requirement
#ifndef NQUEST
				, quest
#endif
				);
		if ((cur_gap > 0) && (cur_gap >= best_gap))
		{ continue; }

		//same deck skip
		if(cur_deck->hash().compare(prev_deck->hash())==0)continue;



		cur_score = fitness(cur_deck, best_score, evaluated_decks, zero_results, skipped_simulations, proc);

		if(acceptanceProbability(prev_score.points, cur_score.points , temperature) > std::uniform_real_distribution<double>(0,1)(re))
		{
			if(cur_score.points > best_score.points)
			{
				best_score = cur_score;
				best_deck = cur_deck->clone();
				best_gap = cur_gap;
				std::cout << "Deck improved: " << best_deck->hash() << ": (temp=" << temperature << ") :";
				print_deck_inline(get_deck_cost(best_deck), best_score, best_deck);
			}
			if(debug_print>0)std::cout << "UPDATED DECK: " << cur_deck->hash() << ": (temp=" << temperature << ") :";
			if(debug_print>0)print_deck_inline(get_deck_cost(cur_deck), cur_score, cur_deck);
			prev_score = cur_score;
			prev_deck = cur_deck->clone();
		}
		temperature *=1-coolingRate;
	}
	unsigned simulations = 0;
	for (auto evaluation: evaluated_decks)
	{ simulations += evaluation.second.second; }
	std::cout << "Evaluated " << evaluated_decks.size() << " decks (" << simulations << " + " << skipped_simulations << " simulations)." << std::endl;
	print_sim_card_values(best_deck,proc,num_iterations);
	std::cout << "Optimized Deck: ";
	print_deck_inline(get_deck_cost(best_deck), best_score, best_deck,true);
	print_upgraded_cards(best_deck);
	return std::make_pair(best_deck->clone(),best_score);
}




void crossover(Deck* src1,Deck* src2, Deck* cur_deck, std::mt19937& re,unsigned best_gap,std::unordered_map<std::string, EvaluatedResults>& evaluated_decks
#ifndef NQUEST
		, Quest & quest
#endif
)
{
	cur_deck->commander = std::uniform_int_distribution<unsigned>(0, 1)(re)?src1->commander:src2->commander;
	cur_deck->alpha_dominion = std::uniform_int_distribution<unsigned>(0, 1)(re)?src1->alpha_dominion:src2->alpha_dominion;
	bool finished = false;
	unsigned itr = 0;
	while(!finished && itr < 100) //todo opt
	{
		itr++;
		cur_deck->cards.clear();
		for(unsigned it =0; it < std::max(src1->cards.size(),src2->cards.size());it++)
		{
			if(src1->cards.size() <=it)
			{cur_deck->cards.push_back(src2->cards[it]);}
			else if(src2->cards.size() <=it)
			{cur_deck->cards.push_back(src1->cards[it]);}
			else
			{
				cur_deck->cards.push_back(std::uniform_int_distribution<unsigned>(0, 1)(re)?src1->cards[it]:src2->cards[it]);
			}
		}
		if(!valid_deck(cur_deck)) {continue;} //repeat

		if(evaluated_decks.count(cur_deck->hash())){continue;} // deck already simmed
		unsigned cur_gap = check_requirement(cur_deck, requirement
#ifndef NQUEST
				, quest
#endif
				);
		if ((cur_gap > 0) && (cur_gap >= best_gap))
		{ continue; }
		finished = true; // exit while
	}
	if(!finished) copy_deck(std::uniform_int_distribution<unsigned>(0, 1)(re)?src1:src2,cur_deck);
}

void mutate(Deck* src, Deck* cur_deck, std::vector<const Card*> all_candidates, std::mt19937& re,unsigned best_gap,std::unordered_map<std::string, EvaluatedResults>& evaluated_decks
#ifndef NQUEST
		, Quest & quest
#endif
)
{
	copy_deck(src,cur_deck);

	bool is_random = (cur_deck->strategy == DeckStrategy::random) || (cur_deck->strategy == DeckStrategy::flexible); //should be same for all decks from input!?

	unsigned deck_cost = 0;

	unsigned from_slot(freezed_cards);
	unsigned from_slot_tmp(freezed_cards);
	unsigned to_slot(1);

	bool finished = false;
	unsigned itr=0;
	while(!finished && itr < 100) // todo opt
	{
		itr++;
		copy_deck(src,cur_deck);

		from_slot = std::uniform_int_distribution<unsigned>(1,std::min<unsigned>(max_deck_len-1, cur_deck->cards.size()))(re);

		const Card* candidate = all_candidates.at(std::uniform_int_distribution<unsigned>(0,all_candidates.size()-1)(re));


		if((!candidate || (candidate->m_category == CardCategory::normal && candidate->m_type != CardType::commander && candidate->m_category != CardCategory::dominion_alpha)))
		{

			to_slot = std::uniform_int_distribution<unsigned>(is_random ? from_slot : candidate ? freezed_cards : (cur_deck->cards.size() -1),(is_random ? (from_slot+1) : (cur_deck->cards.size() + ( from_slot < cur_deck->cards.size() ? 0 : 1)))-1)(re);
			if(candidate ?
					(from_slot < cur_deck->cards.size() && (from_slot == to_slot && candidate == cur_deck->cards[to_slot]))
					:
					(from_slot == cur_deck->cards.size()))
			{
				continue;
			}
			from_slot_tmp = from_slot;
		}
		else if(candidate->m_type == CardType::commander && requirement.num_cards.count(cur_deck->commander) == 0)
		{
			cur_deck->commander = candidate;
			from_slot_tmp = -1;
			to_slot = -1;
		}
		else if(candidate->m_category == CardCategory::dominion_alpha && use_owned_dominions)
		{
			cur_deck->alpha_dominion = candidate;
			from_slot_tmp = -1;
			to_slot = -1;
		}
		else{
			continue;
		}

		std::vector<std::pair<signed, const Card * >> cards_out, cards_in;
		if (!adjust_deck(cur_deck, from_slot_tmp, to_slot, candidate, fund, re, deck_cost, cards_out, cards_in))
		{ continue;}

		if(evaluated_decks.count(cur_deck->hash()))
		{continue;} // deck already simmed
		if(!valid_deck(cur_deck)) {continue;} //repeat
		unsigned cur_gap = check_requirement(cur_deck, requirement
#ifndef NQUEST
				, quest
#endif
				);
		if ((cur_gap > 0) && (cur_gap >= best_gap))
		{ continue; }
		finished = true; // exit while
	}
	if(!finished) copy_deck(src,cur_deck);
}
DeckResults genetic_algorithm(unsigned num_min_iterations, unsigned num_iterations, std::vector<Deck*> your_decks, Process& proc, Requirement & requirement
#ifndef NQUEST
		, Quest & quest
#endif
		)
{
	//std::cerr << "START GENETIC" << std::endl;
	if(pool_size==0){
		if(your_decks.size()>min_pool_size) { //
			pool_size = your_decks.size();
		}
		else {
			pool_size = min_pool_size;
		}
	}
	unsigned pool_cross = pool_size*opt_pool_cross/(opt_pool_mutate+opt_pool_cross+opt_pool_keep);
	unsigned pool_keep = pool_size*opt_pool_keep/(opt_pool_mutate+opt_pool_cross+opt_pool_keep);
	unsigned pool_mutate = pool_size-pool_cross-pool_keep;

	//your_decks.size();
	std::vector<std::pair<Deck*,FinalResults<long double>>> pool;
	Deck* cur_deck = proc.your_decks[0];



	EvaluatedResults zero_results = { EvaluatedResults::first_type(proc.enemy_decks.size()), 0 };

	std::unordered_map<std::string, EvaluatedResults> evaluated_decks{{cur_deck->hash(), zero_results}};
	EvaluatedResults& results = proc.evaluate(num_min_iterations, evaluated_decks.begin()->second);
	print_score_info(results, proc.factors);
	FinalResults<long double> best_score = compute_score(results, proc.factors); //init sim, todo remove

	unsigned deck_cost = get_deck_cost(cur_deck);
	fund = std::max(fund, deck_cost);
	print_deck_inline(deck_cost, best_score, cur_deck,true);
	std::mt19937& re = proc.threads_data[0]->re;
	unsigned cur_gap = check_requirement(cur_deck, requirement
#ifndef NQUEST
			, quest
#endif
			);

	unsigned long skipped_simulations = 0;
	std::vector<const Card*> all_candidates;

	auto mixed_candidates = get_candidate_lists(proc);
	all_candidates.reserve(mixed_candidates.at(0).size()+mixed_candidates.at(1).size()+mixed_candidates.at(2).size());
	all_candidates.insert(all_candidates.end(), std::make_move_iterator(mixed_candidates.at(0).begin()),std::make_move_iterator(mixed_candidates.at(0).end()));
	all_candidates.insert(all_candidates.end(), std::make_move_iterator(mixed_candidates.at(1).begin()),std::make_move_iterator(mixed_candidates.at(1).end()));
	all_candidates.insert(all_candidates.end(), std::make_move_iterator(mixed_candidates.at(2).begin()),std::make_move_iterator(mixed_candidates.at(2).end()));
	//clear
	mixed_candidates.at(0).clear(); mixed_candidates.at(0).shrink_to_fit();
	mixed_candidates.at(1).clear(); mixed_candidates.at(1).shrink_to_fit();
	mixed_candidates.at(2).clear(); mixed_candidates.at(2).shrink_to_fit();
	mixed_candidates.clear();mixed_candidates.shrink_to_fit();

	// add current alpha dominion to candidates if necessary
	// or setup first candidate into the deck if no alpha dominion defined

	for( auto tmp_deck : your_decks) // check all decks for owned dominions
	{
		if (use_owned_dominions)
		{
			if (tmp_deck->alpha_dominion)
			{
				if (!std::count(all_candidates.begin(), all_candidates.end(), tmp_deck->alpha_dominion))
				{
					all_candidates.emplace_back(tmp_deck->alpha_dominion);
				}
			}
			if (!tmp_deck->alpha_dominion && owned_alpha_dominion)
			{
				tmp_deck->alpha_dominion = owned_alpha_dominion;
				std::cout << "Setting up owned Alpha Dominion into a deck: " << tmp_deck->alpha_dominion->m_name << std::endl;
			}
		}
	}
	Deck* best_deck = cur_deck->clone();
	FinalResults<long double> cur_score = best_score;
	unsigned best_gap = cur_gap;

	//fill pool
	if(your_decks.size()>pool_size) your_decks.resize(pool_size); //TODO this drops potential better overflowing decks?!
	for ( unsigned it =your_decks.size(); it < pool_size;it++)
	{
		unsigned j = std::uniform_int_distribution<unsigned>(0,your_decks.size()-1)(re);
		unsigned i = std::uniform_int_distribution<unsigned>(0,your_decks.size()-1)(re);
		Deck* nxt = your_decks[j]->clone();
		if(std::uniform_int_distribution<unsigned>(0,1)(re))
		{
			mutate(your_decks[i],nxt,all_candidates,re,best_gap,evaluated_decks
#ifndef NQUEST
					, quest
#endif
            );
		}
		else
		{
			crossover(your_decks[i],your_decks[j],nxt,re,best_gap,evaluated_decks
#ifndef NQUEST
					, quest
#endif
            );
		}
		your_decks.push_back(nxt);
	}
	//sim pool
	for( auto i_deck :your_decks)
	{
		copy_deck(i_deck,cur_deck);
		cur_score = fitness(cur_deck, best_score, evaluated_decks, zero_results, skipped_simulations, proc,true);
		pool.push_back(std::make_pair(i_deck,cur_score));
		if(cur_score.points > best_score.points)
		{
			best_score = cur_score;
			best_deck = cur_deck->clone();
			best_gap = check_requirement(cur_deck, requirement
#ifndef NQUEST
					, quest
#endif
					);
			std::cout << "Deck improved: " << best_deck->hash() <<":";
			print_deck_inline(get_deck_cost(best_deck), best_score, best_deck);
		}
	}

	for( unsigned gen= 0; gen< generations && !is_timeout_reached() ;gen++ )
	{
		std::cout << "GENERATION: " << gen << std::endl;

		//sort
		auto sort = [](std::pair<Deck*,FinalResults<long double>> l,std::pair<Deck*,FinalResults<long double>> r) {return l.second.points > r.second.points;};
		std::sort(pool.begin(),pool.end(),sort);
		//breed
		//cross
		for ( unsigned it = 0; it < pool_cross;it++)
		{
			unsigned i = std::uniform_int_distribution<unsigned>(0,pool_keep-1)(re);
			unsigned j = std::uniform_int_distribution<unsigned>(pool_keep,pool_size-pool_mutate)(re);
			//unsigned  k= std::uniform_int_distribution<unsigned>(0,pool_size-pool_mutate)(re);
			unsigned k = -1;
			while (k >= pool_size-pool_mutate)
				k=std::geometric_distribution<unsigned>(0.2)(re); //prefer crossover with strong decks
			crossover(pool[i].first,pool[k].first,pool[j].first,re,best_gap, evaluated_decks
#ifndef NQUEST
					, quest
#endif
            );
			//crossover(pool[it+pool_size/4*2].first,pool[it+pool_size/4*3].first,pool[it+pool_size/4*3].first,re,best_gap, evaluated_decks);
			//crossover(pool[it].first,pool[(it+pool_size/8)%(pool_size/4)].first,pool[it+pool_size/4*2].first,re,best_gap, evaluated_decks);
			//mutate(pool[it].first,pool[it+pool_size/4*3].first,all_candidates,re,best_gap, evaluated_decks);
		}
		//mutate pool_keep to replace lowest scores
		for ( unsigned it = pool_size-pool_mutate; it < pool_size;it++)
		{
			unsigned i = std::uniform_int_distribution<unsigned>(0,pool_keep-1)(re);
			//unsigned j = std::uniform_int_distribution<unsigned>(pool_keep,pool_size-1)(re);
			mutate(pool[i].first,pool[it].first,all_candidates,re,best_gap, evaluated_decks
#ifndef NQUEST
					, quest
#endif
            );
		}
		//mutate duplicates
		for ( unsigned it = 0; it < pool_size;it++)
		{
			for (unsigned i = it+1; i < pool_size;i++)
			{
				if(pool[it].first->alpha_dominion && pool[i].first->alpha_dominion && pool[it].first->hash().substr(8)==pool[i].first->hash().substr(8)) //ignore commander + dominion
				{
					mutate(pool[i].first->clone(),pool[i].first,all_candidates,re,best_gap, evaluated_decks
#ifndef NQUEST
					, quest
#endif
                    );

					FinalResults<long double> nil{0, 0, 0, 0, 0, 0, 1};
					pool[i].second = nil; //lowest score approx Null
				}
			}
		}
		//calc fitness
		for (unsigned it = pool_keep; it < pool_size; it++)
		{
			copy_deck(pool[it].first,cur_deck);
			cur_score = fitness(cur_deck, best_score, evaluated_decks, zero_results, skipped_simulations, proc,true);
			pool[it].second = cur_score;
			if(cur_score.points > best_score.points)
			{
				best_score = cur_score;
				best_deck = cur_deck->clone();
				best_gap = check_requirement(cur_deck, requirement
#ifndef NQUEST
						, quest
#endif
						);
				if(it < pool_size-pool_mutate)
				{
					if (debug_print >= 0)
						std::cout << "Crossover: " <<std::endl;
					std::cout << "Deck improved: " << best_deck->hash() <<":";
				}
				else
				{
					if (debug_print >= 0)
						std::cout << "Mutation: " <<std::endl;
					std::cout << "Deck improved: " << best_deck->hash() <<":";
				}

				print_deck_inline(get_deck_cost(best_deck), best_score, best_deck);
			}
		}
#ifndef NDEBUG
		if (debug_print >= 0)
		{
			std::cout << "---------------POOL---------------" << std::endl;
			for (unsigned it =0; it < pool.size();++it)
			{
				if(it==0)std::cout << "---------------KEEP---------------" << std::endl;
				if(it==pool_keep)std::cout << "---------------CROSS--------------" << std::endl;
				if(it==pool_keep+pool_cross)std::cout << "---------------MUTATE-------------" << std::endl;
				auto a = pool[it];
				print_deck_inline(get_deck_cost(a.first),a.second,a.first);
			}
			std::cout << "---------------PEND---------------" << std::endl;
		}
#endif
	}

	unsigned simulations = 0;
	for (auto evaluation: evaluated_decks)
	{ simulations += evaluation.second.second; }
	std::cout << "Evaluated " << evaluated_decks.size() << " decks (" << simulations << " + " << skipped_simulations << " simulations)." << std::endl;
	print_sim_card_values(best_deck,proc,num_iterations);
	std::cout << "Optimized Deck: ";
	print_deck_inline(get_deck_cost(best_deck), best_score, best_deck,true);
	print_upgraded_cards(best_deck);
	return std::make_pair(best_deck->clone(),best_score);
}


unsigned factorial(unsigned n)
{
	unsigned retval = 1;
	for (int i = n; i > 1; --i)
		retval *= i;
	return retval;
}

void recursion(unsigned num_iterations, std::vector<unsigned> used, unsigned pool, std::vector<const Card*> forts,Process&proc, FinalResults<long double>& best_score,std::vector<const Card*> & best_forts,std::unordered_map<std::string,EvaluatedResults> & evaluated_decks, EvaluatedResults& zero_results, unsigned long& skipped_simulations)
{
	if(used.size()==pool)
	{
		for(auto your_deck : proc.your_decks)
		{
			your_deck->fortress_cards.clear();
			for(unsigned i = 0; i < pool;++i)
			{
				your_deck->fortress_cards.emplace_back(forts[used[i]]);
			}
		}
		//sim
		std::stringstream ios;
		encode_deck_ext_b64(ios,proc.your_decks[0]->fortress_cards);
		auto hash = ios.str();
		auto && emplace_rv = evaluated_decks.insert({hash,zero_results});
		auto & prev_results = emplace_rv.first->second;
		if(!emplace_rv.second)
		{
			skipped_simulations += prev_results.second;
		}
		auto compare_results= proc.evaluate(num_iterations, prev_results);
		auto current_score = compute_score(compare_results, proc.factors);

		if(current_score.points > best_score.points+min_increment_of_score) {
			best_score = current_score;
			std::vector<const Card*> copy_forts(proc.your_decks[0]->fortress_cards);
			best_forts = copy_forts;
			std::cout << "Forts improved: " << hash << " : ";
			print_score_info(compare_results, proc.factors);
			print_score_inline(best_score);
			std::cout << ": ";
			print_cards_inline(best_forts);
		}
		used.clear();
		used.shrink_to_fit();
	}
	else
	{
		for(unsigned i =0;i < forts.size();++i)
		{
			if(std::find(used.begin(),used.end(),i)==used.end()) //not contained
			{
				std::vector<unsigned> tmp_used (used);
				tmp_used.emplace_back(i);
				recursion(num_iterations,tmp_used,pool,forts,proc,best_score,best_forts,evaluated_decks,zero_results,skipped_simulations);
			}
		}
	}
}

DeckResults forts_climbing(unsigned num_iterations, Process& proc) {
	EvaluatedResults zero_results = { EvaluatedResults::first_type(proc.enemy_decks.size()*proc.your_decks.size()), 0 };
	unsigned pool = std::get<0>(proc.your_decks[0]->variable_forts[0]);
	std::vector<const Card*> forts(std::get<2>(proc.your_decks[0]->variable_forts[0]));
	for(unsigned i =0; i < proc.your_decks.size();++i)
	{
		proc.your_decks[i]->variable_forts.clear();
	}
	std::vector<unsigned> used;
	used.reserve(pool);
	std::vector<const Card*> best_forts{pool};
	FinalResults<long double> best_score{0, 0, 0, 0, 0, 0, 0};
	unsigned long skipped_simulations{0};
	std::unordered_map<std::string,EvaluatedResults> evaluated_decks{{"",zero_results}};
	recursion(num_iterations,used,pool,forts, proc,best_score,best_forts,evaluated_decks,zero_results,skipped_simulations);
	unsigned simulations = 0;
	for (auto evaluation: evaluated_decks)
	{ simulations += evaluation.second.second; }
	std::cout << "Evaluated " << evaluated_decks.size() << " decks (" << simulations << " + " << skipped_simulations << " simulations)." << std::endl;
	std::cout << "Optimized Deck: ";
	print_cards_inline(best_forts);
	Deck* tmp = proc.your_decks[0]->clone();
	tmp->commander = nullptr;
	tmp->alpha_dominion = nullptr;
	tmp->cards = best_forts; // return forts
	return std::make_pair(tmp,best_score);
}

inline bool contains(std::multimap<FinalResults<long double>, Deck*, std::greater<FinalResults<long double>>> best,Deck* d)
{
		for(auto it = best.begin();it!=best.end();it++)
		{
			if(it->second->hash().compare(d->hash())==0) return true;
		}
		return false;
}

inline void check_and_update(std::multimap<FinalResults<long double>, Deck*, std::greater<FinalResults<long double>>>& best,Deck* cur_deck,bool & deck_has_been_improved, FinalResults<long double>& best_score )
{
	auto tmp_result = (best.size()<pool_size)?0.:(std::next(best.begin(),pool_size))->first.points;
	//std::cout << "IMP CMD" << std::endl;
	if(best_score.points > tmp_result && !contains(best,cur_deck))
	{
			deck_has_been_improved = true;
			if(best_score > best.begin()->first){
					std::cout << "Deck improved: " << cur_deck->hash() <<":" << std::endl;
					print_deck_inline(get_deck_cost(cur_deck), best_score, cur_deck);
			}
			best.insert(std::make_pair(best_score,cur_deck->clone()));
			if(best.size()==pool_size+1)best.erase(std::prev(best.end(),1));
	}
}

DeckResults beam_climb(unsigned num_min_iterations, unsigned num_iterations, std::vector<Deck*> your_decks, Process& proc, Requirement & requirement
#ifndef NQUEST
		, Quest & quest
#endif
		)
{
	if(pool_size==0){
			pool_size = min_beam_size;
	}

	//your_decks.size();
	//std::vector<std::pair<Deck*,FinalResults<long double>>> pool;

	//auto sort = [](FinalResults<long double> l,FinalResults<long double> r) {return l.points > r.points;};
	std::multimap<FinalResults<long double>, Deck*, std::greater<FinalResults<long double>>> best;
	Deck* cur_deck = proc.your_decks[0];


	EvaluatedResults zero_results = { EvaluatedResults::first_type(proc.enemy_decks.size()), 0 };

	std::unordered_map<std::string, EvaluatedResults> evaluated_decks{{cur_deck->hash(), zero_results}};
	EvaluatedResults& results = proc.evaluate(num_min_iterations, evaluated_decks.begin()->second);
	print_score_info(results, proc.factors);
	FinalResults<long double> best_score = compute_score(results, proc.factors); //init sim, todo remove

	unsigned deck_cost = get_deck_cost(cur_deck);
	fund = std::max(fund, deck_cost);
	print_deck_inline(deck_cost, best_score, cur_deck,true);
	std::mt19937& re = proc.threads_data[0]->re;
	unsigned cur_gap = check_requirement(cur_deck, requirement
#ifndef NQUEST
			, quest
#endif
			);

	unsigned long skipped_simulations = 0;

	bool is_random = (cur_deck->strategy == DeckStrategy::random) || (cur_deck->strategy == DeckStrategy::flexible);
	bool deck_has_been_improved = true;

	std::vector<const Card*> commander_candidates;
	std::vector<const Card*> alpha_dominion_candidates;
	std::vector<const Card*> card_candidates;

	auto mixed_candidates = get_candidate_lists(proc);
	commander_candidates = mixed_candidates.at(0);
	alpha_dominion_candidates = mixed_candidates.at(1);
	card_candidates = mixed_candidates.at(2);
	// add current alpha dominion to candidates if necessary
	// or setup first candidate into the deck if no alpha dominion defined
	if (use_owned_dominions)
	{
		for(auto i_deck : your_decks) //add all alpha doms
		{
			if (i_deck->alpha_dominion)
			{
				if (!std::count(alpha_dominion_candidates.begin(), alpha_dominion_candidates.end(),i_deck->alpha_dominion))
				{
					alpha_dominion_candidates.emplace_back(i_deck->alpha_dominion);
				}
			}
		}
		if (debug_print > 0)
		{
			for (const Card* dom_card : alpha_dominion_candidates)
			{
				std::cout << " ** next Alpha Dominion candidate: " << dom_card->m_name
					<< " ($: " << alpha_dominion_cost(dom_card) << ")" << std::endl;
			}
		}
	}

	//Deck* best_deck = cur_deck->clone();
	//FinalResults<long double> cur_score = best_score;
	//unsigned best_gap = cur_gap;



	//if(your_decks.size()>pool_size) your_decks.resize(pool_size);
	Deck * best_deck = cur_deck->clone();
	auto best_decks = your_decks;
	std::string best_hash = cur_deck->hash();
	unsigned from_slot;
	unsigned count_slot=0; //count iterations
	unsigned dead_slot=0; //last deck improvement
	unsigned mod_permute = 10*9*8*7*6*5*4*3*2*1;
	//FinalResults<long double> tmp_result;
	FinalResults<long double> nil{0, 0, 0, 0, 0, 0, num_min_iterations};
	//sim passed decks
	for(auto ii : your_decks){
			auto tdeck = ii->clone();
			copy_deck(tdeck,cur_deck);
			auto tscore = fitness(cur_deck,nil,evaluated_decks,zero_results,skipped_simulations,proc);
			if(!contains(best,cur_deck))best.insert(std::make_pair(tscore,tdeck));
			if(best.size()==pool_size+1)best.erase(std::prev(best.end(),1));
	}
	//fill remaining
	while(best.size()<pool_size){
		unsigned j = std::uniform_int_distribution<unsigned>(0,your_decks.size()-1)(re);
		unsigned i = std::uniform_int_distribution<unsigned>(0,your_decks.size()-1)(re);
		Deck* nxt = your_decks[j]->clone();
		mutate(your_decks[i],nxt,card_candidates,re,cur_gap,evaluated_decks
#ifndef NQUEST
					, quest
#endif
        ); //no crossovers here only fill with mutations
		copy_deck(nxt,cur_deck);
		auto tscore = fitness(cur_deck,nil,evaluated_decks,zero_results,skipped_simulations,proc);
		if(!contains(best,nxt)){best.insert(std::make_pair(tscore,nxt));}
	}

	while(true)
	{

#ifndef NDEBUG
		if (debug_print >= 0)
		{
			std::cout << "---------------BEST---------------" << std::endl;
			for (auto it = best.begin(); it != best.end();++it)
			{
				print_deck_inline(get_deck_cost(it->second),it->first,it->second);
			}
			std::cout << "---------------BEND---------------" << std::endl;
		}
#endif
		count_slot = (count_slot+1)%(mod_permute); //TODO Modulo here? % 10*9*8*7*6*5*4*3*2*1
		if(is_timeout_reached()){break;}
		if (deck_has_been_improved)
		{
			dead_slot = count_slot;
			deck_has_been_improved = false;
		}
		else if((dead_slot<count_slot && count_slot-dead_slot>=10 )||(count_slot<dead_slot && count_slot+mod_permute-dead_slot>=10 )) // nothing improved for 10 cycles
		{
			break; // done climbing
			//TODO climbex like beam climb here
		}

		// get new best decks:
		best_decks.clear();
		for(auto it = best.begin();it!=best.end();it++)
		{
			best_decks.push_back(it->second);
		}
		//sim deck
		for( auto i_deck :best_decks)
		{
			copy_deck(i_deck->clone(),cur_deck);
			best_deck = i_deck->clone();
			//copy_deck(i_deck,best_deck);
			best_score = fitness(cur_deck,nil,evaluated_decks,zero_results,skipped_simulations,proc); // grab from stored results or sim it
			from_slot = std::max(freezed_cards, (count_slot) % std::min<unsigned>(max_deck_len, best_deck->cards.size() + 1));
			//climb + save best ones to best

			// commander
		if (requirement.num_cards.count(best_deck->commander) == 0)
		{
			// << commander candidate loop >>
			for (const Card* commander_candidate: commander_candidates)
			{
				if (best_score.points - target_score > -1e-9)
				{ break; }
				if (commander_candidate == best_deck->commander)
				{continue;}
				//std::cout << "TRY CMD" << std::endl;
				if(try_improve_deck(cur_deck, -1, -1, commander_candidate,
						best_deck->commander, best_deck->alpha_dominion, best_deck->cards, best_score, cur_gap, best_hash,
						evaluated_decks, zero_results, skipped_simulations, proc,false
#ifndef NQUEST
			, quest
#endif
                        ))
						{
								check_and_update(best,cur_deck,deck_has_been_improved,best_score);
						}
			}
			copy_deck(best_deck,cur_deck);
		}

		// alpha dominion
		if (use_owned_dominions && !alpha_dominion_candidates.empty())
		{
			// << alpha dominion candidate loop >>
			for (const Card* alpha_dominion_candidate: alpha_dominion_candidates)
			{
				if (best_score.points - target_score > -1e-9)
				{ break; }
				if (alpha_dominion_candidate == best_deck->alpha_dominion)
				{ continue; }
				if(try_improve_deck(cur_deck, -1, -1, alpha_dominion_candidate,
						best_deck->commander, best_deck->alpha_dominion, best_deck->cards, best_score, cur_gap, best_hash,
						evaluated_decks, zero_results, skipped_simulations, proc,false
#ifndef NQUEST
			, quest
#endif
                        ))
						{
								check_and_update(best,cur_deck,deck_has_been_improved,best_score);
						}
			}
			copy_deck(best_deck,cur_deck);
		}

		// shuffle candidates
		std::shuffle(card_candidates.begin(), card_candidates.end(), re);

		// << card candidate loop >>
		//for (const Card* card_candidate: card_candidates)
		for (auto it = card_candidates.begin(); it != card_candidates.end();++it)
		{
			const Card* card_candidate = *it;
			for (unsigned to_slot(is_random ? from_slot : card_candidate ? freezed_cards : (best_deck->cards.size() - 1));
					to_slot < (is_random ? (from_slot + 1) : (best_deck->cards.size() + (from_slot < best_deck->cards.size() ? 0 : 1)));
					++ to_slot)
			{
				if (card_candidate ?
						(from_slot < best_deck->cards.size() && (from_slot == to_slot && card_candidate == best_deck->cards[to_slot])) // 2 Omega -> 2 Omega
						:
						(from_slot == best_deck->cards.size())) // void -> void
				{ continue; }

				//std::cout << "TRY" << std::endl;
				//print_deck_inline(get_deck_cost(best_deck),best_score,best_deck);
				if(try_improve_deck(cur_deck, from_slot, to_slot, card_candidate,
						best_deck->commander, best_deck->alpha_dominion, best_deck->cards, best_score, cur_gap, best_hash,
						evaluated_decks, zero_results, skipped_simulations, proc,false
#ifndef NQUEST
			, quest
#endif
                        ))
						{
								check_and_update(best,cur_deck,deck_has_been_improved,best_score);
						}
				//print_deck_inline(get_deck_cost(best_deck),best_score,best_deck);
			}
			if (best_score.points - target_score > -1e-9)
			{ break; }

			}
		}
}

	best_deck = best.begin()->second;
	best_score = best.begin()->first;

	unsigned simulations = 0;
	for (auto evaluation: evaluated_decks)
	{ simulations += evaluation.second.second; }
	std::cout << "Evaluated " << evaluated_decks.size() << " decks (" << simulations << " + " << skipped_simulations << " simulations)." << std::endl;
	print_sim_card_values(best_deck,proc,num_iterations);
	std::cout << "Optimized Deck: ";
	print_deck_inline(get_deck_cost(best_deck), best_score, best_deck,true);
	print_upgraded_cards(best_deck);
	return std::make_pair(best_deck->clone(),best_score);
}
