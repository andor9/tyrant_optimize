#include "algorithms_util.h"

DeckResults hill_climbing(unsigned num_min_iterations, unsigned num_iterations, std::vector<Deck*> your_decks , Process& proc, Requirement & requirement
#ifndef NQUEST
		, Quest & quest
#endif
);


DeckResults simulated_annealing(unsigned num_min_iterations, unsigned num_iterations, std::vector<Deck*> your_decks, Process& proc, Requirement & requirement
#ifndef NQUEST
		, Quest & quest
#endif
);

DeckResults genetic_algorithm(unsigned num_min_iterations, unsigned num_iterations, std::vector<Deck*> your_decks, Process& proc, Requirement & requirement
#ifndef NQUEST
		, Quest & quest
#endif
);

DeckResults forts_climbing(unsigned num_iterations, Process& proc);

DeckResults beam_climb(unsigned num_min_iterations, unsigned num_iterations, std::vector<Deck*> your_decks, Process& proc, Requirement & requirement
#ifndef NQUEST
		, Quest & quest
#endif
);
