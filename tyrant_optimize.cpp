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
// #define NDEBUG
#define BOOST_THREAD_USE_LIB

#include <unistd.h>
#include <array>
#include <cassert>
#include <chrono>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <unordered_map>
#include <set>
#include <stack>
#include <fstream>
#include <string>
#include <iomanip>
#include <tuple>
#include <vector>
#include <math.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/math/distributions/binomial.hpp>
#include <boost/optional.hpp>
#include <boost/range/join.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/timer/timer.hpp>
#include <boost/tokenizer.hpp>
#include "card.h"
#include "cards.h"
#include "deck.h"
#include "read.h"
#include "sim.h"
#include "tyrant.h"
#include "xml.h"

#include "hPMML.h"

#define DEFINE_GLOBALS
#include "algorithms.h"

// OpenMP Header
#ifdef _OPENMP
#include <omp.h>
#endif

// Android Headers
#if defined(ANDROID) || defined(__ANDROID__)
#include <jni.h>
#include <android/log.h>
#endif

using namespace tuo;
using namespace proc;
// init
/*
namespace tuo {
  unsigned opt_num_threads=4;
    gamemode_t gamemode{fight};
    OptimizationMode optimization_mode{OptimizationMode::notset};
    std::unordered_map<unsigned, unsigned> owned_cards;
    const Card* owned_alpha_dominion{nullptr};
    bool use_owned_cards{true};
    bool opt_skip_unclaimed_decks{false};
    //bool opt_trim_unclaimed_decks{false};
    unsigned min_deck_len{1};
    unsigned max_deck_len{10};
    unsigned opt_freezed_cards{0};
    unsigned freezed_cards{0};
    unsigned fund{0};
    long double target_score{100};
    long double min_increment_of_score{0};
    long double confidence_level{0.99};
    bool use_top_level_card{true};
    bool use_top_level_commander{true};
    bool mode_open_the_deck{false};
    bool use_owned_dominions{true};
    bool use_maxed_dominions{false};
    unsigned use_fused_card_level{0};
    unsigned use_fused_commander_level{0};
    bool print_upgraded{false};
    bool print_values{false};
    bool simplify_output{false};
    bool show_ci{false};
    bool use_harmonic_mean{false};
    unsigned iterations_multiplier{10};
    unsigned sim_seed{0};
    unsigned flexible_iter{20};
    unsigned flexible_turn{10};
    Requirement requirement;
#ifndef NQUEST
    Quest quest;
#endif
    std::unordered_set<unsigned> allowed_candidates;
    std::unordered_set<unsigned> disallowed_candidates;
    std::chrono::time_point<std::chrono::system_clock> start_time;
    long double maximum_time{0};
    //anneal
    long double temperature = 1000;
    long double coolingRate = 0.001;
    //genetic
    unsigned generations = 50;
    unsigned pool_size = 0;
    unsigned min_pool_size = 20;
    double opt_pool_keep = 1;
    double opt_pool_mutate = 1;
    double opt_pool_cross = 1;
    //fort_climb
    unsigned yfpool{0};
    unsigned efpool{0};
    std::vector<Faction> factions;
    bool invert_factions{false};
    bool only_recent{false};
    bool prefered_recent{false};
    unsigned recent_percent{5};
    std::vector<Skill::Skill> skills;
    bool invert_skills{false};
    std::vector<Skill::Skill> prefered_skills;
    unsigned prefered_factor{3};

#if defined(ANDROID) || defined(__ANDROID__)
    JNIEnv *envv;
    jobject objv;
#endif
}
*/

static const unsigned int crc32_table[] =
    {
        0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
        0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
        0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
        0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
        0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
        0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
        0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
        0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
        0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
        0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
        0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
        0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
        0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
        0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
        0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
        0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
        0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
        0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
        0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
        0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
        0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
        0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
        0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
        0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
        0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
        0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
        0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
        0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
        0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
        0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
        0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
        0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
        0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
        0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
        0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
        0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
        0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
        0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
        0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
        0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
        0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
        0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
        0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
        0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
        0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
        0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
        0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
        0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
        0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
        0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
        0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
        0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
        0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
        0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
        0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
        0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
        0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
        0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
        0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
        0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
        0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
        0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
        0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
        0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4};

unsigned int
xcrc32(const unsigned char *buf, int len, unsigned int init)
{
    unsigned int crc = init;
    while (len--)
    {
        crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *buf) & 255];
        buf++;
    }
    return crc;
}

unsigned int file_xrcrc32(std::string filename, unsigned int init)
{
    std::ifstream file;
    file.open(filename);
    std::string line;
    unsigned int crc = init;
    while (getline(file, line))
    {
        crc = xcrc32((const unsigned char *)line.c_str(), line.length(), crc);
    }
    return crc;
}

unsigned int checksumcards(std::string prefix)
{
    unsigned int crc = 0;
    // loop cardsections
    unsigned int ii = 1;

    crc = file_xrcrc32(prefix + "data/raids.xml", crc);
    crc = file_xrcrc32(prefix + "data/missions.xml", crc);
    std::string fname = prefix + "data/cards_section_1.xml";
    while (access(fname.c_str(), F_OK) == 0)
    {
        crc = file_xrcrc32(fname, crc);
        ii += 1;
        fname = prefix + "data/cards_section_" + std::to_string(ii) + ".xml";
    }
    return crc;
}

void load_ml(std::string prefix)
{
    if (use_ml)
    {
        win_model = hpmml::Model(prefix + "data/win.pmml");
        stall_model = hpmml::Model(prefix + "data/stall.pmml");
        loss_model = hpmml::Model(prefix + "data/loss.pmml");
        points_model = hpmml::Model(prefix + "data/points.pmml");
    }
}

// load database map from file
void load_db(std::string prefix)
{
    if (use_db_load)
    {
        // open file to read from
        std::ifstream file;
        file.open(prefix + "data/database.yml");
        // read map from file
        std::string name;
        uint64_t wins, draws, losses, points, count;
        std::string version, check;
        std::string line;
        getline(file, line);
        if (line.find(":") != std::string::npos)
            version = line.substr(line.find(":") + 2);
        std::cout << "DB version " << version << std::endl;
        getline(file, line);
        if (line.find(":") != std::string::npos)
            check = line.substr(line.find(":") + 2);
        std::cout << "DB checksum " << check << std::endl;

        if (use_strict_db)
        {
            if (version.compare(TYRANT_OPTIMIZER_VERSION) != 0)
            {
                std::cout << "TUO DB version, please delete database.yml and restart" << std::endl;
                std::cout << "TUO DB version " << version << " != " << TYRANT_OPTIMIZER_VERSION << std::endl;
                exit(1);
            }
            if (check.compare(std::to_string(checksumcards(prefix))) != 0)
            {
                std::cout << "cards_sections mismatch to db, please delete database.yml and restart" << std::endl;
                std::cout << "cards_sections checksum " << checksumcards(prefix) << " != " << check << std::endl;
                exit(1);
            }
        }

        std::string hfield;
        std::string hydeck;
        std::string hedeck;
        while (getline(file, line))
        {
            if (line.rfind("\t\t", 0) == 0 && line.find(":") != std::string::npos)
            {
                hedeck = line.substr(2, line.find(":") - 2);
                std::istringstream reader(line.substr(line.find(":") + 2));
                reader >> wins >> draws >> losses >> points >> count;
                database[hfield][hydeck][hedeck] = {wins, draws, losses, points, count};
                // std::cout << "load db: " << hfield << " " << hydeck << " " << hedeck << " " <<wins<< " "<< draws<< " "<< losses<< " " <<points<< " " <<count << std::endl;
            }
            else if (line.rfind("\t", 0) == 0 && line.find(":") != std::string::npos)
            {
                hydeck = line.substr(1, line.find(":") - 1);
            }
            else if (line.find(":") != std::string::npos)
            {
                hfield = line.substr(0, line.find(":"));
            }
            else
            {
                std::cout << "unknown db line: " << line;
            }
        }
        // close file
        file.close();
    }
}

// write database map to file
void write_db(std::string prefix)
{
    if (use_db_write)
    {
        // open file to write to
        std::ofstream file;
        file.open(prefix + "data/database.yml");
        file << "version: " << TYRANT_OPTIMIZER_VERSION << std::endl;

        file << "xml_check_sum: " << checksumcards(prefix) << std::endl;
        auto lines_to_write = db_limit;
        // write map to file
        for (auto it1 = database.begin(); lines_to_write != 0 && it1 != database.end(); ++it1)
        {
            file << it1->first << ":" << std::endl;
            for (auto it2 = it1->second.begin(); lines_to_write != 0 && it2 != it1->second.end(); ++it2)
            {
                file << "\t" << it2->first << ":" << std::endl;
                for (auto it3 = it2->second.begin(); lines_to_write != 0 && it3 != it2->second.end(); ++it3)
                {
                    file << "\t\t" << it3->first << ": " << it3->second.wins << " " << it3->second.draws << " " << it3->second.losses << " " << it3->second.points << " " << it3->second.count << std::endl;
                    if (lines_to_write > 0)
                        lines_to_write--;
                }
            }
        }
        // close file
        file.close();
    }
}

void init()
{
    thread_num_iterations = 0; // written by threads
    thread_results = nullptr;  // written by threads
    thread_best_results = nullptr;
    thread_compare = false;
    thread_compare_stop = false; // written by threads
    destroy_threads;
    opt_num_threads = 4;
    gamemode = fight;
    optimization_mode = OptimizationMode::notset;
    owned_cards.clear();
    owned_alpha_dominion = nullptr;
    use_owned_cards = true;
    opt_skip_unclaimed_decks = false;
    // opt_trim_unclaimed_decks=false;
    min_deck_len = 1;
    max_deck_len = 10;
    opt_freezed_cards = 0;
    freezed_cards = 0;
    fund = 0;
    target_score = 100;
    min_increment_of_score = 0;
    confidence_level = 0.99;
    use_top_level_card = true;
    use_top_level_commander = true;
    mode_open_the_deck = false;
    use_owned_dominions = true;
    use_maxed_dominions = false;
    use_fused_card_level = 0;
    use_fused_commander_level = 0;
    print_upgraded = false;
    print_values = false;
    vc_x = 0;
    simplify_output = false;
    show_ci = false;
    use_harmonic_mean = false;
    iterations_multiplier = 10;
    sim_seed = 0;
    flexible_iter = 20;
    flexible_turn = 20;
    eval_iter = 8;
    eval_turn = 8;
    requirement.num_cards.clear();
    for (unsigned i(0); i < Skill::num_skills; ++i)
    {
        auto s = static_cast<Skill::Skill>(i);
        x_skill_scale[s] = 1.0;
        n_skill_scale[s] = 1.0;
        c_skill_scale[s] = 1.0;
    }
    hp_scale = 1.0;
    atk_scale = 1.0;
#ifndef NQUEST
    // quest = new Quest(); //TODO Quest bugged in Android now here
#endif
	allowed_candidates.clear();
	disallowed_candidates.clear();
	disallowed_recipes.clear();

	//std::chrono::time_point<std::chrono::system_clock> start_time;
	maximum_time=0;
	temperature = 1000;
	coolingRate = 0.001;

	generations = 50;
	pool_size = 0;
	min_pool_size = 20;
	opt_pool_keep = 1;
	opt_pool_mutate = 1;
	opt_pool_cross = 1;

	min_beam_size = 5;

	yfpool=0;
	efpool=0;

	factions.clear();
	invert_factions=false;
	only_recent=false;
	prefered_recent=false;
	recent_percent=5;
	skills.clear();
	invert_skills=false;
	prefered_skills.clear();
	prefered_factor=3;

	for(Card *c : all_cards.all_cards) delete c; // prevent memory leak
	all_cards.visible_cardset.clear();


	//fix defaults
	for (int i=0; i < Fix::num_fixes;++i) fixes[i]=false;

    //recommended/default fixes
	fixes[Fix::enhance_early] = true;
	fixes[Fix::revenge_on_death] = true;
	fixes[Fix::death_from_bge] = true;
	fixes[Fix::legion_under_mega] = true;

	fixes[Fix::barrier_each_turn] = true;
	fixes[Fix::dont_evade_mimic_selection] = true;
	fixes[Fix::leech_increase_max_hp] = true;
	fixes[Fix::subdue_before_attack] = true;

	fixes[Fix::counter_without_damage] = false;
	fixes[Fix::corrosive_protect_armor] = false;
	fixes[Fix::poison_after_attacked] = false;
  
  
    db_limit = -1;
    use_strict_db = false;
    use_db_write = true;
    use_db_load = true;

    use_ml = false;
    use_only_ml = false;
    ml_precision = 0.01;
}

#if defined(ANDROID) || defined(__ANDROID__)
extern "C" JNIEXPORT void

    JNICALL
    Java_de_neuwirthinformatik_alexander_mtuo_TUO_callMain(
        JNIEnv *env,
        jobject obj /* this */, jobjectArray stringArray)
{
    envv = env;
    objv = obj;
    // from: https://stackoverflow.com/questions/8870174/is-stdcout-usable-in-android-ndk and https://gist.github.com/dzhioev/6127982
    class androidbuf : public std::streambuf
    {
    public:
        enum
        {
            bufsize = 256
        }; // ... or some other suitable buffer size
        androidbuf() { this->setp(buffer, buffer + bufsize - 1); }

    private:
        int overflow(int c)
        {
            if (c == traits_type::eof())
            {
                *this->pptr() = traits_type::to_char_type(c);
                this->sbumpc();
            }
            return this->sync() ? traits_type::eof() : traits_type::not_eof(c);
        }
        int sync()
        {
            int rc = 0;
            if (this->pbase() != this->pptr())
            {
                auto sss = std::string(this->pbase(),
                                       this->pptr() - this->pbase())
                               .c_str();
                __android_log_print(ANDROID_LOG_DEBUG,
                                    "TUO_TUO",
                                    "%s",
                                    sss);
                jstring jstr = envv->NewStringUTF(sss);
                jclass clazz = envv->FindClass("de/neuwirthinformatik/alexander/mtuo/TUO");
                jmethodID messageMe = envv->GetMethodID(clazz, "output", "(Ljava/lang/String;)V");
                envv->CallVoidMethod(objv, messageMe, jstr);
                rc = 0;
                this->setp(buffer, buffer + bufsize - 1);
            }
            return rc;
        }
        char buffer[bufsize];
    };
    std::cout.rdbuf(new androidbuf);
    std::cerr.rdbuf(new androidbuf);
    __android_log_write(ANDROID_LOG_DEBUG, "TUO_TUO", "START");
    int stringCount = env->GetArrayLength(stringArray);
    char **param = new char *[stringCount];
    const char **cparam = new const char *[stringCount];
    jstring *strs = new jstring[stringCount];
    for (int i = 0; i < stringCount; i++)
    {
        strs[i] = (jstring)(*env).GetObjectArrayElement(stringArray, i);
        cparam[i] = ((*env).GetStringUTFChars(strs[i], NULL));
        param[i] = const_cast<char *>(cparam[i]);
    }

    main(stringCount, cparam);
    std::cout << std::flush;
    __android_log_write(ANDROID_LOG_DEBUG, "TUO_TUO", "END");

    for (int i = 0; i < stringCount; i++)
    {
        env->ReleaseStringUTFChars(strs[i], cparam[i]);
    }
    // std::string text = "return";
    // return env->NewStringUTF(text.c_str());
}

extern "C" JNIEXPORT jstring

    JNICALL
    Java_de_neuwirthinformatik_alexander_mtuo_TUO_stringFromJNI(JNIEnv *env,
                                                                jobject thiz, jstring s)
{
    std::string str = env->GetStringUTFChars(s, NULL);
    str += "hello.txt";
    __android_log_write(ANDROID_LOG_DEBUG, "TUO_TUO", str.c_str());
    FILE *file = fopen(str.c_str(), "w+");

    if (file != NULL)
    {
        fputs("HELLO WORLD!\n", file);
        fflush(file);
        fclose(file);
    }

    return env->NewStringUTF("Hello from JNI (with file io)!");
}
#endif
using namespace std::placeholders;

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
Deck *find_deck(Decks &decks, const Cards &all_cards, std::string deck_name)
{
    Deck *deck = decks.find_deck_by_name(deck_name);
    if (deck != nullptr)
    {
        deck->resolve();
        return (deck);
    }
    decks.decks.emplace_back(Deck{all_cards});
    deck = &decks.decks.back();
    deck->set(deck_name);
    deck->resolve();
    return (deck);
}
//---------------------- $80 deck optimization ---------------------------------

unsigned get_required_cards_before_upgrade(std::unordered_map<unsigned, unsigned> &owned_cards,
                                           const std::vector<const Card *> &card_list, std::map<const Card *, unsigned> &num_cards)
{
    unsigned deck_cost = 0;
    std::set<const Card *> unresolved_cards;
    for (const Card *card : card_list)
    {
        ++num_cards[card];
        unresolved_cards.insert(card);
    }
    // un-upgrade according to type/category
    // * use fund for normal cards
    // * use only top-level cards for initial (basic) dominion (Alpha Dominion) and dominion material (Dominion Shard)
    while (!unresolved_cards.empty())
    {
        // pop next unresolved card
        auto card_it = unresolved_cards.end();
        auto card = *(--card_it);
        unresolved_cards.erase(card_it);

        // assume unlimited common/rare level-1 cards (standard set)
        if ((card->m_set == 1000) && (card->m_rarity <= 2) && (card->is_low_level_card()))
        {
            continue;
        }

        // keep un-defused (top-level) basic dominion & its material
        if ((card->m_id == 50002) || (card->m_category == CardCategory::dominion_material))
        {
            continue;
        }

        // defuse if inventory lacks required cards and recipe is not empty
        if ((fund || (card->m_category != CardCategory::normal)) && (owned_cards[card->m_id] < num_cards[card]) && !card->m_recipe_cards.empty())
        {
            unsigned num_under = num_cards[card] - owned_cards[card->m_id];
            num_cards[card] = owned_cards[card->m_id];

            // do count cost (in SP) only for normal cards
            if (card->m_category == CardCategory::normal)
            {
                deck_cost += num_under * card->m_recipe_cost;
            }

            // enqueue recipe cards as unresolved
            for (auto recipe_it : card->m_recipe_cards)
            {
                num_cards[recipe_it.first] += num_under * recipe_it.second;
                unresolved_cards.insert(recipe_it.first);
            }
        }
    }
    return deck_cost;
}

inline unsigned get_required_cards_before_upgrade(const std::vector<const Card *> &card_list, std::map<const Card *, unsigned> &num_cards)
{
    return get_required_cards_before_upgrade(owned_cards, card_list, num_cards);
}

unsigned get_deck_cost(const Deck *deck)
{
    if (!use_owned_cards)
    {
        return 0;
    }
    std::map<const Card *, unsigned> num_in_deck;
    unsigned deck_cost = 0;
    if (deck->commander)
    {
        deck_cost += get_required_cards_before_upgrade({deck->commander}, num_in_deck);
    }
    deck_cost += get_required_cards_before_upgrade(deck->cards, num_in_deck);
    for (auto it : num_in_deck)
    {
        unsigned card_id = it.first->m_id;
        if (it.second > owned_cards[card_id])
        {
            return UINT_MAX;
        }
    }
    return deck_cost;
}

bool is_in_recipe(const Card *card, const Card *material)
{
    // is it already material?
    if (card == material)
    {
        return true;
    }

    // no recipes
    if (card->m_recipe_cards.empty())
    {
        return false;
    }

    // avoid illegal
    if (card->m_category == CardCategory::dominion_material)
    {
        return false;
    }

    // check recursively
    for (auto recipe_it : card->m_recipe_cards)
    {
        // is material found?
        if (recipe_it.first == material)
        {
            return true;
        }

        // go deeper ...
        if (is_in_recipe(recipe_it.first, material))
        {
            return true;
        }
    }

    // found nothing
    return false;
}

bool is_owned_or_can_be_fused(const Card *card)
{
    if (owned_cards[card->m_id])
    {
        return true;
    }
    if (!fund && (card->m_category == CardCategory::normal))
    {
        return false;
    }
    std::map<const Card *, unsigned> num_in_deck;
    unsigned deck_cost = get_required_cards_before_upgrade({card}, num_in_deck);
    if ((card->m_category == CardCategory::normal) && (deck_cost > fund))
    {
        while (!card->is_low_level_card() && (deck_cost > fund))
        {
            card = card->downgraded();
            num_in_deck.clear();
            deck_cost = get_required_cards_before_upgrade({card}, num_in_deck);
        }
        if (deck_cost > fund)
        {
            return false;
        }
    }
    std::map<unsigned, unsigned> num_under;
    for (auto it : num_in_deck)
    {
        if (it.second > owned_cards[it.first->m_id])
        {
            if ((card->m_category == CardCategory::dominion_alpha) && use_maxed_dominions && !is_in_recipe(card, owned_alpha_dominion))
            {
                if (it.first->m_id != 50002)
                {
                    num_under[it.first->m_id] += it.second - owned_cards[it.first->m_id];
                }
                continue;
            }
            return false;
        }
    }
    if (!num_under.empty())
    {
        std::map<const Card *, unsigned> &refund = dominion_refund[owned_alpha_dominion->m_fusion_level][owned_alpha_dominion->m_level];
        for (auto &refund_it : refund)
        {
            unsigned refund_id = refund_it.first->m_id;
            if (!num_under.count(refund_id))
            {
                continue;
            }
            num_under[refund_id] = safe_minus(num_under[refund_id], refund_it.second);
            if (!num_under[refund_id])
            {
                num_under.erase(refund_id);
            }
        }
    }
    return num_under.empty();
}

std::string alpha_dominion_cost(const Card *dom_card)
{
    assert(dom_card->m_category == CardCategory::dominion_alpha);
    if (!owned_alpha_dominion)
    {
        return "(no owned alpha dominion)";
    }
    std::unordered_map<unsigned, unsigned> _owned_cards;
    std::unordered_map<unsigned, unsigned> refund_owned_cards;
    std::map<const Card *, unsigned> num_cards;
    std::map<const Card *, unsigned> &refund = dominion_refund[owned_alpha_dominion->m_fusion_level][owned_alpha_dominion->m_level];
    unsigned own_dom_id = 50002;
    if (is_in_recipe(dom_card, owned_alpha_dominion))
    {
        own_dom_id = owned_alpha_dominion->m_id;
    }
    else if (owned_alpha_dominion->m_id != 50002)
    {
        for (auto &it : refund)
        {
            if (it.first->m_category != CardCategory::dominion_material)
            {
                continue;
            }
            refund_owned_cards[it.first->m_id] += it.second;
        }
    }
    _owned_cards[own_dom_id] = 1;
    get_required_cards_before_upgrade(_owned_cards, {dom_card}, num_cards);
    std::string value("");
    for (auto &it : num_cards)
    {
        if (it.first->m_category != CardCategory::dominion_material)
        {
            continue;
        }
        value += it.first->m_name + " x " + std::to_string(it.second) + ", ";
    }
    if (!is_in_recipe(dom_card, owned_alpha_dominion))
    {
        num_cards.clear();
        get_required_cards_before_upgrade(_owned_cards, {owned_alpha_dominion}, num_cards);
        value += "using refund: ";
        for (auto &it : refund)
        {
            signed num_under(it.second - (signed)num_cards[it.first]);
            value += it.first->m_name + " x " + std::to_string(it.second) + "/" + std::to_string(num_under) + ", ";
        }
    }
    // remove trailing ', ' for non-empty string / replace empty by '(none)'
    if (!value.empty())
    {
        value.erase(value.end() - 2, value.end());
    }
    else
    {
        value += "(none)";
    }
    return value;
}

// check if claim_cards is necessary => i.e. can the deck be build from the ownedcards
bool claim_cards_needed(const std::vector<const Card *> &card_list)
{
    std::map<const Card *, unsigned> num_cards;
    get_required_cards_before_upgrade(card_list, num_cards);
    for (const auto &it : num_cards)
    {
        const Card *card = it.first;
        if (card->m_category == CardCategory::dominion_material && use_maxed_dominions)
            continue;
        if (card->m_category == CardCategory::dominion_alpha && use_maxed_dominions)
            continue;
        unsigned num_to_claim = safe_minus(it.second, owned_cards[card->m_id]);
        if (num_to_claim > 0)
        {
            return true;
        }
    }
    return false;
}

void claim_cards(const std::vector<const Card *> &card_list)
{
    std::map<const Card *, unsigned> num_cards;
    get_required_cards_before_upgrade(card_list, num_cards);
    for (const auto &it : num_cards)
    {
        const Card *card = it.first;
        if (card->m_category == CardCategory::dominion_material)
            continue;
        if (card->m_category == CardCategory::dominion_alpha)
            continue;
        unsigned num_to_claim = safe_minus(it.second, owned_cards[card->m_id]);
        if (num_to_claim > 0)
        {
            owned_cards[card->m_id] += num_to_claim;
            if (debug_print >= 0)
            {
                std::cerr << "WARNING: Need extra " << num_to_claim << " " << card->m_name << " to build your initial deck: adding to owned card list.\n";
            }
        }
    }
}

bool valid_deck(Deck *your_deck)
{
    if (claim_cards_needed({your_deck->commander}))
        return false;
    if (claim_cards_needed(your_deck->cards))
        return false;
    if (your_deck->alpha_dominion && claim_cards_needed({your_deck->alpha_dominion}))
        return false;
    return true; // valid
}
//------------------------------------------------------------------------------
FinalResults<long double> compute_score(const EvaluatedResults &results, std::vector<long double> &factors)
{
    FinalResults<long double> final{0, 0, 0, 0, 0, 0, results.second};
    long double max_possible = max_possible_score[(size_t)optimization_mode];
    for (unsigned index(0); index < results.first.size(); ++index)
    {
        // std::cout << results.second << " " << results.first[index].points << " " << results.first[index].count << std::endl;
        final.wins += results.first[index].wins * factors[index] * results.second / results.first[index].count;
        final.draws += results.first[index].draws * factors[index] * results.second / results.first[index].count;
        final.losses += results.first[index].losses * factors[index] * results.second / results.first[index].count;
        // APN
        auto trials = results.second;
        auto prob = 1 - confidence_level;
        auto successes = results.first[index].points * results.second / results.first[index].count / max_possible;
        if (successes > trials)
        {
            successes = trials;
            // printf("WARNING: biominal successes > trials");
            _DEBUG_MSG(2, "WARNING: biominal successes > trials");
        }

        auto lower_bound = boost::math::binomial_distribution<>::find_lower_bound_on_p(trials, successes, prob) * max_possible;
        auto upper_bound = boost::math::binomial_distribution<>::find_upper_bound_on_p(trials, successes, prob) * max_possible;
        if (use_harmonic_mean)
        {
            final.points += factors[index] / (results.first[index].points * results.second / results.first[index].count);
            final.points_lower_bound += factors[index] / lower_bound;
            final.points_upper_bound += factors[index] / upper_bound;
        }
        else
        {
            final.points += results.first[index].points * factors[index] * results.second / results.first[index].count;
            final.points_lower_bound += lower_bound * factors[index];
            final.points_upper_bound += upper_bound * factors[index];
        }
    }
    long double factor_sum = std::accumulate(factors.begin(), factors.end(), 0.);
    final.wins /= factor_sum * (long double)results.second;
    final.draws /= factor_sum * (long double)results.second;
    final.losses /= factor_sum * (long double)results.second;
    if (use_harmonic_mean)
    {
        final.points = factor_sum / ((long double)results.second * final.points);
        final.points_lower_bound = factor_sum / final.points_lower_bound;
        final.points_upper_bound = factor_sum / final.points_upper_bound;
    }
    else
    {
        final.points /= factor_sum * (long double)results.second;
        final.points_lower_bound /= factor_sum;
        final.points_upper_bound /= factor_sum;
    }
    return final;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Per thread data.
// seed should be unique for each thread.
// d1 and d2 are intended to point to read-only process-wide data.

void SimulationData::set_decks(std::vector<Deck *> const your_decks_, std::vector<Deck *> const &enemy_decks_)
{
    for (unsigned i(0); i < your_decks_.size(); ++i)
    {
        your_decks[i].reset(your_decks_[i]->clone());
        your_hands[i]->deck = your_decks[i].get();
    }
    for (unsigned i(0); i < enemy_decks_.size(); ++i)
    {
        enemy_decks[i].reset(enemy_decks_[i]->clone());
        enemy_hands[i]->deck = enemy_decks[i].get();
    }
}

std::vector<std::unordered_map<std::string, std::string>> Process::samples()
{
    std::vector<std::unordered_map<std::string, std::string>> samples;

    int i = 0;
    // loop over partial ids and prefix with f add to fmap
    auto ids = partial_ids();
    for (auto const &ydeck : your_decks)
    {
        auto yids = ydeck->sorted_ids();
        for (auto const &edeck : enemy_decks)
        {
            auto eids = edeck->sorted_ids();
            std::unordered_map<std::string, std::string> fmap;
            i = 0;
            for (auto const &id : ids)
            {
                fmap.emplace("f" + std::to_string(i), std::to_string(id));
                i++;
            }
            i = 0;
            for (auto const &id : yids)
            {
                fmap.emplace("y" + std::to_string(i), std::to_string(id->m_id));
                i++;
            }
            // fill up with zeros up to id 12
            while (i < 12)
            {
                fmap.emplace("y" + std::to_string(i), std::to_string(0));
                i++;
            }
            i = 0;
            for (auto const &id : eids)
            {
                fmap.emplace("e" + std::to_string(i), std::to_string(id->m_id));
                i++;
            }
            // fill up with zeros up to id 12
            while (i < 12)
            {
                fmap.emplace("e" + std::to_string(i), std::to_string(0));
                i++;
            }
            samples.emplace_back(fmap);
        }
    }
    /*
    auto myMap = samples.at(0);
    for(auto it = myMap.begin(); it != myMap.end(); ++it)
    {
        std::cout << it->first << " " << it->second  <<"\n";
    }
    */

    return samples;
}

std::vector<std::array<std::string, 3>> Process::hashes()
{
    std::vector<std::array<std::string, 3>> hashes;
    hashes.reserve(your_decks.size() * enemy_decks.size());
    std::string hash = partial_hash();
    for (auto const &ydeck : your_decks)
    {
        for (auto const &edeck : enemy_decks)
        {
            hashes.emplace_back(std::array<std::string, 3>{hash, ydeck->hash(), edeck->hash()});
        }
    }
    return hashes;
}

std::vector<unsigned int> Process::partial_ids()
{
    std::vector<unsigned int> ids;
    ids.emplace_back(gamemode);
    ids.emplace_back(optimization_mode);
    for (int i = 0; i < Fix::num_fixes; ++i)
    {
        ids.emplace_back(fixes[i]);
    }
    ids.emplace_back(flexible_iter);
    ids.emplace_back(flexible_turn);
    ids.emplace_back(eval_iter);
    ids.emplace_back(eval_turn);
    for (int i = 0; i < PassiveBGE::num_passive_bges; ++i)
        ids.emplace_back(your_bg_effects[i]);
    for (int i = 0; i < PassiveBGE::num_passive_bges; ++i)
        ids.emplace_back(enemy_bg_effects[i]);
    if (your_bg_skills.size() > 0)
    {
        auto bge = your_bg_skills[0];
        ids.emplace_back(bge.id);
        ids.emplace_back(bge.x);
        ids.emplace_back(bge.y);
        ids.emplace_back(bge.n);
        ids.emplace_back(bge.c);
        ids.emplace_back(bge.s);
        ids.emplace_back(bge.s2);
        ids.emplace_back(bge.all);
        ids.emplace_back(bge.card_id);
    }
    else
    {
        // emplace back 9 zeros
        for (int i = 0; i < 9; ++i)
            ids.emplace_back(0);
    }
    if (enemy_bg_skills.size() > 0)
    {
        auto bge = enemy_bg_skills[0];
        ids.emplace_back(bge.id);
        ids.emplace_back(bge.x);
        ids.emplace_back(bge.y);
        ids.emplace_back(bge.n);
        ids.emplace_back(bge.c);
        ids.emplace_back(bge.s);
        ids.emplace_back(bge.s2);
        ids.emplace_back(bge.all);
        ids.emplace_back(bge.card_id);
    }
    else
    {
        // emplace back 9 zeros
        for (int i = 0; i < 9; ++i)
            ids.emplace_back(0);
    }

    return ids;
}

// Compute field hash from two decks
std::string Process::partial_hash()
{
    // TODO fail if quest enabled since, db caching is incompatible with quest mode
    std::stringstream ios;
    for (auto id : partial_ids())
        encode_id_ext_b64(ios, id);

    return ios.str();
}

inline std::vector<Results<uint64_t>> SimulationData::evaluate(const std::vector<bool> &deck_mask)
{
    std::vector<Results<uint64_t>> res;
    res.reserve(enemy_hands.size() * your_hands.size());
    assert(deck_mask.size() == your_hands.size() * enemy_hands.size());
    int i = 0;
    for (Hand *your_hand : your_hands)
    {
        for (Hand *enemy_hand : enemy_hands)
        {
            // TUO5 added mask to already computed decks from db
            if (deck_mask[i] != 0)
            {
                your_hand->reset(re);
                enemy_hand->reset(re);
                Field fd(re, cards, *your_hand, *enemy_hand, gamemode, optimization_mode,
#ifndef NQUEST
                         quest,
#endif
                         your_bg_effects, enemy_bg_effects, your_bg_skills, enemy_bg_skills, fixes, flexible_iter, flexible_turn, eval_iter, eval_turn);
                Results<uint64_t> result(play(&fd));
                if (__builtin_expect(mode_open_the_deck, false))
                {
                    // are there remaining (unopened) cards?
                    if (fd.players[1]->deck->shuffled_cards.size())
                    {
                        // apply min score (there are unopened cards, so mission failed)
                        result.points = min_possible_score[(size_t)optimization_mode];
                    }
                }
                res.emplace_back(result);
            }
            else
            {
                res.emplace_back(Results<uint64_t>()); // no change since masked
            }
            i++;
        }
    }
    // std::cout << std::endl<<  "Deck hash: " << your_hand.deck->hash() << "#"<< std::endl;
    return (res);
}

inline std::vector<Results<uint64_t>> SimulationData::evaluate()
{
    return evaluate(std::vector<bool>(your_hands.size() * enemy_hands.size(), true));
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Process
#ifdef _OPENMP
void Process::openmp_evaluate_reduction(EvaluatedResults &evaluated_results)
{

    std::vector<Results<uint64_t>> save_results(evaluated_results.first);
    std::vector<Results<uint64_t>> results(evaluated_results.first);
#pragma omp declare reduction(VecPlus                            \
                              : std::vector <Results <uint64_t>> \
                              : omp_out = merge(omp_out, omp_in))
#pragma omp parallel default(none) shared(evaluated_results, thread_num_iterations, results)
    {
        SimulationData *sim = threads_data.at(omp_get_thread_num());
        sim->set_decks(this->your_decks, this->enemy_decks);
#pragma omp for reduction(VecPlus \
                          : results) schedule(runtime)
        for (unsigned i = 0; i < thread_num_iterations; ++i)
        {
            if (results.size() == 0)
                results = sim->evaluate(mask); // calculate single sim
            else
            {
                results = merge(results, sim->evaluate(mask)); // calculate single sim
            }
        }
#pragma omp for schedule(runtime)
        for (unsigned i = 0; i < results.size(); ++i)
            evaluated_results.first[i] = results[i]; //+?
    }
    evaluated_results.second += thread_num_iterations;
}

void Process::openmp_compare_reduction(EvaluatedResults &evaluated_results)
{

    const unsigned c_num_threads = opt_num_threads;
    std::vector<Results<uint64_t>> save_results(evaluated_results.first);
    std::vector<Results<uint64_t>> results(evaluated_results.first);
    bool compare_stop{false};
    unsigned trials[c_num_threads] = {0};
    long double successes[c_num_threads] = {.0};
    const long double max_possible = max_possible_score[(size_t)optimization_mode];
    const long double prob = 1 - confidence_level;
    omp_lock_t locks[c_num_threads];
    for (unsigned i = 0; i < c_num_threads; ++i)
        omp_init_lock(&locks[i]);
#pragma omp declare reduction(VecPlus                            \
                              : std::vector <Results <uint64_t>> \
                              : omp_out = merge(omp_out, omp_in))
#pragma omp parallel default(none) shared(evaluated_results, trials, successes, locks, compare_stop, thread_num_iterations, results, \
                                          thread_best_results, min_increment_of_score)
    {
        SimulationData *sim = threads_data.at(omp_get_thread_num());
        sim->set_decks(this->your_decks, this->enemy_decks);
        const long double sim_accum = std::accumulate(sim->factors.begin(), sim->factors.end(), .0);
#pragma omp for reduction(VecPlus \
                          : results) schedule(runtime)
        for (unsigned i = 0; i < thread_num_iterations; ++i)
        {
#pragma omp cancellation point for
            if (!compare_stop)
            {
                if (results.size() == 0)
                    results = sim->evaluate(mask); // calculate single sim
                else
                    results = merge(results, sim->evaluate(mask)); // calculate single sim
                long double score_accum_d = 0.0;
                for (unsigned j = 0; j < results.size(); ++j)
                    score_accum_d += results[j].points * sim->factors[j];
                score_accum_d /= sim_accum * max_possible;
                omp_set_lock(&locks[omp_get_thread_num()]);
                trials[omp_get_thread_num()]++;
                successes[omp_get_thread_num()] = score_accum_d;
                omp_unset_lock(&locks[omp_get_thread_num()]);
                // #pragma omp master
                if (omp_get_thread_num() == 0)
                {

                    unsigned ttrials = 0;
                    long double ssuccesses = 0.0;
                    for (unsigned l = 0; l < c_num_threads; ++l)
                    {
                        omp_set_lock(&locks[l]);
                        ttrials += trials[l];
                        ssuccesses += successes[l];
                        omp_unset_lock(&locks[l]);
                    }
                    if (ssuccesses > ttrials)
                    {
                        ssuccesses = ttrials;
                        printf("WARNING: biominal successes {%Le} > trials {%d} in Threads\n", ssuccesses, ttrials);
                        //_DEBUG_MSG(2,"WARNING: biominal successes > trials in Threads");
                    }
                    compare_stop = (boost::math::binomial_distribution<>::find_upper_bound_on_p(ttrials, ssuccesses, prob) * max_possible <
                                    thread_best_results->points + min_increment_of_score);
                    if (compare_stop)
                    {
#pragma omp cancel for
                    }
                }
            }
        }
#pragma omp for schedule(runtime)
        for (unsigned i = 0; i < results.size(); ++i)
            evaluated_results.first[i] = results[i]; //+?
    }
    evaluated_results.second += thread_num_iterations;
    for (unsigned i = 0; i < c_num_threads; ++i)
        omp_destroy_lock(&locks[i]);
}

std::vector<Results<uint64_t>> merge(std::vector<Results<uint64_t>> out, std::vector<Results<uint64_t>> in)
{
    // printf("merging out: %d in: %d \n", (int)out[0].wins, (int)in[0].wins);
    // printf("out%p ",(void *)&out );
    for (unsigned i = 0; i < out.size(); ++i)
        out[i] += in[i];
    // printf("merged out: %d \n", (int)out[0].wins);
    return out;
}
#endif

inline bool Process::eval_ml(unsigned num_iterations, EvaluatedResults &evaluated_results)
{
    if (!use_ml)
        return true;
    bool run = false;
    auto sampls = samples();
    // iterate results from samples()
    for (unsigned i = 0; i < sampls.size(); i++)
    {
        // already found in db?
        if (mask[i])
        {
            auto s = sampls[i];
            auto wins = atof(win_model.predict(s).c_str());
            auto losses = atof(loss_model.predict(s).c_str());
            auto draws = atof(stall_model.predict(s).c_str());
            auto points = atof(points_model.predict(s).c_str());
            if (wins < 0)
                wins = 0;
            if (losses < 0)
                losses = 0;
            if (draws < 0)
                draws = 0;
            if (points < 0)
                points = 0;
            // if((wins + losses + draws) > 1.05 || (wins + losses + draws) < 0.95) {
            //     std::cout << "WARNING: ML model returned invalid sample: " << wins << " "  << draws << " " <<losses << " points " << points << std::endl;
            // }
            if (use_only_ml || ((wins + losses + draws) < 1.00 + ml_precision && (wins + losses + draws) > 1.00 - ml_precision)) // we use the sum of wins, losses and draws to determine if the sample is valid
            {
                // std::cout << "ML model returned: " << wins << " "  << draws << " " <<losses << " points " << points << " " << num_iterations << std::endl;
                mask[i] = false;
                Results<uint64_t> r = {
                    static_cast<uint64_t>(std::round((1.0 * wins * num_iterations / (wins + losses + draws)))),
                    static_cast<uint64_t>(std::round((1.0 * draws * num_iterations / (wins + losses + draws)))),
                    static_cast<uint64_t>(std::round((1.0 * losses * num_iterations / (wins + losses + draws)))),
                    static_cast<uint64_t>(std::round((1.0 * points * num_iterations))),
                    static_cast<uint64_t>(num_iterations)};
                evaluated_results.first[i] = r;
            }
            else
            {
                mask[i] = true;
                run = true;
            }
        }
    }

    return run;
}

inline bool Process::check_db(std::vector<std::array<std::string, 3>> const &vhashes, unsigned num_iterations, EvaluatedResults &evaluated_results)
{
    if (!use_db_load)
        return true;
    bool run = false;
    // TODO TUO5 check db for results
    //  TODO TUO5 calculate mask for sim evaluate based on missing results in db
    bool found = false;
    for (unsigned i = 0; i < vhashes.size(); i++)
    {
        std::string hash = vhashes[i][0];
        // TODO TUO5 check db for hash
        auto it1 = database.find(hash);
        std::map<std::string, Results<uint64_t>>::iterator it;
        if (it1 != database.end())
        {
            auto it2 = it1->second.find(vhashes[i][1]);
            if (it2 != it1->second.end())
            {
                auto it3 = it2->second.find(vhashes[i][2]);
                if (it3 != it2->second.end())
                {
                    it = it3;
                    found = true;
                    // std::cout << "hit databse" << std::endl;
                }
            }
        }
        // TODO TUO5 if hash not in db, add to mask
        if (!found)
        {
            mask[i] = true;
            run = true;
        }
        else
        {
            if (it->second.count >= num_iterations)
            {
                mask[i] = false;
                // TUO5 load results from db
                // renormalized to asked num_iterations
                Results<uint64_t> r = {
                    static_cast<uint64_t>(std::round((1.0 * it->second.wins * num_iterations) / it->second.count)),
                    static_cast<uint64_t>(std::round((1.0 * it->second.draws * num_iterations) / it->second.count)),
                    static_cast<uint64_t>(std::round((1.0 * it->second.losses * num_iterations) / it->second.count)),
                    static_cast<uint64_t>(std::round((1.0 * it->second.points * num_iterations) / it->second.count)),
                    static_cast<uint64_t>(num_iterations)};
                evaluated_results.first[i] = r;
            }
            else
            {
                mask[i] = true;
                run = true;
            }
        }
    }
    return run;
}

inline void Process::save_db(std::vector<std::array<std::string, 3>> const &vhashes, EvaluatedResults &evaluated_results)
{
    // TODO TUO5 save results to db
    for (unsigned i = 0; i < vhashes.size(); i++)
    {
        if (mask[i])
        {
            std::array<std::string, 3> hash = vhashes[i];
            // check if map is initialized otherwise initialize
            if (database.find(hash[0]) == database.end())
            {
                database[hash[0]] = std::map<std::string, std::map<std::string, Results<uint64_t>>>();
            }
            if (database[hash[0]].find(hash[1]) == database[hash[0]].end())
            {
                database[hash[0]][hash[1]] = std::map<std::string, Results<uint64_t>>();
            }
            database[hash[0]][hash[1]][hash[2]] = evaluated_results.first[i];
        }
    }
}

EvaluatedResults &Process::evaluate(unsigned num_iterations, EvaluatedResults &evaluated_results)
{
    if (num_iterations <= evaluated_results.second)
    {
        return evaluated_results;
    }
    std::vector<std::array<std::string, 3>> vhashes = hashes();
    // fill mask with true
    mask = std::vector<bool>(vhashes.size(), true);
    if (!check_db(vhashes, num_iterations, evaluated_results))
    {
        // 100% covered by db
        evaluated_results.second = num_iterations;
        return evaluated_results;
    }
    if (!eval_ml(num_iterations, evaluated_results))
    {
        // ml is good
        evaluated_results.second = num_iterations;
        return evaluated_results;
    }

    thread_num_iterations = num_iterations - evaluated_results.second;
    thread_results = &evaluated_results;
    thread_compare = false;
#ifndef _OPENMP
    // unlock all the threads
    main_barrier.wait();
    // wait for the threads
    main_barrier.wait();
#else
    openmp_evaluate_reduction(evaluated_results);
#endif
    save_db(vhashes, evaluated_results);

    return evaluated_results;
}

EvaluatedResults &Process::compare(unsigned num_iterations, EvaluatedResults &evaluated_results, const FinalResults<long double> &best_results)
{
    if (num_iterations <= evaluated_results.second)
    {
        return evaluated_results;
    }
    std::vector<std::array<std::string, 3>> vhashes = hashes();
    // fill mask with true
    mask = std::vector<bool>(vhashes.size(), true);
    if (!check_db(vhashes, num_iterations, evaluated_results))
    {
        // 100% covered by db
        evaluated_results.second = num_iterations;
        return evaluated_results;
    }
    if (!eval_ml(num_iterations, evaluated_results))
    {
        // ml is good
        evaluated_results.second = num_iterations;
        return evaluated_results;
    }
    thread_num_iterations = num_iterations - evaluated_results.second;
    thread_results = &evaluated_results;
    thread_best_results = &best_results;
    thread_compare = true;
    thread_compare_stop = false;
#ifndef _OPENMP
    // unlock all the threads
    main_barrier.wait();
    // wait for the threads
    main_barrier.wait();
#else
    openmp_compare_reduction(evaluated_results);
#endif
    save_db(vhashes, evaluated_results);
    return evaluated_results;
}
//------------------------------------------------------------------------------
void thread_evaluate(boost::barrier &main_barrier,
                     boost::mutex &shared_mutex,
                     SimulationData &sim,
                     const Process &p,
                     unsigned thread_id)
{
#ifndef _OPENMP
    while (true)
    {
        main_barrier.wait();
        sim.set_decks(p.your_decks, p.enemy_decks);
        if (destroy_threads)
        {
            return;
        }
        while (true)
        {
            shared_mutex.lock();                                                       //<<<<
            if (thread_num_iterations == 0 || (thread_compare && thread_compare_stop)) //!
            {
                shared_mutex.unlock(); //>>>>
                main_barrier.wait();
                break;
            }
            else
            {
                --thread_num_iterations; //!
                shared_mutex.unlock();   //>>>>
                std::vector<Results<uint64_t>> result{sim.evaluate(p.mask)};
                shared_mutex.lock();                                                        //<<<<
                std::vector<uint64_t> thread_score_local(thread_results->first.size(), 0u); //!
                unsigned counter = 0;
                for (unsigned index(0); index < result.size(); ++index)
                {
                    thread_results->first[index] += result[index]; //!
                    if (counter == 0 || thread_results->first[index].count < counter)
                    {
                        counter = thread_results->first[index].count;
                    }
                }
                for (unsigned index(0); index < result.size(); ++index)
                {
                    // TUO5 we weight scores by the number of times a deck is used, since the results are no langer same sized in partially cached case
                    thread_score_local[index] = (thread_results->first[index].points * (counter)) / thread_results->first[index].count; //!
                                                                                                                                        // thread_score_local[index] = static_cast<uint64_t>(std::round((thread_results->first[index].points * (counter))/thread_results->first[index].count)); //!
                }
                ++thread_results->second;                            //!
                unsigned thread_total_local{thread_results->second}; //!
                shared_mutex.unlock();                               //>>>>
                if (thread_compare && thread_id == 0 && thread_total_local > 1)
                {
                    unsigned score_accum = 0;
                    // Multiple defense decks case: scaling by factors and approximation of a "discrete" number of events.
                    if (result.size() > 1)
                    {
                        long double score_accum_d = 0.0;
                        for (unsigned i = 0; i < thread_score_local.size(); ++i)
                        {
                            score_accum_d += thread_score_local[i] * sim.factors[i];
                        }
                        score_accum_d /= std::accumulate(sim.factors.begin(), sim.factors.end(), .0);
                        score_accum = score_accum_d;
                    }
                    else
                    {
                        score_accum = thread_score_local[0];
                    }
                    bool compare_stop(false);
                    long double max_possible = max_possible_score[(size_t)optimization_mode];

                    // APN
                    auto trials = thread_total_local;
                    auto prob = 1 - confidence_level;
                    auto successes = score_accum / max_possible;
                    if (successes > trials)
                    {
                        std::cout << "WARNING: biominal successes > trials in Threads" << successes << " " << trials << " " << counter << std::endl;
                        _DEBUG_MSG(2, "WARNING: biominal successes > trials in Threads");
                        successes = trials;
                    }
                    else
                    {

                        // Get a loose (better than no) upper bound. TODO: Improve it.
                        compare_stop = (boost::math::binomial_distribution<>::find_upper_bound_on_p(trials, successes, prob) * max_possible <
                                        thread_best_results->points + min_increment_of_score);
                        if (compare_stop)
                        {
                            shared_mutex.lock(); //<<<<
                            // std::cout << thread_total_local << "\n";
                            thread_compare_stop = true; //!
                            shared_mutex.unlock();      //>>>>
                        }
                    }
                }
            }
        }
    }
#endif
}
//------------------------------------------------------------------------------
void print_score_info(const EvaluatedResults &results, std::vector<long double> &factors)
{
    auto final = compute_score(results, factors);
    std::cout << final.points << " (";
    if (!simplify_output)
    {
        if (show_ci)
        {
            std::cout << final.points_lower_bound << "-" << final.points_upper_bound << ", ";
        }
        for (const auto &val : results.first)
        {
            switch (optimization_mode)
            {
            case OptimizationMode::raid:
            case OptimizationMode::campaign:
            case OptimizationMode::brawl:
            case OptimizationMode::brawl_defense:
            case OptimizationMode::war:
            case OptimizationMode::war_defense:
#ifndef NQUEST
            case OptimizationMode::quest:
#endif
                std::cout << val.points << " ";
                break;
            default:
                std::cout << val.points / 100 << " ";
                break;
            }
        }
    }
    else
    {
        std::cout << "...";
    }
    std::cout << "/ " << results.second << ")" << std::endl;
}
//------------------------------------------------------------------------------
void print_results(const EvaluatedResults &results, std::vector<long double> &factors)
{
    auto final = compute_score(results, factors);
    std::cout << "win%: " << final.wins * 100.0 << " (";
    if (!simplify_output)
    {
        for (const auto &val : results.first)
        {
            std::cout << val.wins << " ";
        }
    }
    else
    {
        std::cout << "...";
    }
    std::cout << "/ " << results.second << ")" << std::endl;

    std::cout << "stall%: " << final.draws * 100.0 << " (";
    if (!simplify_output)
    {
        for (const auto &val : results.first)
        {
            std::cout << val.draws << " ";
        }
    }
    else
    {
        std::cout << "...";
    }
    std::cout << "/ " << results.second << ")" << std::endl;

    std::cout << "loss%: " << final.losses * 100.0 << " (";
    if (!simplify_output)
    {
        for (const auto &val : results.first)
        {
            std::cout << val.losses << " ";
        }
    }
    else
    {
        std::cout << "...";
    }
    std::cout << "/ " << results.second << ")" << std::endl;

#ifndef NQUEST
    if (optimization_mode == OptimizationMode::quest)
    {
        // points = win% * win_score + (must_win ? win% : 100%) * quest% * quest_score
        // quest% = (points - win% * win_score) / (must_win ? win% : 100%) / quest_score
        std::cout << "quest%: " << (final.points - final.wins * tuo::quest.win_score) / (tuo::quest.must_win ? final.wins : 1) / tuo::quest.quest_score * 100 << std::endl;
    }
#endif

    unsigned min_score = min_possible_score[(size_t)optimization_mode];
    unsigned max_score = max_possible_score[(size_t)optimization_mode];
    switch (optimization_mode)
    {
    case OptimizationMode::raid:
    case OptimizationMode::campaign:
    case OptimizationMode::brawl:
    case OptimizationMode::brawl_defense:
    case OptimizationMode::war:
    case OptimizationMode::war_defense:
#ifndef NQUEST
    case OptimizationMode::quest:
#endif
        std::cout << "score: " << final.points;
        if (optimization_mode == OptimizationMode::brawl)
        {
            auto win_points = final.wins ? ((final.points - min_score * (1.0 - final.wins)) / final.wins) : final.points;
            std::cout << " [" << win_points << " per win]";
        }
        else if (optimization_mode == OptimizationMode::brawl_defense)
        {
            auto opp_win_points = final.losses ? max_score - ((final.points - (max_score - min_score) * (1.0 - final.losses)) / final.losses) : final.points;
            std::cout << " [" << opp_win_points << " per opp win]";
        }
        else if (optimization_mode == OptimizationMode::war)
        {
            auto win_points = final.wins ? ((final.points - min_score * (1.0 - final.wins)) / final.wins) : final.points;
            std::cout << " [" << win_points << " per win]";
        }
        else if (optimization_mode == OptimizationMode::war_defense)
        {
            auto opp_win_points = final.losses ? max_score - ((final.points - (max_score - min_score) * (1.0 - final.losses)) / final.losses) : final.points;
            std::cout << " [" << opp_win_points << " per opp win]";
        }
        std::cout << " (";
        if (!simplify_output)
        {
            for (const auto &val : results.first)
            {
                std::cout << val.points << " ";
            }
        }
        else
        {
            std::cout << "...";
        }
        std::cout << "/ " << results.second << ")" << std::endl;
        if (show_ci)
        {
            std::cout << "ci: " << final.points_lower_bound << " - " << final.points_upper_bound << std::endl;
        }
        break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
// Prints which cards have been upgraded
void print_upgraded_cards(Deck *deck)
{
    if (!print_upgraded)
        return;
    auto owned_cards_c = owned_cards;
    std::cout << "Upgraded Cards: ";
    auto cards = deck->cards;
    cards.insert(cards.begin(), deck->commander);
    for (const Card *card : cards)
    {

        if ((card->m_set == 1000) && (card->m_rarity <= 2) && (card->is_low_level_card()))
        {
            continue;
        }

        if ((card->m_id == 50002) || (card->m_category == CardCategory::dominion_material))
        {
            continue;
        }

        if ((fund || (card->m_category != CardCategory::normal)) && (owned_cards_c[card->m_id] == 0))
        {
            // print card
            std::cout << card->m_name << ", ";
        }
        if (owned_cards_c[card->m_id] > 0)
            owned_cards_c[card->m_id]--;
    }
    std::cout << std::endl;
}
//------------------------------------------------------------------------------
void print_cards_inline(std::vector<const Card *> cards, std::ostream &os, Deck *deck)
{
    std::string last_name = "";
    unsigned num_repeat(0);
    bool first = true;

    // for (const Card* card: cards)
    //{
    for (unsigned i = 0; i < cards.size(); ++i)
    {
        auto card = cards[i];

        if (deck == nullptr && card->m_name == last_name)
        {
            ++num_repeat;
        }
        else
        {
            if (num_repeat > 1)
            {
                os << " #" << num_repeat;
            }
            if (deck != nullptr)
                os << (first ? "" : ", ") << (deck->card_marks[i] == '!' ? "!" : "") << card->m_name;
            else
                os << (first ? "" : ", ") << card->m_name;
            first = false;
            last_name = card->m_name;
            num_repeat = 1;
        }
    }
    if (num_repeat > 1)
    {
        os << " #" << num_repeat;
    }
    os << "\n";
}
void print_score_inline(const FinalResults<long double> score)
{
    // print optimization result details
    switch (optimization_mode)
    {
    case OptimizationMode::raid:
    case OptimizationMode::campaign:
    case OptimizationMode::brawl:
    case OptimizationMode::brawl_defense:
    case OptimizationMode::war:
    case OptimizationMode::war_defense:
#ifndef NQUEST
    case OptimizationMode::quest:
#endif
        std::cout << "(" << score.wins * 100 << "% win";
#ifndef NQUEST
        if (optimization_mode == OptimizationMode::quest)
        {
            std::cout << ", " << (score.points - score.wins * tuo::quest.win_score) / (tuo::quest.must_win ? score.wins : 1) / tuo::quest.quest_score * 100 << "% quest";
        }
#endif
        if (show_ci)
        {
            std::cout << ", " << score.points_lower_bound << "-" << score.points_upper_bound;
        }
        std::cout << ") ";
        break;
    case OptimizationMode::defense:
        std::cout << "(" << score.draws * 100.0 << "% stall) ";
        break;
    default:
        break;
    }
    std::cout << score.points;
    unsigned min_score = min_possible_score[(size_t)optimization_mode];
    unsigned max_score = max_possible_score[(size_t)optimization_mode];
    if (optimization_mode == OptimizationMode::brawl)
    {
        auto win_points = score.wins ? ((score.points - min_score * (1.0 - score.wins)) / score.wins) : score.points;
        std::cout << " [" << win_points << " per win]";
    }
    else if (optimization_mode == OptimizationMode::brawl_defense)
    {
        auto opp_win_points = score.losses ? max_score - ((score.points - (max_score - min_score) * (1.0 - score.losses)) / score.losses) : score.points;
        std::cout << " [" << opp_win_points << " per opp win]";
    }
    else if (optimization_mode == OptimizationMode::war)
    {
        auto win_points = score.wins ? ((score.points - min_score * (1.0 - score.wins)) / score.wins) : score.points;
        std::cout << " [" << win_points << " per win]";
    }
    else if (optimization_mode == OptimizationMode::war_defense)
    {
        auto opp_win_points = score.losses ? max_score - ((score.points - (max_score - min_score) * (1.0 - score.losses)) / score.losses) : score.points;
        std::cout << " [" << opp_win_points << " per opp win]";
    }
}
//------------------------------------------------------------------------------
// Calculates and prints individual card value in this deck
void print_sim_card_values(Deck *original_deck, Process &p, unsigned iter) // run_deck == p.your_decks[0]
{
    if (!print_values && vc_x == 0)
        return;
    if (p.your_decks.size() != 1)
        return; // only for single deck
    // auto deck = original_deck;
    if (original_deck->strategy == DeckStrategy::random || original_deck->strategy == DeckStrategy::flexible || original_deck->strategy == DeckStrategy::evaluate || original_deck->strategy == DeckStrategy::evaluate_twice)
    {
        std::sort(original_deck->cards.begin(), original_deck->cards.end(), [](const Card *a, const Card *b)
                  { return a->m_id < b->m_id; });
    }
    std::string last_name;
    long double score;
    Deck *sim_deck = p.your_decks[0];
    Deck *your_deck = original_deck->clone(); // save the original deck
    auto cards = your_deck->cards;
    // sim original/base result
    sim_deck->commander = your_deck->commander;
    sim_deck->cards = your_deck->cards;
    sim_deck->alpha_dominion = your_deck->alpha_dominion;
    EvaluatedResults results = {EvaluatedResults::first_type(p.enemy_decks.size() * p.your_decks.size()), 0};
    results = p.evaluate(iter, results);
    const FinalResults<long double> fr_base = compute_score(results, p.factors);
    long double base = fr_base.points;
    std::multimap<long double, std::string> msn;
    std::multimap<std::string, long double> mns;
    for (unsigned i = 0; i < cards.size(); ++i)
    {
        auto card = cards[i];
        // if (card->m_name == last_name)
        //{
        // }
        // else
        //{
        last_name = card->m_name;
        // sim it
        sim_deck->cards = your_deck->cards;                 // reset cards
        sim_deck->cards.erase(sim_deck->cards.begin() + i); // remove to test card
        EvaluatedResults results = {EvaluatedResults::first_type(p.enemy_decks.size() * p.your_decks.size()), 0};
        results = p.evaluate(iter, results);
        const FinalResults<long double> fr = compute_score(results, p.factors);
        score = base - fr.points; // subtract from result to get value
        // std::cout << card->m_name << " (" << score << "), ";
        msn.insert(std::make_pair(score, card->m_name));
        mns.insert(std::make_pair(card->m_name, score));
        //}
    }
    sim_deck->cards = your_deck->cards; // reset cards
    if (vc_x > 0)
    {
        // std::cout << "Locked Deck: ";
        ////std::cout << original_deck->cards.size() << " units: ";
        ////print_score_inline(fr_base);
        ////std::cout <<  ": ";
        // std::cout << original_deck->commander->m_name;
        //  print dominions
        // if (original_deck->alpha_dominion)
        //{ std::cout << ", " << original_deck->alpha_dominion->m_name; }

        for (unsigned i = 0; i < cards.size(); ++i)
        {
            original_deck->card_marks.erase(i);
            if (vc_x >= std::distance(msn.find(mns.find(cards[i]->m_name)->second), msn.end())) // or mns.start()
            {
                original_deck->card_marks[i] = '!';
            }
        }
    }
    if (print_values)
    {
        std::cout << "Value of Cards: ";
        for (unsigned i = 0; i < cards.size(); ++i)
        {
            std::cout << cards[i]->m_name << " (" << mns.find(cards[i]->m_name)->second << "), ";
        }
        std::cout << std::endl;
    }
}
//------------------------------------------------------------------------------
void print_deck_inline(const unsigned deck_cost, const FinalResults<long double> score, Deck *deck, bool print_locked)
{
    // print units count
    std::cout << deck->cards.size() << " units: ";

    // print deck cost (if fund is enabled)
    if (fund > 0)
    {
        std::cout << "$" << deck_cost << " ";
    }
    print_score_inline(score);
    /*
    // print optimization result details
    switch(optimization_mode)
    {
    case OptimizationMode::raid:
    case OptimizationMode::campaign:
    case OptimizationMode::brawl:
    case OptimizationMode::brawl_defense:
    case OptimizationMode::war:
    case OptimizationMode::war_defense:
#ifndef NQUEST
case OptimizationMode::quest:
#endif
std::cout << "(" << score.wins * 100 << "% win";
#ifndef NQUEST
if (optimization_mode == OptimizationMode::quest)
{
std::cout << ", " << (score.points - score.wins * quest.win_score) / (quest.must_win ? score.wins : 1) / quest.quest_score * 100 << "% quest";
}
#endif
if (show_ci)
{
std::cout << ", " << score.points_lower_bound << "-" << score.points_upper_bound;
}
std::cout << ") ";
break;
case OptimizationMode::defense:
std::cout << "(" << score.draws * 100.0 << "% stall) ";
break;
default:
break;
}
std::cout << score.points;
unsigned min_score = min_possible_score[(size_t)optimization_mode];
unsigned max_score = max_possible_score[(size_t)optimization_mode];
if (optimization_mode == OptimizationMode::brawl)
{
auto win_points = score.wins ? ((score.points - min_score * (1.0 - score.wins)) / score.wins) : score.points;
std::cout << " [" << win_points << " per win]";
}
else if (optimization_mode == OptimizationMode::brawl_defense)
{
auto opp_win_points = score.losses ? max_score - ((score.points - (max_score - min_score) * (1.0 - score.losses)) / score.losses) : score.points;
std::cout << " [" << opp_win_points << " per opp win]";
}
else if (optimization_mode == OptimizationMode::war)
{
auto win_points = score.wins ? ((score.points - min_score * (1.0 - score.wins)) / score.wins) : score.points;
std::cout << " [" << win_points << " per win]";
}
else if (optimization_mode == OptimizationMode::war_defense)
{
auto opp_win_points = score.losses ? max_score - ((score.points - (max_score - min_score) * (1.0 - score.losses)) / score.losses) : score.points;
std::cout << " [" << opp_win_points << " per opp win]";
}
*/

    // print commander
    std::cout << ": " << deck->commander->m_name;

    // print dominions
    if (deck->alpha_dominion)
    {
        std::cout << ", " << deck->alpha_dominion->m_name;
    }

    // print deck cards
    if (deck->strategy == DeckStrategy::random || deck->strategy == DeckStrategy::flexible || deck->strategy == DeckStrategy::evaluate || deck->strategy == DeckStrategy::evaluate_twice)
    {
        std::sort(deck->cards.begin(), deck->cards.end(), [](const Card *a, const Card *b)
                  { return a->m_id < b->m_id; });
    }
    std::cout << ", ";
    if (print_locked)
        print_cards_inline(deck->cards, std::cout, deck);
    else
        print_cards_inline(deck->cards, std::cout);
}
//------------------------------------------------------------------------------
bool is_timeout_reached()
{
    if (__builtin_expect(maximum_time > 0, false))
    {
        if ((std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start_time)).count() > maximum_time * 60 * 60)
        {
            std::cout << "Time expired! Current result:" << std::endl;
            return true;
        }
    }
    return false;
}
std::vector<std::vector<const Card *>> get_candidate_lists(Process &proc)
{
    std::vector<std::vector<const Card *>> ret_candidates;
    std::vector<const Card *> commander_candidates;
    std::vector<const Card *> alpha_dominion_candidates;
    std::vector<const Card *> card_candidates;

    // resolve available to player cards
    auto player_assaults_and_structures = proc.cards.player_commanders;
    player_assaults_and_structures.insert(player_assaults_and_structures.end(), proc.cards.player_structures.begin(), proc.cards.player_structures.end());
    player_assaults_and_structures.insert(player_assaults_and_structures.end(), proc.cards.player_assaults.begin(), proc.cards.player_assaults.end());
    for (auto it = player_assaults_and_structures.begin(); it != player_assaults_and_structures.end(); ++it)
    {
        const Card *card = *it;
        // skip illegal
        if ((card->m_category != CardCategory::dominion_alpha) && (card->m_category != CardCategory::normal))
        {
            continue;
        }

        // skip dominions when their climbing is disabled
        if ((card->m_category == CardCategory::dominion_alpha) && (!use_owned_dominions))
        {
            continue;
        }

        // try to skip a card unless it's allowed
        if (!allowed_candidates.count(card->m_id))
        {
            // skip disallowed always
            if (disallowed_candidates.count(card->m_id))
            {
                continue;
            }

            // handle dominions
            if (card->m_category == CardCategory::dominion_alpha)
            {
                // skip non-top-level dominions anyway
                // (will check it later and downgrade if necessary according to amount of material (shards))
                /*if (!card->is_top_level_card())
                  { continue; }

                // skip basic dominions
                if ((card->m_id == 50001) || (card->m_id == 50002))
                { continue; }*/
            }

            // handle normal cards
            else
            {
                // skip non-top-level cards (adjust_deck() will try to downgrade them if necessary)
                bool use_top_level = (card->m_type == CardType::commander) ? use_top_level_commander : use_top_level_card;
                if (!card->is_top_level_card() and (fund || use_top_level || !owned_cards[card->m_id]))
                {
                    continue;
                }

                // skip lowest fusion levels
                unsigned use_fused_level = (card->m_type == CardType::commander) ? use_fused_commander_level : use_fused_card_level;
                if (card->m_fusion_level < use_fused_level)
                {
                    continue;
                }
            }
        }

        if (use_owned_cards && card->m_category == CardCategory::dominion_alpha && !owned_cards[card->m_id])
        {
            if (use_maxed_dominions && card->m_used_for_cards.size() == 0)
            {
            }
            else
            {
                continue;
            }
        }
        // skip unavailable cards anyway when ownedcards is used
        if (use_owned_cards && !(card->m_category == CardCategory::dominion_alpha) && !is_owned_or_can_be_fused(card))
        {
            continue;
        }
        // mono
        if (!factions.empty() && (std::find(factions.begin(), factions.end(), card->m_faction) == factions.end()) != invert_factions) // XOR
        {
            continue;
        }

        // enqueue candidate according to category & type
        if (card->m_type == CardType::commander)
        {
            commander_candidates.emplace_back(card);
        }
        else if (card->m_category == CardCategory::dominion_alpha)
        {
            alpha_dominion_candidates.emplace_back(card);
        }
        else if (card->m_category == CardCategory::normal)
        {
            bool contains{false};
            if (!skills.empty())
            {
                for (Skill::Skill skill_id : skills)
                {
                    if (card->m_skill_value[skill_id])
                    {
                        contains = true;
                        break;
                    }
                }
                if (!contains != invert_skills)
                    continue; // XOR
            }
            if (only_recent && !((it + (player_assaults_and_structures.size() * recent_percent / 100)) > player_assaults_and_structures.end()))
                continue;                                                                                                                      // latest %
            if (prefered_recent && it + (player_assaults_and_structures.size() * recent_percent) / 100 > player_assaults_and_structures.end()) // latest %
            {
                for (unsigned k = 0; k < prefered_factor; ++k)
                    card_candidates.emplace_back(card);
            }
            for (Skill::Skill skill_id : prefered_skills)
            {
                if (card->m_skill_value[skill_id])
                {
                    for (unsigned k = 0; k < prefered_factor; ++k)
                        card_candidates.emplace_back(card);
                }
            }

            card_candidates.emplace_back(card);
        }
    }
    card_candidates.emplace_back(nullptr);

    ret_candidates.emplace_back(commander_candidates);
    ret_candidates.emplace_back(alpha_dominion_candidates);
    ret_candidates.emplace_back(card_candidates);
    return ret_candidates;
}
//------------------------------------------------------------------------------
enum Operation
{
    noop,
    simulate,
    climb,
    climb_forts,
    anneal,
    genetic,
    beam,
    reorder,
    debug,
    debuguntil,
};
//------------------------------------------------------------------------------
extern void (*skill_table[Skill::num_skills])(Field *, CardStatus *src_status, const SkillSpec &);
void print_available_effects()
{
    std::cout << "Available effects besides activation skills:\n"
                 "  Bloodlust X\n"
                 "  Brigade\n"
                 "  Counterflux\n"
                 "  Divert\n"
                 "  Enduring-Rage\n"
                 "  Fortification\n"
                 "  Heroism\n"
                 "  Zealots-Preservation\n"
                 "  Metamorphosis\n"
                 "  Megamorphosis\n"
                 "  Revenge X\n"
                 "  Turning-Tides\n"
                 "  Virulence\n"
                 "  Halted-Orders\n"
                 "  Devour X\n"
                 "  Critical-Reach\n"
                 "  Temporal-Backlash\n"
                 "  Furiosity\n"
                 "  Oath-Of-Loyalty\n"
                 "  Blood-Vengeance\n"
                 "  Cold-Sleep\n"
                 "  Iron-Will\n"
                 "  Unity\n"
                 "  Devotion\n"
                 "  Crackdown\n"
                 "  SuperHeroism\n";
}
bool check_input_amount(int argc, const char **argv, int argIndex, int number)
{
    if (argc <= argIndex + number)
    {
        std::cerr << argv[argIndex] << " needs " << number << " paramters" << std::endl;
        return true;
    }
    return false;
}
void input_error(std::string msg)
{
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}
void usage(int argc, const char **argv)
{
    std::cout << "Tyrant Unleashed Optimizer (TUO) " << TYRANT_OPTIMIZER_VERSION << "\n"
                                                                                    "usage: "
              << argv[0] << " Your_Deck Enemy_Deck [Flags] [Operations]\n"
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
                            "  -e \"<effect>\": set the battleground effect; you may use -e multiple times.\n"
                            "  -r: the attack deck is played in order instead of randomly (respects the 3 cards drawn limit).\n"
                            "  -s: use surge (default is fight).\n"
                            "  -t <num>: set the number of threads, default is 4.\n"
                            "  win:     simulate/optimize for win rate. default for non-raids.\n"
                            "  defense: simulate/optimize for win rate + stall rate. can be used for defending deck or win rate oriented raid simulations.\n"
                            "  raid:    simulate/optimize for average raid damage (ARD). default for raids.\n"
                            "Flags for climb:\n"
                            "  -c: don't try to optimize the commander.\n"
                            "  -L <min> <max>: restrict deck size between <min> and <max>.\n"
                            "  -o: restrict to the owned cards listed in \"data/ownedcards.txt\".\n"
                            "  -o=<filename>: restrict to the owned cards listed in <filename>.\n"
                            "  fund <num>: invest <num> SP to upgrade cards.\n"
                            "  target <num>: stop as soon as the score reaches <num>.\n"
                            "\n"
                            "Operations:\n"
                            "  sim <num>: simulate <num> battles to evaluate a deck.\n"
                            "  climb <num>: perform hill-climbing starting from the given attack deck, using up to <num> battles to evaluate a deck.\n"
                            "  reorder <num>: optimize the order for given attack deck, using up to <num> battles to evaluate an order.\n"
#ifndef NDEBUG
                            "  debug: testing purpose only. very verbose output. only one battle.\n"
                            "  debuguntil <min> <max>: testing purpose only. fight until the last fight results in range [<min>, <max>]. recommend to redirect std::coutput.\n"
#endif
        ;
}

bool parse_bge(
    std::string bge_name,
    unsigned player,
    const std::unordered_map<std::string, std::string> &bge_aliases,
    std::array<signed short, PassiveBGE::num_passive_bges> &your_bg_effects,
    std::array<signed short, PassiveBGE::num_passive_bges> &enemy_bg_effects,
    std::vector<SkillSpec> &your_bg_skills,
    std::vector<SkillSpec> &enemy_bg_skills,
    std::unordered_set<std::string> used_bge_aliases)
{
    // skip empty
    tuo::trim(bge_name);
    if (bge_name.empty())
    {
        return true;
    }

    // is effect combined?
    if (bge_name.find_first_of(";|") != std::string::npos)
    {
        std::vector<std::string> bges;
        boost::split(bges, bge_name, boost::is_any_of(";|"));
        for (auto &&next_bge : bges)
        {
            if (!parse_bge(next_bge, player, bge_aliases, your_bg_effects, enemy_bg_effects, your_bg_skills, enemy_bg_skills, used_bge_aliases))
            {
                return false;
            }
        }
        return true;
    }

    // try to resolve bge name as alias
    std::string simple_bge_name = simplify_name(bge_name);
    const auto bge_alias_itr = bge_aliases.find(simple_bge_name);
    if (bge_alias_itr != bge_aliases.end())
    {
        if (!used_bge_aliases.insert(simple_bge_name).second)
        {
            throw std::runtime_error("BGE alias: " + bge_name + ": Circular reference");
        }
        return parse_bge(bge_alias_itr->second, player, bge_aliases, your_bg_effects, enemy_bg_effects, your_bg_skills, enemy_bg_skills, used_bge_aliases);
    }

    // parse as passive or skill based BGE
    std::vector<std::string> tokens, skill_name_list;
    boost::split(tokens, bge_name, boost::is_any_of(" "));
    boost::split(skill_name_list, tokens[0], boost::is_any_of("+"));
    try
    {
        for (auto &&skill_name : skill_name_list)
        {
            PassiveBGE::PassiveBGE passive_bge_id = passive_bge_name_to_id(skill_name);
            Skill::Skill skill_id = skill_name_to_id(skill_name);
            if (passive_bge_id != PassiveBGE::no_bge)
            {
                // map bge id to its value (if present otherwise zero)
                signed short bge_value = (tokens.size() > 1) ? boost::lexical_cast<signed short>(tokens[1]) : 1;
                if (!bge_value)
                    throw std::runtime_error("BGE " + skill_name + ": zero value means no BGE");
                if ((player == 0) or (player == 2))
                    your_bg_effects[passive_bge_id] = bge_value;
                if ((player == 1) or (player == 2))
                    enemy_bg_effects[passive_bge_id] = bge_value;
            }
            else if (skill_table[skill_id] != nullptr)
            {
                unsigned skill_index = 1;
                // activation BG skill
                SkillSpec bg_skill{
                    skill_id,
                    0,
                    allfactions,
                    0,
                    0,
                    Skill::no_skill,
                    Skill::no_skill,
                    false,
                    0,
                };

                // skill [ ALL | N ] ...
                if (skill_index < tokens.size() && boost::to_lower_copy(tokens[skill_index]) == "all")
                {
                    bg_skill.all = true;
                    skill_index += 1;
                }
                else if (skill_index + 1 < tokens.size() && isdigit(*tokens[skill_index].c_str()))
                {
                    bg_skill.n = boost::lexical_cast<unsigned>(tokens[skill_index]);
                    skill_index += 1;
                }

                // skill n [ FACTION ] ...
                if ((skill_index + 1) < tokens.size())
                {
                    for (auto f = allfactions + 1; f < num_factions; ++f)
                    {
                        if (boost::to_lower_copy(tokens[skill_index]) == faction_names[f])
                        {
                            bg_skill.y = static_cast<Faction>(f);
                            skill_index += 1;
                            break;
                        }
                    }
                }

                if (skill_index < tokens.size())
                {
                    bg_skill.s = skill_name_to_id(tokens[skill_index]);
                    if (bg_skill.s != Skill::no_skill)
                    {
                        skill_index += 1;
                        if (skill_index < tokens.size() && (boost::to_lower_copy(tokens[skill_index]) == "to" || boost::to_lower_copy(tokens[skill_index]) == "into"))
                        {
                            skill_index += 1;
                        }
                        if (skill_index < tokens.size())
                        {
                            bg_skill.s2 = skill_name_to_id(tokens[skill_index]);
                            if (bg_skill.s2 != Skill::no_skill)
                            {
                                skill_index += 1;
                            }
                        }
                    }
                }
                if (skill_index < tokens.size())
                {
                    if (bg_skill.id == Skill::jam || bg_skill.id == Skill::overload)
                    {
                        bg_skill.n = boost::lexical_cast<unsigned>(tokens[skill_index]);
                    }
                    else
                    {
                        bg_skill.x = boost::lexical_cast<unsigned>(tokens[skill_index]);
                    }
                }
                switch (player)
                {
                case 0:
                    your_bg_skills.push_back(bg_skill);
                    break;
                case 1:
                    enemy_bg_skills.push_back(bg_skill);
                    break;
                case 2:
                    your_bg_skills.push_back(bg_skill);
                    enemy_bg_skills.push_back(bg_skill);
                    break;
                default:
                    throw std::runtime_error("Bad player number: " + boost::lexical_cast<std::string>(player));
                }
            }
            else
            {
                std::cerr << "Error: unrecognized effect \"" << bge_name << "\".\n";
                std::cout << "Unrecognized effect \"" << bge_name << "\".\n";
                print_available_effects();
                return false;
            }
        }
    }
    catch (const boost::bad_lexical_cast &e)
    {
        throw std::runtime_error("Expect a number in effect \"" + bge_name + "\"");
    }
    return true;
}

DeckResults run(int argc, const char **argv)
{
    start_time = std::chrono::system_clock::now();
    DeckResults fr;
    opt_num_threads = 4;
    DeckStrategy::DeckStrategy opt_your_strategy(DeckStrategy::random);
    DeckStrategy::DeckStrategy opt_enemy_strategy(DeckStrategy::random);
    std::string opt_forts, opt_enemy_forts;
    std::string opt_doms, opt_enemy_doms;
    std::string opt_hand, opt_enemy_hand;
    std::string opt_vip;
    std::string opt_allow_candidates;
    std::string opt_disallow_candidates;
    std::string opt_disallow_recipes;
#ifndef NQUEST
    std::string opt_quest;
#endif
    std::string opt_target_score;
    std::vector<std::string> fn_suffix_list{
        "",
    };
    std::string prefix = "";
    std::vector<std::string> opt_owned_cards_str_list;
    bool opt_do_optimization(false);
    bool opt_multi_optimization(false);
    bool opt_do_reorder(false);
    bool opt_keep_commander{false};
    std::vector<std::tuple<unsigned, unsigned, Operation>> opt_todo;
    std::vector<std::string> opt_effects[3]; // 0-you; 1-enemy; 2-global
    std::array<signed short, PassiveBGE::num_passive_bges> opt_bg_effects[2];
    std::vector<SkillSpec> opt_bg_skills[2];

    std::string your_deck_list{argv[1]};
    std::string enemy_deck_list{argv[2]};

    for (int argIndex = 3; argIndex < argc; ++argIndex)
    {
        if (strcmp(argv[argIndex], "deck") == 0)
        {
            your_deck_list = std::string(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "enemy:deck") == 0)
        {
            enemy_deck_list = std::string(argv[argIndex + 1]);
            argIndex += 1;
        }
        // Codec
        else if (strcmp(argv[argIndex], "ext_b64") == 0)
        {
            hash_to_ids = hash_to_ids_ext_b64;
            encode_deck = encode_deck_ext_b64;
        }
        else if (strcmp(argv[argIndex], "wmt_b64") == 0)
        {
            hash_to_ids = hash_to_ids_wmt_b64;
            encode_deck = encode_deck_wmt_b64;
        }
        else if (strcmp(argv[argIndex], "ddd_b64") == 0)
        {
            hash_to_ids = hash_to_ids_ddd_b64;
            encode_deck = encode_deck_ddd_b64;
        }
        else if (strcmp(argv[argIndex], "ml") == 0)
        {
            use_ml = true;
            use_db_write = false;
        }
        else if (strcmp(argv[argIndex], "noop") == 0)
        {
        }
        else if (strcmp(argv[argIndex], "no-ml") == 0)
        {
            use_ml = false;
        }
        else if (strcmp(argv[argIndex], "only-ml") == 0)
        {
            use_ml = true;
            use_only_ml = true;
            use_db_write = false;
        }
        else if (strcmp(argv[argIndex], "ml-precision") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            ml_precision = std::stof(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "no-db-write") == 0)
        {
            use_db_write = false;
        }
        else if (strcmp(argv[argIndex], "no-db-load") == 0)
        {
            use_db_load = false;
        }
        else if (strcmp(argv[argIndex], "no-db") == 0)
        {
            use_db_load = false;
            use_db_write = false;
        }
        else if (strcmp(argv[argIndex], "strict-db") == 0)
        {
            use_strict_db = true;
        }
        else if (strcmp(argv[argIndex], "db-limit") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            db_limit = std::stoi(argv[argIndex + 1]);
            argIndex += 1;
        }

        // Base Game Mode
        else if (strcmp(argv[argIndex], "fight") == 0)
        {
            gamemode = fight;
        }
        else if (strcmp(argv[argIndex], "-s") == 0 || strcmp(argv[argIndex], "surge") == 0)
        {
            gamemode = surge;
        }
        // Base Scoring Mode
        else if (strcmp(argv[argIndex], "win") == 0)
        {
            optimization_mode = OptimizationMode::winrate;
        }
        else if (strcmp(argv[argIndex], "defense") == 0)
        {
            optimization_mode = OptimizationMode::defense;
        }
        else if (strcmp(argv[argIndex], "raid") == 0)
        {
            optimization_mode = OptimizationMode::raid;
        }
        // Mode Package
        else if (strcmp(argv[argIndex], "campaign") == 0)
        {
            gamemode = surge;
            optimization_mode = OptimizationMode::campaign;
        }
        else if (strcmp(argv[argIndex], "pvp") == 0)
        {
            gamemode = fight;
            optimization_mode = OptimizationMode::winrate;
        }
        else if (strcmp(argv[argIndex], "pvp-defense") == 0)
        {
            gamemode = surge;
            optimization_mode = OptimizationMode::defense;
        }
        else if (strcmp(argv[argIndex], "brawl") == 0)
        {
            gamemode = surge;
            optimization_mode = OptimizationMode::brawl;
        }
        else if (strcmp(argv[argIndex], "brawl-defense") == 0)
        {
            gamemode = fight;
            optimization_mode = OptimizationMode::brawl_defense;
        }
        else if (strcmp(argv[argIndex], "gw") == 0)
        {
            gamemode = surge;
            optimization_mode = OptimizationMode::war;
        }
        else if (strcmp(argv[argIndex], "gw-defense") == 0)
        {
            gamemode = fight;
            optimization_mode = OptimizationMode::war_defense;
        }
        // Others
        else if (strcmp(argv[argIndex], "keep-commander") == 0 || strcmp(argv[argIndex], "-c") == 0)
        {
            opt_keep_commander = true;
        }
        else if (strcmp(argv[argIndex], "mono") == 0 || strcmp(argv[argIndex], "-m") == 0 || strcmp(argv[argIndex], "factions") == 0 || strcmp(argv[argIndex], "-f") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            if (strcmp(argv[argIndex + 1], "") != 0)
            {
                factions.push_back(faction_name_to_id(argv[argIndex + 1]));
            }
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "no-mono") == 0 || strcmp(argv[argIndex], "no-factions") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            factions.push_back(faction_name_to_id(argv[argIndex + 1]));
            invert_factions = true;
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "strategy") == 0 || strcmp(argv[argIndex], "skill") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            if (strcmp(argv[argIndex + 1], "") != 0)
            {
                if (strcmp(argv[argIndex + 1], "recent") == 0)
                {
                    only_recent = true;
                }
                else
                {
                    skills.push_back(skill_name_to_id(argv[argIndex + 1]));
                }
            }
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "no-strategy") == 0 || strcmp(argv[argIndex], "no-skill") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            if (strcmp(argv[argIndex + 1], "") != 0)
            {
                skills.push_back(skill_name_to_id(argv[argIndex + 1]));
                invert_skills = true;
            }
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "prefered-strategy") == 0 || strcmp(argv[argIndex], "prefered-skill") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            if (strcmp(argv[argIndex + 1], "") != 0)
            {
                if (strcmp(argv[argIndex + 1], "recent") == 0)
                {
                    prefered_recent = true;
                }
                else
                {
                    prefered_skills.push_back(skill_name_to_id(argv[argIndex + 1]));
                }
            }
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "prefered-factor") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            prefered_factor = std::stoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "recent-percent") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            recent_percent = std::stoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "effect") == 0 || strcmp(argv[argIndex], "-e") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_effects[2].push_back(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "ye") == 0 || strcmp(argv[argIndex], "yeffect") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_effects[0].push_back(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "ee") == 0 || strcmp(argv[argIndex], "eeffect") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_effects[1].push_back(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "freeze") == 0 || strcmp(argv[argIndex], "-F") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_freezed_cards = atoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "-L") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 2))
                exit(1);
            min_deck_len = atoi(argv[argIndex + 1]);
            max_deck_len = atoi(argv[argIndex + 2]);
            argIndex += 2;
        }
        else if (strcmp(argv[argIndex], "-o-") == 0)
        {
            use_owned_cards = false;
        }
        else if (strcmp(argv[argIndex], "-o") == 0)
        {
            opt_owned_cards_str_list.push_back(prefix + "data/ownedcards.txt");
            use_owned_cards = true;
        }
        else if (strncmp(argv[argIndex], "-o=", 3) == 0)
        {
            std::string fname{prefix};
            fname += (argv[argIndex] + 3);
            opt_owned_cards_str_list.push_back(fname);
            use_owned_cards = true;
        }
        else if (strncmp(argv[argIndex], "_", 1) == 0)
        {
            fn_suffix_list.push_back(argv[argIndex]);
        }
        else if (strcmp(argv[argIndex], "prefix") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            prefix = argv[argIndex + 1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "fund") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            fund = atoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "dom-none") == 0)
        {
            use_owned_dominions = false;
            use_maxed_dominions = false;
        }
        else if (strcmp(argv[argIndex], "dom+") == 0 || strcmp(argv[argIndex], "dominion+") == 0 || strcmp(argv[argIndex], "dom-owned") == 0)
        {
            use_owned_dominions = true;
            use_maxed_dominions = false;
        }
        else if (strcmp(argv[argIndex], "dom-") == 0 || strcmp(argv[argIndex], "dominion-") == 0 || strcmp(argv[argIndex], "dom-maxed") == 0)
        {
            use_owned_dominions = true;
            use_maxed_dominions = true;
        }
        else if (strcmp(argv[argIndex], "random") == 0)
        {
            opt_your_strategy = DeckStrategy::random;
        }
        else if (strcmp(argv[argIndex], "-r") == 0 || strcmp(argv[argIndex], "ordered") == 0)
        {
            opt_your_strategy = DeckStrategy::ordered;
        }
        else if (strcmp(argv[argIndex], "flexible") == 0 || strcmp(argv[argIndex], "flex") == 0)
        {
            opt_your_strategy = DeckStrategy::flexible;
        }
        else if (strcmp(argv[argIndex], "flexible-iter") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            flexible_iter = atoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "flexible-turn") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            flexible_turn = atoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "evaluate") == 0 || strcmp(argv[argIndex], "eval") == 0)
        {
            opt_your_strategy = DeckStrategy::evaluate;
        }
        else if (strcmp(argv[argIndex], "evaluate2") == 0 || strcmp(argv[argIndex], "eval2") == 0)
        {
            opt_your_strategy = DeckStrategy::evaluate_twice;
        }
        else if (strcmp(argv[argIndex], "eval-iter") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            eval_iter = atoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "eval-turn") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            eval_turn = atoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "exact-ordered") == 0)
        {
            opt_your_strategy = DeckStrategy::exact_ordered;
        }
        else if (strcmp(argv[argIndex], "enemy:evaluate") == 0)
        {
            opt_enemy_strategy = DeckStrategy::evaluate;
        }
        else if (strcmp(argv[argIndex], "enemy:evaluate2") == 0)
        {
            opt_enemy_strategy = DeckStrategy::evaluate_twice;
        }
        else if (strcmp(argv[argIndex], "enemy:flexible") == 0)
        {
            opt_enemy_strategy = DeckStrategy::flexible;
        }
        else if (strcmp(argv[argIndex], "enemy:ordered") == 0)
        {
            opt_enemy_strategy = DeckStrategy::ordered;
        }
        else if (strcmp(argv[argIndex], "enemy:flexible") == 0)
        {
            opt_enemy_strategy = DeckStrategy::flexible;
        }
        else if (strcmp(argv[argIndex], "enemy:exact-ordered") == 0)
        {
            opt_enemy_strategy = DeckStrategy::exact_ordered;
        }
        else if (strcmp(argv[argIndex], "endgame") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            use_fused_card_level = atoi(argv[argIndex + 1]);
            argIndex += 1;
        }
#ifndef NQUEST
        else if (strcmp(argv[argIndex], "quest") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_quest = argv[argIndex + 1];
            argIndex += 1;
        }
#endif
        else if (strcmp(argv[argIndex], "threads") == 0 || strcmp(argv[argIndex], "-t") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_num_threads = atoi(argv[argIndex + 1]);
#ifdef _OPENMP
            omp_set_num_threads(opt_num_threads);
#endif

            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "target") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_target_score = argv[argIndex + 1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "turnlimit") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            turn_limit = atoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "mis") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            min_increment_of_score = atof(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "timeout") == 0) // set timeout in hours. tuo will stop approx. at the given time.
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            maximum_time = atof(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "strict-ownedcards") == 0) // won't add missing cards, instead skips faulty decks
        {
            opt_skip_unclaimed_decks = true;
        }
        /*else if (strcmp(argv[argIndex], "trim-ownedcards") == 0) //won't add missing cards, instead removes missing cards from decks
          {
          opt_trim_unclaimed_decks = true;
          }*/
        else if (strcmp(argv[argIndex], "cl") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            confidence_level = atof(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "+so") == 0)
        {
            simplify_output = true;
        }
        else if (strcmp(argv[argIndex], "+uc") == 0)
        {
            print_upgraded = true;
        }
        else if (strcmp(argv[argIndex], "+vc") == 0)
        {
            print_values = true;
        }
        else if (strcmp(argv[argIndex], "+vc-x") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            vc_x = atoi(argv[argIndex + 1]);
            // print_values = true;
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "+ci") == 0)
        {
            show_ci = true;
        }
        else if (strcmp(argv[argIndex], "+hm") == 0)
        {
            use_harmonic_mean = true;
        }
        else if (strcmp(argv[argIndex], "no-fix") == 0)
        {
            for (int i = 0; i < Fix::num_fixes; ++i)
                fixes[i] = false;
        }
        else if (strcmp(argv[argIndex], "fix-enhance-early") == 0)
        {
            fixes[Fix::enhance_early] = true;
        }
        else if (strcmp(argv[argIndex], "fix-revenge-on-death") == 0)
        {
            fixes[Fix::revenge_on_death] = true;
        }
        else if (strcmp(argv[argIndex], "fix-death-from-bge") == 0)
        {
            fixes[Fix::death_from_bge] = true;
        }
        else if (strcmp(argv[argIndex], "fix-legion-under-megamorphosis") == 0)
        {
            fixes[Fix::legion_under_mega] = true;
        }
    else if (strcmp(argv[argIndex], "update-barrier-each-turn") == 0)
		{
			fixes[Fix::barrier_each_turn] = true;
		}
        else if (strcmp(argv[argIndex], "no-update-barrier-each-turn") == 0)
		{
			fixes[Fix::barrier_each_turn] = false;
		}
        else if (strcmp(argv[argIndex], "update-dont-evade-mimic-selection") == 0)
		{
			fixes[Fix::dont_evade_mimic_selection] = true;
		}
        else if (strcmp(argv[argIndex], "no-update-dont-evade-mimic-selection") == 0)
		{
			fixes[Fix::dont_evade_mimic_selection] = false;
		}
        else if (strcmp(argv[argIndex], "update-leech-increase-max-hp") == 0)
		{
			fixes[Fix::leech_increase_max_hp] = true;
		}
        else if (strcmp(argv[argIndex], "no-update-leech-increase-max-hp") == 0)
		{
			fixes[Fix::leech_increase_max_hp] = false;
		}
        else if (strcmp(argv[argIndex], "update-counter-without-damage") == 0)
		{
			fixes[Fix::counter_without_damage] = true;
		}
        else if (strcmp(argv[argIndex], "no-update-counter-without-damage") == 0)
		{
			fixes[Fix::counter_without_damage] = false;
		}
        else if (strcmp(argv[argIndex], "update-corrosive-protect-armor") == 0)
		{
			fixes[Fix::corrosive_protect_armor] = true;
		}
        else if (strcmp(argv[argIndex], "no-update-corrosive-protect-armor") == 0)
		{
			fixes[Fix::corrosive_protect_armor] = false;
		}
        else if (strcmp(argv[argIndex], "update-poison-after-attacked") == 0)
		{
			fixes[Fix::poison_after_attacked] = true;
		}
        else if (strcmp(argv[argIndex], "no-update-poison-after-attacked") == 0)
		{
			fixes[Fix::poison_after_attacked] = false;
		}
        else if (strcmp(argv[argIndex], "update-subdue-before-attack") == 0)
		{
			fixes[Fix::subdue_before_attack] = true;
		}
        else if (strcmp(argv[argIndex], "no-update-subdue-before-attack") == 0)
		{
			fixes[Fix::subdue_before_attack] = false;
		}
        else if (strcmp(argv[argIndex], "seed") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            sim_seed = atoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "-v") == 0)
        {
            --debug_print;
        }
        else if (strcmp(argv[argIndex], "+v") == 0)
        {
            ++debug_print;
        }
        else if (strcmp(argv[argIndex], "vip") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_vip = argv[argIndex + 1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "allow-candidates") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_allow_candidates = argv[argIndex + 1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "disallow-candidates") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_disallow_candidates = argv[argIndex + 1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "disallow-recipes") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_disallow_recipes = argv[argIndex + 1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "hand") == 0) // set initial hand for test
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_hand = argv[argIndex + 1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "enemy:hand") == 0) // set enemies' initial hand for test
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_enemy_hand = argv[argIndex + 1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "yf") == 0 || strcmp(argv[argIndex], "yfort") == 0) // set forts
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_forts = std::string(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "yfpool") == 0 || strcmp(argv[argIndex], "yfortpool") == 0) // set forts
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            yfpool = std::stoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "ef") == 0 || strcmp(argv[argIndex], "efort") == 0) // set enemies' forts
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_enemy_forts = std::string(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "efpool") == 0 || strcmp(argv[argIndex], "efortpool") == 0) // set forts
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            efpool = std::stoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "yd") == 0 || strcmp(argv[argIndex], "ydom") == 0) // set dominions
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_doms = std::string(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "ed") == 0 || strcmp(argv[argIndex], "edom") == 0) // set enemies' dominions
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_enemy_doms = std::string(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "sim") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), 0u, simulate));
            if (std::get<0>(opt_todo.back()) < 10)
            {
                opt_num_threads = 1;
            }
            argIndex += 1;
        }
        // climbing tasks
        else if (strcmp(argv[argIndex], "climbex") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 2))
                exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 2]), climb));
            if (std::get<1>(opt_todo.back()) < 10)
            {
                opt_num_threads = 1;
            }
            opt_do_optimization = true;
            opt_multi_optimization = true;
            argIndex += 2;
        }
        else if (strcmp(argv[argIndex], "climb") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 1]), climb));
            if (std::get<1>(opt_todo.back()) < 10)
            {
                opt_num_threads = 1;
            }
            opt_do_optimization = true;
            opt_multi_optimization = true;
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "climb_forts") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 1]), climb_forts));
            if (std::get<1>(opt_todo.back()) < 10)
            {
                opt_num_threads = 1;
            }
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "anneal") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 3))
                exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 1]), anneal));
            temperature = std::stod(argv[argIndex + 2]);
            coolingRate = std::stod(argv[argIndex + 3]);
            if (std::get<1>(opt_todo.back()) < 10)
            {
                opt_num_threads = 1;
            }
            opt_do_optimization = true;
            opt_multi_optimization = true;
            argIndex += 3;
        }
        else if (strcmp(argv[argIndex], "genetic") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 1]), genetic));
            if (std::get<1>(opt_todo.back()) < 10)
            {
                opt_num_threads = 1;
            }
            opt_do_optimization = true;
            opt_multi_optimization = true;
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "genetic-pool") == 0 || strcmp(argv[argIndex], "beam-size") == 0 || strcmp(argv[argIndex], "size") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            pool_size = std::stod(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "genetic-gen") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            generations = std::stod(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "genetic-opts") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 3))
                exit(1);
            opt_pool_keep = std::stod(argv[argIndex + 1]);
            opt_pool_cross = std::stod(argv[argIndex + 2]);
            opt_pool_mutate = std::stod(argv[argIndex + 3]);
            argIndex += 3;
        }
        else if (strcmp(argv[argIndex], "beam") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 1]), beam));
            if (std::get<1>(opt_todo.back()) < 10)
            {
                opt_num_threads = 1;
            }
            opt_do_optimization = true;
            opt_multi_optimization = true;
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "reorder") == 0)
        {
            if (check_input_amount(argc, argv, argIndex, 1))
                exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 1]), reorder));
            if (std::get<1>(opt_todo.back()) < 10)
            {
                opt_num_threads = 1;
            }
            opt_do_reorder = true;
            argIndex += 1;
        }
        else if (strncmp(argv[argIndex], "scale-opts:", 11) == 0)
        {
            std::string climb_opts_str(argv[argIndex] + 11);
            boost::tokenizer<boost::char_delimiters_separator<char>> climb_opts{climb_opts_str, boost::char_delimiters_separator<char>{false, ",", ""}};
            for (const auto &opt : climb_opts)
            {
                const auto dot_pos = opt.find(".");
                const auto slash_pos = opt.find("/");
                const bool has_value = (dot_pos != std::string::npos);
                if (slash_pos == std::string::npos)
                    throw std::runtime_error("scale-opts:" + opt + " requires an argument");
                const std::string &opt_type = has_value ? opt.substr(0, dot_pos) : "";
                const std::string opt_name{has_value ? opt.substr(dot_pos + 1, slash_pos - dot_pos - 1) : opt.substr(0, slash_pos)};
                const std::string opt_value{opt.substr(slash_pos + 1)};
                if ((opt_name == "hp"))
                {
                    hp_scale = atof(opt_value.c_str());
                }
                else if ((opt_name == "atk"))
                {
                    atk_scale = atof(opt_value.c_str());
                }
                else if (opt_name == "x")
                {
                    x_skill_scale[skill_name_to_id(opt_type)] = atof(opt_value.c_str());
                }
                else if (opt_name == "n")
                {
                    n_skill_scale[skill_name_to_id(opt_type)] = atof(opt_value.c_str());
                }
                else if (opt_name == "c")
                {
                    c_skill_scale[skill_name_to_id(opt_type)] = atof(opt_value.c_str());
                }
                else
                {
                    std::cerr << "Error: Unknown scale option " << opt_name << " of " << opt_type;
                    if (has_value)
                    {
                        std::cerr << " (value is: " << opt_value << ")";
                    }
                    std::cerr << std::endl;
                    exit(1);
                }
            }
        }
        // climbing options
        else if (strncmp(argv[argIndex], "climb-opts:", 11) == 0)
        {
            std::string climb_opts_str(argv[argIndex] + 11);
            boost::tokenizer<boost::char_delimiters_separator<char>> climb_opts{climb_opts_str, boost::char_delimiters_separator<char>{false, ",", ""}};
            for (const auto &opt : climb_opts)
            {
                const auto delim_pos = opt.find("=");
                const bool has_value = (delim_pos != std::string::npos);
                const std::string &opt_name = has_value ? opt.substr(0, delim_pos) : opt;
                const std::string opt_value{has_value ? opt.substr(delim_pos + 1) : opt};
                auto ensure_opt_value = [](const bool has_value, const std::string &opt_name)
                {
                    if (!has_value)
                    {
                        throw std::runtime_error("climb-opts:" + opt_name + " requires an argument");
                    }
                };
                if ((opt_name == "iter-mul") or (opt_name == "iterations-multiplier"))
                {
                    ensure_opt_value(has_value, opt_name);
                    iterations_multiplier = std::stoi(opt_value);
                }
                else if ((opt_name == "egc") or (opt_name == "endgame-commander") or (opt_name == "min-commander-fusion-level"))
                {
                    ensure_opt_value(has_value, opt_name);
                    use_fused_commander_level = std::stoi(opt_value);
                }
                else if (opt_name == "use-all-commander-levels")
                {
                    use_top_level_commander = false;
                }
                else if (opt_name == "use-all-card-levels")
                {
                    use_top_level_card = false;
                }
                else if ((opt_name == "recent-boost") or (opt_name == "rb")) // prefer new cards in hill climb and break climb loop faster
                {
                    prefered_recent = true;
                }
                else if ((opt_name == "recent-boost-times") or (opt_name == "rbt")) // prefer new cards in hill climb and break climb loop faster
                {
                    ensure_opt_value(has_value, opt_name);
                    prefered_factor = std::stoi(opt_value);
                }
                else if ((opt_name == "recent-boost-percent") or (opt_name == "rbp")) // prefer new cards in hill climb and break climb loop faster
                {
                    ensure_opt_value(has_value, opt_name);
                    recent_percent = std::stoi(opt_value);
                }
                else if ((opt_name == "otd") or (opt_name == "open-the-deck"))
                {
                    mode_open_the_deck = true;
                }
                else
                {
                    std::cerr << "Error: Unknown climb option " << opt_name;
                    if (has_value)
                    {
                        std::cerr << " (value is: " << opt_value << ")";
                    }
                    std::cerr << std::endl;
                    exit(1);
                }
            }
        }
        else if (strcmp(argv[argIndex], "debug") == 0)
        {
            opt_todo.push_back(std::make_tuple(0u, 0u, debug));
            opt_num_threads = 1;
        }
        else if (strcmp(argv[argIndex], "debuguntil") == 0)
        {
            // output the debug info for the first battle that min_score <= score <= max_score.
            // E.g., 0 0: lose; 100 100: win (non-raid); 20 100: at least 20 damage (raid).
            if (check_input_amount(argc, argv, argIndex, 2))
                exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 2]), debuguntil));
            opt_num_threads = 1;
            argIndex += 2;
        }
        else
        {
            std::cerr << "Error: Unknown option " << argv[argIndex] << std::endl;
            exit(1);
        }
    }
    load_db(prefix);
    load_ml(prefix);


#ifdef _OPENMP
    opt_num_threads = omp_get_max_threads();
#endif
    // TODO delete ? since prefix/suffix might change we reload all cards.
    // if(all_cards.all_cards.size()>0) delete(&all_cards); // complains invalid pointer
    // all_cards.organize();
    all_cards = Cards();
    Decks decks;
    std::unordered_map<std::string, std::string> bge_aliases;
    load_skills_set_xml(all_cards, prefix + "data/skills_set.xml", true);
    for (unsigned section = 1;
         load_cards_xml(all_cards, prefix + "data/cards_section_" + std::to_string(section) + ".xml", false);
         ++section)
        ;
    all_cards.organize();
    load_levels_xml(all_cards, prefix + "data/levels.xml", true);
    all_cards.fix_dominion_recipes();
    for (const auto &suffix : fn_suffix_list)
    {
        load_decks_xml(decks, all_cards, prefix + "data/missions" + suffix + ".xml", prefix + "data/raids" + suffix + ".xml", suffix.empty());
        load_recipes_xml(all_cards, prefix + "data/fusion_recipes_cj2" + suffix + ".xml", suffix.empty());
        read_card_abbrs(all_cards, prefix + "data/cardabbrs" + suffix + ".txt");
    }
    for (const auto &suffix : fn_suffix_list)
    {
        load_custom_decks(decks, all_cards, prefix + "data/customdecks" + suffix + ".txt");
        map_keys_to_set(read_custom_cards(all_cards, prefix + "data/allowed_candidates" + suffix + ".txt", false), allowed_candidates);
        map_keys_to_set(read_custom_cards(all_cards, prefix + "data/disallowed_candidates" + suffix + ".txt", false), disallowed_candidates);
        map_keys_to_set(read_custom_cards(all_cards, prefix + "data/disallowed_recipes" + suffix + ".txt", false), disallowed_recipes);
    }

    read_bge_aliases(bge_aliases, prefix + "data/bges.txt");

    fill_skill_table();

    if (opt_do_optimization and use_owned_cards)
    {
        if (opt_owned_cards_str_list.empty())
        { // load default files only if specify no -o=
            for (const auto &suffix : fn_suffix_list)
            {
                std::string filename = prefix + "data/ownedcards" + suffix + ".txt";
                if (boost::filesystem::exists(filename))
                {
                    opt_owned_cards_str_list.push_back(filename);
                }
            }
        }
        std::map<unsigned, unsigned> _owned_cards;
        for (const auto &oc_str : opt_owned_cards_str_list)
        {
            read_owned_cards(all_cards, _owned_cards, oc_str);
        }

        // keep only one copy of alpha dominion
        for (auto owned_it = _owned_cards.begin(); owned_it != _owned_cards.end();)
        {
            const Card *owned_card = all_cards.by_id(owned_it->first);
            bool need_remove = (!owned_it->second);
            if (!need_remove && (owned_card->m_category == CardCategory::dominion_alpha))
            {
                if (!owned_alpha_dominion)
                {
                    owned_alpha_dominion = owned_card;
                }
                else
                {
                    /*std::cerr << "Warning: ownedcards already contains alpha dominion (" << owned_alpha_dominion->m_name
                      << "): removing additional " << owned_card->m_name << std::endl;
                      need_remove = true;*/
                }
            }
            if (need_remove)
            {
                owned_it = _owned_cards.erase(owned_it);
            }
            else
            {
                ++owned_it;
            }
        }
        if (!owned_alpha_dominion && use_owned_dominions)
        {
            // owned_alpha_dominion = all_cards.by_id(50002);
            // std::cerr << "Warning: dominion climbing enabled and no alpha dominion found in owned cards, adding default "
            //     << owned_alpha_dominion->m_name << std::endl;
        }
        if (owned_alpha_dominion)
        {
            _owned_cards[owned_alpha_dominion->m_id] = 1;
        }

        // remap owned cards to unordered map (should be quicker for searching)
        owned_cards.reserve(_owned_cards.size());
        for (auto owned_it = _owned_cards.begin(); owned_it != _owned_cards.end(); ++owned_it)
        {
            owned_cards[owned_it->first] = owned_it->second;
        }
    }

    // parse BGEs
    opt_bg_effects[0].fill(0);
    opt_bg_effects[1].fill(0);
    for (int player = 2; player >= 0; --player)
    {
        for (auto &&opt_effect : opt_effects[player])
        {
            std::unordered_set<std::string> used_bge_aliases;
            if (!parse_bge(opt_effect, player, bge_aliases, opt_bg_effects[0], opt_bg_effects[1], opt_bg_skills[0], opt_bg_skills[1], used_bge_aliases))
            {
                exit(1);
            }
        }
    }

    // parse allowed candidates from options
    try
    {
        auto &&id_marks = string_to_ids(all_cards, opt_allow_candidates, "allowed-candidates");
        for (const auto &cid : id_marks.first)
        {
            allowed_candidates.insert(cid);
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Error: allow-candidates " << opt_allow_candidates << ": " << e.what() << std::endl;
        exit(1);
    }

    // parse disallowed candidates from options
    try
    {
        auto &&id_marks = string_to_ids(all_cards, opt_disallow_candidates, "disallowed-candidates");
        for (const auto &cid : id_marks.first)
        {
            disallowed_candidates.insert(cid);
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Error: disallow-candidates " << opt_disallow_candidates << ": " << e.what() << std::endl;
        exit(1);
    }

    // parse & drop disallowed recipes
    try
    {
        auto &&id_dis_recipes = string_to_ids(all_cards, opt_disallow_recipes, "disallowed-recipes");
        for (auto &cid : id_dis_recipes.first)
        {
            all_cards.erase_fusion_recipe(cid);
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Error: disallow-recipes " << opt_disallow_recipes << ": " << e.what() << std::endl;
        exit(1);
    }
    for (auto cid : disallowed_recipes)
    {
        all_cards.erase_fusion_recipe(cid);
    }

#ifndef NQUEST
    if (!opt_quest.empty())
    {
        try
        {
            optimization_mode = OptimizationMode::quest;
            std::vector<std::string> tokens;
            boost::split(tokens, opt_quest, boost::is_any_of(" -"));
            if (tokens.size() < 3)
            {
                throw std::runtime_error("Expect one of: su n skill; sd n skill; cu n faction/strcture; ck n structure");
            }
            auto type_str = boost::to_lower_copy(tokens[0]);
            tuo::quest.quest_value = boost::lexical_cast<unsigned>(tokens[1]);
            auto key_str = boost::to_lower_copy(tokens[2]);
            unsigned quest_index = 3;
            if (type_str == "su" || type_str == "sd")
            {
                Skill::Skill skill_id = skill_name_to_id(key_str);
                if (skill_id == Skill::no_skill)
                {
                    std::cerr << "Error: Expect skill in quest \"" << opt_quest << "\".\n";
                    exit(1);
                }
                tuo::quest.quest_type = type_str == "su" ? QuestType::skill_use : QuestType::skill_damage;
                tuo::quest.quest_key = skill_id;
            }
            else if (type_str == "cu" || type_str == "ck")
            {
                if (key_str == "assault")
                {
                    tuo::quest.quest_type = type_str == "cu" ? QuestType::type_card_use : QuestType::type_card_kill;
                    tuo::quest.quest_key = CardType::assault;
                }
                else if (key_str == "structure")
                {
                    tuo::quest.quest_type = type_str == "cu" ? QuestType::type_card_use : QuestType::type_card_kill;
                    tuo::quest.quest_key = CardType::structure;
                }
                else
                {
                    for (unsigned i = 1; i < Faction::num_factions; ++i)
                    {
                        if (key_str == boost::to_lower_copy(faction_names[i]))
                        {
                            tuo::quest.quest_type = type_str == "cu" ? QuestType::faction_assault_card_use : QuestType::faction_assault_card_kill;
                            tuo::quest.quest_key = i;
                            break;
                        }
                    }
                    if (tuo::quest.quest_key == 0)
                    {
                        std::cerr << "Error: Expect assault, structure or faction in quest \"" << opt_quest << "\".\n";
                        exit(1);
                    }
                }
            }
            else if (type_str == "cs")
            {
                unsigned card_id;
                unsigned card_num;
                char num_sign;
                char mark;
                try
                {
                    parse_card_spec(all_cards, key_str, card_id, card_num, num_sign, mark);
                    tuo::quest.quest_type = QuestType::card_survival;
                    tuo::quest.quest_key = card_id;
                }
                catch (const std::runtime_error &e)
                {
                    std::cerr << "Error: Expect a card in quest \"" << opt_quest << "\".\n";
                    exit(1);
                }
            }
            else if (type_str == "suoc" && tokens.size() >= 4)
            {
                Skill::Skill skill_id = skill_name_to_id(key_str);
                if (skill_id == Skill::no_skill)
                {
                    std::cerr << "Error: Expect skill in quest \"" << opt_quest << "\".\n";
                    exit(1);
                }
                unsigned card_id;
                unsigned card_num;
                char num_sign;
                char mark;
                try
                {
                    parse_card_spec(all_cards, boost::to_lower_copy(tokens[3]), card_id, card_num, num_sign, mark);
                    quest_index += 1;
                    tuo::quest.quest_type = QuestType::skill_use;
                    tuo::quest.quest_key = skill_id;
                    tuo::quest.quest_2nd_key = card_id;
                }
                catch (const std::runtime_error &e)
                {
                    std::cerr << "Error: Expect a card in quest \"" << opt_quest << "\".\n";
                    exit(1);
                }
            }
            else
            {
                throw std::runtime_error("Expect one of: su n skill; sd n skill; cu n faction/strcture; ck n structure");
            }
            tuo::quest.quest_score = tuo::quest.quest_value;
            for (unsigned i = quest_index; i < tokens.size(); ++i)
            {
                const auto &token = tokens[i];
                if (token == "each")
                {
                    tuo::quest.must_fulfill = true;
                    tuo::quest.quest_score = 100;
                }
                else if (token == "win")
                {
                    tuo::quest.must_win = true;
                }
                else if (token.substr(0, 2) == "q=")
                {
                    tuo::quest.quest_score = boost::lexical_cast<unsigned>(token.substr(2));
                }
                else if (token.substr(0, 2) == "w=")
                {
                    tuo::quest.win_score = boost::lexical_cast<unsigned>(token.substr(2));
                }
                else
                {
                    throw std::runtime_error("Cannot recognize " + token);
                }
            }
            max_possible_score[(size_t)optimization_mode] = tuo::quest.quest_score + tuo::quest.win_score;
        }
        catch (const boost::bad_lexical_cast &e)
        {
            std::cerr << "Error: Expect a number in quest \"" << opt_quest << "\".\n";
            exit(1);
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error: quest " << opt_quest << ": " << e.what() << std::endl;
            exit(1);
        }
    }
#endif

    // std::string your_deck_name{argv[1]};
    auto &&your_deck_list_parsed = parse_deck_list(your_deck_list, decks);
    auto &&enemy_deck_list_parsed = parse_deck_list(enemy_deck_list, decks);

    // Deck* your_deck{nullptr};
    std::vector<Deck *> your_decks;
    std::vector<Deck *> enemy_decks;
    std::vector<long double> your_decks_factors;
    std::vector<long double> enemy_decks_factors;
    for (auto deck_parsed : your_deck_list_parsed)
    {
        Deck *your_deck{nullptr};
        try
        {
            your_deck = find_deck(decks, all_cards, deck_parsed.first)->clone();
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error: Deck " << deck_parsed.first << ": " << e.what() << std::endl;
            exit(1);
        }
        if (your_deck == nullptr)
        {
            std::cerr << "Error: Invalid attack deck name/hash " << deck_parsed.first << ".\n";
            exit(1);
        }
        else if (!your_deck->variable_cards.empty())
        {
            std::cerr << "Error: Invalid attack deck " << deck_parsed.first << ": has optional cards.\n";
            your_deck = nullptr;
        }
        else if (!your_deck->variable_forts.empty())
        {
            std::cerr << "Error: Invalid attack deck " << deck_parsed.first << ": has optional cards.\n";
            your_deck = nullptr;
        }
        if (your_deck == nullptr)
        {
            usage(argc, argv);
            exit(1);
            // return 255;
        }

        your_deck->strategy = opt_your_strategy;
        if (!opt_forts.empty())
        {
            try
            {
                if (!yfpool)
                    your_deck->add_forts(opt_forts + ",");
                else
                    your_deck->add_pool_forts(opt_forts + ",", yfpool);
            }
            catch (const std::runtime_error &e)
            {
                std::cerr << "Error: yfort " << opt_forts << ": " << e.what() << std::endl;
                exit(1);
            }
        }
        if (!opt_doms.empty())
        {
            try
            {
                your_deck->add_dominions(opt_doms + ",", true);
            }
            catch (const std::runtime_error &e)
            {
                std::cerr << "Error: ydom " << opt_doms << ": " << e.what() << std::endl;
                exit(1);
            }
        }

        try
        {
            your_deck->set_vip_cards(opt_vip);
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error: vip " << opt_vip << ": " << e.what() << std::endl;
            exit(1);
        }

        try
        {
            your_deck->set_given_hand(opt_hand);
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error: hand " << opt_hand << ": " << e.what() << std::endl;
            exit(1);
        }

        // add cards from the decks to requirement/inventory
        if (opt_do_optimization || opt_do_reorder)
        {
            if (opt_keep_commander) // TODO this does not work with multi deck mode
            {
                requirement.num_cards[your_deck->commander] = 1;
            }
            for (auto &&card_mark : your_deck->card_marks)
            {
                auto &&card = card_mark.first < 0 ? your_deck->commander : your_deck->cards[card_mark.first];
                auto mark = card_mark.second;
                if ((mark == '!') && ((card_mark.first >= 0) || !opt_keep_commander))
                {
                    requirement.num_cards[card] += 1;
                }
            }
            if (opt_skip_unclaimed_decks)
            {
                // skip decks that can not be build
                if (claim_cards_needed({your_deck->commander}))
                    continue;
                if (claim_cards_needed(your_deck->cards))
                    continue;
                if (your_deck->alpha_dominion && claim_cards_needed({your_deck->alpha_dominion}))
                    continue;
            }
            else if (opt_do_optimization and use_owned_cards)
            {
                // Force to claim cards in your initial deck.
                claim_cards({your_deck->commander});
                claim_cards(your_deck->cards);
                if (your_deck->alpha_dominion)
                    claim_cards({your_deck->alpha_dominion});
            }

            // shrink any oversized deck to maximum of 10 cards + commander
            // NOTE: do this AFTER the call to claim_cards so that passing an initial deck of >10 cards
            //       can be used as a "shortcut" for adding them to owned cards. Also this allows climb
            //       to figure out which are the best 10, rather than restricting climb to the first 10.
            if (your_deck->cards.size() > max_deck_len)
            {
                your_deck->shrink(max_deck_len);
                if (debug_print >= 0)
                {
                    std::cerr << "WARNING: Too many cards in your deck. Trimmed.\n";
                }
            }
        }

        your_decks.push_back(your_deck);
        your_decks_factors.push_back(deck_parsed.second);
    }
    if (your_decks.size() == 0)
    {
        std::cerr << "No deck set. Probably due to 'strict-ownedcards' and a deck that is incompatible to the set 'ownedcards'." << std::endl;
        exit(1);
    }
    target_score = opt_target_score.empty() ? max_possible_score[(size_t)optimization_mode] : boost::lexical_cast<long double>(opt_target_score);

    for (auto deck_parsed : enemy_deck_list_parsed)
    {
        Deck *enemy_deck{nullptr};
        try
        {
            enemy_deck = find_deck(decks, all_cards, deck_parsed.first);
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error: Deck " << deck_parsed.first << ": " << e.what() << std::endl;
            exit(1);
        }
        if (enemy_deck == nullptr)
        {
            std::cerr << "Error: Invalid defense deck name/hash " << deck_parsed.first << ".\n";
            usage(argc, argv);
            exit(1);
        }
        if (optimization_mode == OptimizationMode::notset)
        {
            if (enemy_deck->decktype == DeckType::raid)
            {
                optimization_mode = OptimizationMode::raid;
            }
            else if (enemy_deck->decktype == DeckType::campaign)
            {
                gamemode = surge;
                optimization_mode = OptimizationMode::campaign;
            }
            else
            {
                optimization_mode = OptimizationMode::winrate;
            }
        }
        enemy_deck->strategy = opt_enemy_strategy;
        if (!opt_enemy_doms.empty())
        {
            try
            {
                enemy_deck->add_dominions(opt_enemy_doms + ",", true);
            }
            catch (const std::runtime_error &e)
            {
                std::cerr << "Error: edom " << opt_enemy_doms << ": " << e.what() << std::endl;
                exit(1);
            }
        }
        if (!opt_enemy_forts.empty())
        {
            try
            {
                if (!efpool)
                    enemy_deck->add_forts(opt_enemy_forts + ",");
                else
                    enemy_deck->add_pool_forts(opt_enemy_forts + ",", efpool);
            }
            catch (const std::runtime_error &e)
            {
                std::cerr << "Error: efort " << opt_enemy_forts << ": " << e.what() << std::endl;
                exit(1);
            }
        }
        try
        {
            enemy_deck->set_given_hand(opt_enemy_hand);
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error: enemy:hand " << opt_enemy_hand << ": " << e.what() << std::endl;
            exit(1);
        }
        enemy_decks.push_back(enemy_deck);
        enemy_decks_factors.push_back(deck_parsed.second);
    }

    std::vector<long double> factors((opt_multi_optimization ? 1 : your_decks_factors.size()) * enemy_decks_factors.size());
    for (unsigned i = 0; i < factors.size(); ++i)
    {
        factors[i] = your_decks_factors[i / enemy_decks_factors.size()] * enemy_decks_factors[i % enemy_decks_factors.size()];
    }

    if ((opt_do_optimization || opt_do_reorder) && (your_decks.size() != 1 && !opt_multi_optimization))
    {
        std::cerr << "Optimization only works with a single deck" << std::endl;
        exit(1);
    }

    if (debug_print >= 0)
    {
        for (unsigned i(0); i < your_decks.size(); ++i)
        {
            auto your_deck = your_decks[i];
            std::cout << "Your Deck:" << your_decks_factors[i] << ": " << (debug_print > 0 ? your_deck->long_description() : your_deck->medium_description()) << std::endl;
        }
        for (unsigned bg_effect = PassiveBGE::no_bge; bg_effect < PassiveBGE::num_passive_bges; ++bg_effect)
        {
            auto bge_value = opt_bg_effects[0][bg_effect];
            if (!bge_value)
                continue;
            std::cout << "Your BG Effect: " << passive_bge_names[bg_effect];
            if (bge_value != -1)
                std::cout << " " << bge_value;
            std::cout << std::endl;
        }
        for (const auto &bg_skill : opt_bg_skills[0])
        {
            std::cout << "Your BG Skill: " << skill_description(all_cards, bg_skill) << std::endl;
        }

        for (unsigned i(0); i < enemy_decks.size(); ++i)
        {
            auto enemy_deck = enemy_decks[i];
            std::cout << "Enemy's Deck:" << enemy_decks_factors[i] << ": "
                      << (debug_print > 0 ? enemy_deck->long_description() : enemy_deck->medium_description()) << std::endl;
        }
        for (unsigned bg_effect = PassiveBGE::no_bge; bg_effect < PassiveBGE::num_passive_bges; ++bg_effect)
        {
            auto bge_value = opt_bg_effects[1][bg_effect];
            if (!bge_value)
                continue;
            std::cout << "Enemy's BG Effect: " << passive_bge_names[bg_effect];
            if (bge_value != -1)
                std::cout << " " << bge_value;
            std::cout << std::endl;
        }
        for (const auto &bg_skill : opt_bg_skills[1])
        {
            std::cout << "Enemy's BG Skill: " << skill_description(all_cards, bg_skill) << std::endl;
        }
    }
    if (enemy_decks.size() == 1)
    {
        auto enemy_deck = enemy_decks[0];
        for (auto x_mult_ss : enemy_deck->effects)
        {
            if (debug_print >= 0)
            {
                std::cout << "Enemy's X-Mult BG Skill (effective X = round_up[X * " << enemy_deck->level << "]): "
                          << skill_description(all_cards, x_mult_ss);
                if (x_mult_ss.x)
                {
                    std::cout << " (eff. X = " << ceil(x_mult_ss.x * enemy_deck->level) << ")";
                }
                std::cout << std::endl;
            }
            opt_bg_skills[1].push_back({x_mult_ss.id,
                                        (unsigned)ceil(x_mult_ss.x * enemy_deck->level),
                                        x_mult_ss.y, x_mult_ss.n, x_mult_ss.c,
                                        x_mult_ss.s, x_mult_ss.s2, x_mult_ss.all});
        }
    }
    auto proc_decks = your_decks;
    if (opt_multi_optimization)
    { // only one deck at a time
        proc_decks.erase(proc_decks.begin() + 1, proc_decks.end());
    }
    Process p(opt_num_threads, all_cards, decks, proc_decks, enemy_decks, factors, gamemode,
#ifndef NQUEST
              tuo::quest,
#endif
              opt_bg_effects[0], opt_bg_effects[1], opt_bg_skills[0], opt_bg_skills[1]);

    auto your_deck = your_decks[0];

    auto op = opt_todo.back();
    // for (auto op: opt_todo)
    {
        switch (std::get<2>(op))
        {
        case noop:
            break;
        case simulate:
        {
            EvaluatedResults results = {EvaluatedResults::first_type(enemy_decks.size() * your_decks.size()), 0};
            results = p.evaluate(std::get<0>(op), results);
            print_results(results, p.factors);
            fr = std::make_pair(your_deck->clone(), compute_score(results, p.factors));
            print_sim_card_values(fr.first, p, std::get<0>(op));
            break;
        }
        case climb_forts:
        {
            fr = forts_climbing(std::get<0>(op), p);
            break;
        }
        case climb:
        {
            // TODO check for your_decks.size()==1
            fr = hill_climbing(std::get<0>(op), std::get<1>(op), your_decks, p, requirement
#ifndef NQUEST
                               ,
                               tuo::quest
#endif
            );
            break;
        }
        case anneal:
        {
            // TODO check for your_decks.size()==1
            fr = simulated_annealing(std::get<0>(op), std::get<1>(op), your_decks, p, requirement
#ifndef NQUEST
                                     ,
                                     tuo::quest
#endif
            );
            break;
        }
        case genetic:
        {
            fr = genetic_algorithm(std::get<0>(op), std::get<1>(op), your_decks, p, requirement
#ifndef NQUEST
                                   ,
                                   tuo::quest
#endif
            );
            break;
        }

        case beam:
        {
            fr = beam_climb(std::get<0>(op), std::get<1>(op), your_decks, p, requirement
#ifndef NQUEST
                            ,
                            tuo::quest
#endif
            );
            break;
        }
        case reorder:
        {
            // TODO multi deck mode for reorder
            your_deck->strategy = DeckStrategy::ordered;
            use_owned_cards = true;
            use_top_level_card = false;
            use_top_level_commander = false;
            use_owned_dominions = false;
            if (min_deck_len == 1 && max_deck_len == 10)
            {
                min_deck_len = max_deck_len = your_deck->cards.size();
            }
            fund = 0;
            debug_print = -1;
            owned_cards.clear();
            claim_cards({your_deck->commander});
            claim_cards(your_deck->cards);
            std::vector<Deck *> single_deck = {your_deck};
            fr = hill_climbing(std::get<0>(op), std::get<1>(op), single_deck, p, requirement
#ifndef NQUEST
                               ,
                               tuo::quest
#endif
            );
            break;
        }
            // fr=nllptr no return
        case debug:
        {
            ++debug_print;
            debug_str.clear();
            EvaluatedResults results{EvaluatedResults::first_type(enemy_decks.size()), 0};
            results = p.evaluate(1, results);
            print_results(results, p.factors);
            --debug_print;
            break;
        }
            // fr=nllptr no return
        case debuguntil:
        {
            ++debug_print;
            ++debug_cached;
            while (true)
            {
                debug_str.clear();
                EvaluatedResults results{EvaluatedResults::first_type(enemy_decks.size()), 0};
                results = p.evaluate(1, results);
                auto score = compute_score(results, p.factors);
                if (score.points >= std::get<0>(op) && score.points <= std::get<1>(op))
                {
                    std::cout << debug_str << std::flush;
                    print_results(results, p.factors);
                    break;
                }
            }
            --debug_cached;
            --debug_print;
            break;
        }
        }
    }
    write_db(prefix);
    return fr;
}

std::vector<const char *> strlist(std::vector<std::string> &input)
{
    std::vector<const char *> result;

    // remember the nullptr terminator
    result.reserve(input.size() + 1);

    for (auto &i : input)
        result.push_back(i.data());
    result.push_back(nullptr);
    return result;
}

DeckResults start(int argc, const char **argv)
{
    std::vector<std::string> result_decks;
    DeckResults drc;
    bool first = true;
    result_decks.push_back(argv[1]);
    for (int j = 0; j < argc; ++j)
    {
        int add_j_inc = 0;
        bool do_it = false;
        std::stringstream param_file;
        if (strcmp(argv[j], "boost-ml") == 0)
        {
            do_it = true;
            add_j_inc = -1;
            param_file << "noop" << std::endl
                       << "only-ml" << std::endl
                       << "deck @1@ no-ml" << std::endl;
        }
        if (strcmp(argv[j], "-p") == 0 || strcmp(argv[j], "params") == 0)
        {
            if (j + 1 < argc)
            {
                std::ifstream in_param_file(argv[j + 1]);
                if (in_param_file.good())
                {
                    param_file << in_param_file.rdbuf();
                    in_param_file.close();
                    do_it = true;
                }
                else
                {
                    std::cout << "Error loading params file " << argv[j + 1] << std::endl;
                }
            }
            else
            {
                std::cout << "-p/params needs a file as parameter" << std::endl;
            }
        }
        if (do_it)
        {
            {
                param_file.clear();
                param_file.seekg(0, std::ios::beg);
                // param_file to istream
                std::cout << "Loading params file " << argv[j + 1] << std::endl;
                std::string line, tmp, first_line = "";
                std::vector<std::string> first_split, cur_split;
                // first count parameters in file
                while (param_file && !param_file.eof())
                {
                    std::getline(param_file, line);
                    if (is_line_empty_or_commented(line))
                        continue;
                    for (int z = 0; z < argc - j - 1; ++z)
                    {
                        std::string rep("$" + std::to_string(z) + "$");
                        if (line.find(rep) != std::string::npos && z > add_j_inc)
                            add_j_inc = z;
                    }
                }
                param_file.clear();
                param_file.seekg(0, std::ios::beg);
                while (param_file && !param_file.eof())
                {
                    std::getline(param_file, line);
                    if (is_line_empty_or_commented(line))
                        continue;
                    // std::cout << line << std::endl;
                    if (first_line == "")
                    {
                        first_line = line;
                        std::istringstream ss(first_line);
                        while (ss >> boost::io::quoted(tmp))
                        {
                            for (unsigned z = 0; z < result_decks.size(); ++z)
                            {
                                boost::replace_all(tmp, "@" + std::to_string(z) + "@", result_decks[z]);
                            }
                            for (int z = 0; z < argc - j - 1; ++z)
                            {
                                std::string rep("$" + std::to_string(z) + "$");
                                boost::replace_all(tmp, rep, std::string(argv[j + 1 + z]));
                            }
                            first_split.push_back(tmp);
                        }
                    }
                    else
                    {
                        cur_split = first_split;
                        std::istringstream ss(line);
                        while (ss >> boost::io::quoted(tmp))
                        {
                            for (unsigned z = 0; z < result_decks.size(); ++z)
                            {
                                boost::replace_all(tmp, "@" + std::to_string(z) + "@", result_decks[z]);
                            }
                            for (int z = 0; z < argc - j - 1; ++z)
                            {
                                std::string rep("$" + std::to_string(z) + "$");
                                boost::replace_all(tmp, rep, std::string(argv[j + 1 + z]));
                            }
                            cur_split.push_back(tmp);
                        }
                        for (int i = 0; i < argc; ++i)
                        {
                            if (i < j)
                                cur_split.insert(cur_split.begin() + i, std::string(argv[i]));
                            if (i > j + 1 + add_j_inc)
                                cur_split.push_back(std::string(argv[i]));
                        }
                        if (!first)
                        {
                            // replace passed deck with most recent deck
                            cur_split.erase(cur_split.begin() + 1);
                            cur_split.insert(cur_split.begin() + 1, result_decks[result_decks.size() - 1]);
                        }
                        first = false;

                        std::cout << std::endl
                                  << "///////////////" << std::endl;
                        // std::cout << result_decks[result_decks.size()-1] <<std::endl;
                        int k = 0;
                        for (auto &str : cur_split)
                        {
                            if (k > 0)
                                std::cout << "\"";
                            std::cout << str;
                            if (k > 0)
                                std::cout << "\"";
                            std::cout << ' ';
                            k++;
                        }
                        std::cout << std::endl;
                        std::cout << "///////////////" << std::endl
                                  << std::endl;

                        drc = start(cur_split.size(), strlist(cur_split).data());

                        // result to string
                        std::stringstream oss;
                        if (drc.first->commander)
                            oss << drc.first->commander->m_name << ", ";
                        if (drc.first->alpha_dominion)
                            oss << drc.first->alpha_dominion->m_name << ", ";
                        print_cards_inline(drc.first->cards, oss, drc.first);
                        std::string decks(oss.str());
                        std::replace(decks.begin(), decks.end(), '\n', ' ');
                        result_decks.push_back(decks);

                        // print_cards_inline(drc.first->cards,std::cout);
                    }
                }
                return drc;
            }
        }
        else
        {
            // return run(argc,argv);
        }
    }
    init();
    auto rtrn = run(argc, argv);
    return rtrn;
}

#if !defined(TEST)
int main(int argc, const char **argv)
{
#ifndef NTIMER
    boost::timer::auto_cpu_timer t;
#endif
    if (argc == 2 && strcmp(argv[1], "-version") == 0)
    {
        std::cout << "Tyrant Unleashed Optimizer " << TYRANT_OPTIMIZER_VERSION << std::endl;
        return 0;
    }
    if (argc < 2)
    {
        usage(argc, argv);
        return 255;
    }
    // init();
    start(argc, argv);
    return 0;
}
#endif
