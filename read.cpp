#include "read.h"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <cstring>
#include <map>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <exception>

#include "tyrant.h"
#include "card.h"
#include "cards.h"
#include "deck.h"

// trim from start
static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

template<typename Iterator, typename Functor> Iterator advance_until(Iterator it, Iterator it_end, Functor f)
{
    while(it != it_end)
    {
        if(f(*it))
        {
            break;
        }
        ++it;
    }
    return(it);
}

// take care that "it" is 1 past current.
template<typename Iterator, typename Functor> Iterator recede_until(Iterator it, Iterator it_beg, Functor f)
{
    if(it == it_beg) { return(it_beg); }
    --it;
    do
    {
        if(f(*it))
        {
            return(++it);
        }
        --it;
    } while(it != it_beg);
    return(it_beg);
}

template<typename Iterator, typename Functor, typename Token> Iterator read_token(Iterator it, Iterator it_end, Functor f, Token& token)
{
    Iterator token_start = advance_until(it, it_end, [](const char& c){return(c != ' ');});
    Iterator token_end_after_spaces = advance_until(token_start, it_end, f);
    if(token_start != token_end_after_spaces)
    {
        Iterator token_end = recede_until(token_end_after_spaces, token_start, [](const char& c){return(c != ' ');});
        token = boost::lexical_cast<Token>(std::string{token_start, token_end});
    }
    return(token_end_after_spaces);
}

DeckList & normalize(DeckList & decklist)
{
    long double factor_sum = 0;
    for (const auto & it : decklist)
    {
        factor_sum += it.second;
    }
    if (factor_sum > 0)
    {
        for (auto & it : decklist)
        {
            it.second /= factor_sum;
        }
    }
    return decklist;
}

DeckList expand_deck_to_list(std::string deck_name, Decks& decks)
{
    static std::unordered_set<std::string> expanding_decks;
    if (expanding_decks.count(deck_name))
    {
        std::cerr << "Warning: circular referred deck: " << deck_name << std::endl;
        return DeckList();
    }
    auto deck_string = deck_name;
    Deck* deck = decks.find_deck_by_name(deck_name);
    if (deck != nullptr)
    {
        deck_string = deck->deck_string;
        if (deck_string.find_first_of(";:") != std::string::npos || decks.find_deck_by_name(deck_string) != nullptr)
        {
            // deck_name refers to a deck list
            expanding_decks.insert(deck_name);
            auto && decklist = parse_deck_list(deck_string, decks);
            expanding_decks.erase(deck_name);
            return normalize(decklist);
        }
    }

    if (deck_string.length() >= 3 && deck_string.front() == '/' && deck_string.back() == '/')
    {
        // deck_name is, or refers to, a regex
        DeckList res;
        std::string regex_string(deck_string, 1, deck_string.length() - 2);
        boost::regex regex(regex_string);
        boost::smatch smatch;
        expanding_decks.insert(deck_name);
        for (const auto & deck_it: decks.by_name)
        {
            if (boost::regex_search(deck_it.first, smatch, regex))
            {
                auto && decklist = expand_deck_to_list(deck_it.first, decks);
                for (const auto & it : decklist)
                {
                    res[it.first] += it.second;
                }
            }
        }
        expanding_decks.erase(deck_name);
        if (res.size() == 0)
        {
            std::cerr << "Warning: regular expression matches nothing: /" << regex_string << "/." << std::endl;
        }
        return normalize(res);
    }
    else
    {
        return {{deck_name, 1}};
    }
}

DeckList parse_deck_list(std::string list_string, Decks& decks)
{
    DeckList res;
    boost::tokenizer<boost::char_delimiters_separator<char>> list_tokens{list_string, boost::char_delimiters_separator<char>{false, ";", ""}};
    for(const auto & list_token : list_tokens)
    {
        boost::tokenizer<boost::char_delimiters_separator<char>> deck_tokens{list_token, boost::char_delimiters_separator<char>{false, ":", ""}};
        auto deck_token = deck_tokens.begin();
        auto deck_name = *deck_token;
        double factor = 1.0;
        ++ deck_token;
        if (deck_token != deck_tokens.end())
        {
            try
            {
                factor = boost::lexical_cast<long double>(*deck_token);
            }
            catch (const boost::bad_lexical_cast & e)
            {
                std::cerr << "Warning: Is ':' a typo? Skip deck [" << list_token << "]\n";
                continue;
            }
        }
        auto && decklist = expand_deck_to_list(deck_name, decks);
        for (const auto & it : decklist)
        {
            res[it.first] += it.second * factor;
        }
    }
    return res;
}

void parse_card_spec(const Cards& all_cards, const std::string& card_spec, unsigned& card_id, unsigned& card_num, char& num_sign, char& mark)
{
//    static std::set<std::string> recognized_abbr;
    auto card_spec_iter = card_spec.begin();
    card_id = 0;
    card_num = 1;
    num_sign = 0;
    mark = 0;
    std::string card_name;
    card_spec_iter = read_token(card_spec_iter, card_spec.end(), [](char c){return(c=='#' || c=='(' || c=='\r');}, card_name);
    if(card_name[0] == '!')
    {
        mark = card_name[0];
        card_name.erase(0, 1);
    }
    // If card name is not found, try find card id quoted in '[]' in name, ignoring other characters.
    std::string simple_name{simplify_name(card_name)};
    const auto && abbr_it = all_cards.player_cards_abbr.find(simple_name);
    if(abbr_it != all_cards.player_cards_abbr.end())
    {
//        if(recognized_abbr.count(card_name) == 0)
//        {
//            std::cout << "Recognize abbreviation " << card_name << ": " << abbr_it->second << std::endl;
//            recognized_abbr.insert(card_name);
//        }
        simple_name = simplify_name(abbr_it->second);
    }
    auto card_it = all_cards.cards_by_name.find(simple_name);
    auto card_id_iter = advance_until(simple_name.begin(), simple_name.end(), [](char c){return(c=='[');});
    if (card_it != all_cards.cards_by_name.end())
    {
        card_id = card_it->second->m_id;
        if (all_cards.ambiguous_names.count(simple_name))
        {
            std::cerr << "Warning: There are multiple cards named " << card_name << " in cards.xml. [" << card_id << "] is used.\n";
        }
    }
    else if(card_id_iter != simple_name.end())
    {
        ++ card_id_iter;
        card_id_iter = read_token(card_id_iter, simple_name.end(), [](char c){return(c==']');}, card_id);
    }
    if(card_spec_iter != card_spec.end() && (*card_spec_iter == '#' || *card_spec_iter == '('))
    {
        ++card_spec_iter;
        if(card_spec_iter != card_spec.end())
        {
           if(strchr("+-$", *card_spec_iter))
           {
               num_sign = *card_spec_iter;
               ++card_spec_iter;
           }
        }
        card_spec_iter = read_token(card_spec_iter, card_spec.end(), [](char c){return(c < '0' || c > '9');}, card_num);
    }
    if(card_id == 0)
    {
        throw std::runtime_error("Unknown card: " + card_name);
    }
}

const std::pair<std::vector<unsigned>, std::map<signed, char>> string_to_ids(const Cards& all_cards, const std::string& deck_string, const std::string & description)
{
    std::vector<unsigned> card_ids;
    std::map<signed, char> card_marks;
    std::vector<std::string> error_list;
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
            error_list.push_back(e.what());
            continue;
        }
    }
    if (! card_ids.empty())
    {
        if (! error_list.empty())
        {
            std::cerr << "Warning: Ignore some cards while resolving " << description << ": ";
            for (auto error: error_list)
            {
                std::cerr << '[' << error << ']';
            }
            std::cerr << std::endl;
        }
        return {card_ids, card_marks};
    }
    try
    {
        hash_to_ids(deck_string.c_str(), card_ids);
        for (auto & card_id: card_ids)
        {
            try
            {
                all_cards.by_id(card_id);
            }
            catch(std::exception& e)
            {
                throw std::runtime_error(std::string("Deck not found. Error to treat as hash: ") + e.what());
            }
        }
    }
    catch(std::exception& e)
    {
        std::cerr << "Error: Failed to resolve " << description << ": " << e.what() << std::endl;
        throw;
    }
    return {card_ids, card_marks};
}

unsigned read_card_abbrs(Cards& all_cards, const std::string& filename)
{
    if(!boost::filesystem::exists(filename))
    {
        return(0);
    }
    std::ifstream abbr_file(filename);
    if(!abbr_file.is_open())
    {
        std::cerr << "Error: Card abbreviation file " << filename << " could not be opened\n";
        return(2);
    }
    unsigned num_line(0);
    abbr_file.exceptions(std::ifstream::badbit);
    try
    {
        while(abbr_file && !abbr_file.eof())
        {
            std::string abbr_string;
            getline(abbr_file, abbr_string);
            trim(abbr_string);
            ++num_line;
            if(abbr_string.size() == 0 || strncmp(abbr_string.c_str(), "//", 2) == 0)
            {
                continue;
            }
            std::string abbr_name;
            auto abbr_string_iter = read_token(abbr_string.begin(), abbr_string.end(), [](char c){return(c == ':');}, abbr_name);
            if(abbr_string_iter == abbr_string.end() || abbr_name.empty())
            {
                std::cerr << "Error in card abbreviation file " << filename << " at line " << num_line << ", could not read the name.\n";
                continue;
            }
            abbr_string_iter = advance_until(abbr_string_iter + 1, abbr_string.end(), [](const char& c){return(c != ' ');});
            if(all_cards.cards_by_name.find(abbr_name) != all_cards.cards_by_name.end())
            {
                std::cerr << "Warning in card abbreviation file " << filename << " at line " << num_line << ": ignored because the name has been used by an existing card." << std::endl;
            }
            else
            {
                all_cards.player_cards_abbr[simplify_name(abbr_name)] = std::string{abbr_string_iter, abbr_string.end()};
            }
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception while parsing the card abbreviation file " << filename;
        if(num_line > 0)
        {
            std::cerr << " at line " << num_line;
        }
        std::cerr << ": " << e.what() << ".\n";
        return(3);
    }
    return(0);
}


// Error codes:
// 2 -> file not readable
// 3 -> error while parsing file
unsigned load_custom_decks(Decks& decks, Cards& all_cards, const std::string & filename)
{
    if (!boost::filesystem::exists(filename))
    {
        return 0;
    }
    std::ifstream decks_file(filename);
    if (!decks_file.is_open())
    {
        std::cerr << "Error: Custom deck file " << filename << " could not be opened\n";
        return 2;
    }
    unsigned num_line(0);
    decks_file.exceptions(std::ifstream::badbit);
    try
    {
        while(decks_file && !decks_file.eof())
        {
            std::string deck_string;
            getline(decks_file, deck_string);
            trim(deck_string);
            ++num_line;
            if(deck_string.size() == 0 || strncmp(deck_string.c_str(), "//", 2) == 0)
            {
                continue;
            }
            std::string deck_name;
            auto deck_string_iter = read_token(deck_string.begin(), deck_string.end(), [](char c){return(strchr(":,", c));}, deck_name);
            if(deck_string_iter == deck_string.end() || deck_name.empty())
            {
                std::cerr << "Error in custom deck file " << filename << " at line " << num_line << ", could not read the deck name.\n";
                continue;
            }
            deck_string_iter = advance_until(deck_string_iter + 1, deck_string.end(), [](const char& c){return(c != ' ');});
            Deck* deck = decks.find_deck_by_name(deck_name);
            if (deck != nullptr)
            {
                std::cerr << "Warning in custom deck file " << filename << " at line " << num_line << ", name conflicts, overrides " << deck->short_description() << std::endl;
            }
            decks.decks.push_back(Deck{all_cards, DeckType::custom_deck, num_line, deck_name});
            deck = &decks.decks.back();
            deck->set(std::string{deck_string_iter, deck_string.end()});
            decks.add_deck(deck, deck_name);
            std::stringstream alt_name;
            alt_name << decktype_names[deck->decktype] << " #" << deck->id;
            decks.add_deck(deck, alt_name.str());
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception while parsing the custom deck file " << filename;
        if(num_line > 0)
        {
            std::cerr << " at line " << num_line;
        }
        std::cerr << ": " << e.what() << ".\n";
        return 3;
    }
    return(0);
}

void add_owned_card(Cards& all_cards, std::map<unsigned, unsigned>& owned_cards, std::string& card_spec)
{
    unsigned card_id{0};
    unsigned card_num{1};
    char num_sign{0};
    char mark{0};
    parse_card_spec(all_cards, card_spec, card_id, card_num, num_sign, mark);
    all_cards.by_id(card_id); // check that the id is valid
    assert(mark == 0);
    if(num_sign == 0)
    {
        owned_cards[card_id] = card_num;
    }
    else if(num_sign == '+')
    {
        owned_cards[card_id] += card_num;
    }
    else if(num_sign == '-')
    {
        owned_cards[card_id] = owned_cards[card_id] > card_num ? owned_cards[card_id] - card_num : 0;
    }
}

void read_owned_cards(Cards& all_cards, std::map<unsigned, unsigned>& owned_cards, const std::string & filename)
{
    std::ifstream owned_file{filename};
    if(!owned_file.good())
    {
        // try parse the string as a cards instead of as a filename
        try
        {
            std::string card_list(filename);
            boost::tokenizer<boost::char_delimiters_separator<char>> card_tokens{card_list, boost::char_delimiters_separator<char>{false, ",", ""}};
            auto token_iter = card_tokens.begin();
            for (; token_iter != card_tokens.end(); ++token_iter)
            {
                std::string card_spec(*token_iter);
                add_owned_card(all_cards, owned_cards, card_spec);
            }
        } 
        catch (std::exception& e)
        {
            std::cerr << "Error: Failed to parse owned cards: '" << filename << "' is neither a file nor a valid set of cards (" << e.what() << ")" << std::endl;
            exit(0);
        }
        return;
    }
    unsigned num_line(0);
    while(owned_file && !owned_file.eof())
    {
        std::string card_spec;
        getline(owned_file, card_spec);
        trim(card_spec);
        ++num_line;
        if(card_spec.size() == 0 || strncmp(card_spec.c_str(), "//", 2) == 0)
        {
            continue;
        }
        try
        {
            add_owned_card(all_cards, owned_cards, card_spec);
        }
        catch(std::exception& e)
        {
            std::cerr << "Error in owned cards file " << filename << " at line " << num_line << " while parsing card '" << card_spec << "': " << e.what() << "\n";
        }
    }
}

unsigned read_bge_aliases(std::unordered_map<std::string, std::string> & bge_aliases, const std::string& filename)
{
    if(!boost::filesystem::exists(filename))
    {
        return(0);
    }
    std::ifstream bgefile(filename);
    if(!bgefile.is_open())
    {
        std::cerr << "Error: BGE file " << filename << " could not be opened\n";
        return(2);
    }
    unsigned num_line(0);
    bgefile.exceptions(std::ifstream::badbit);
    try
    {
        while(bgefile && !bgefile.eof())
        {
            std::string bge_string;
            getline(bgefile, bge_string);
            ++num_line;
            if(bge_string.size() == 0 || strncmp(bge_string.c_str(), "//", 2) == 0)
            {
                continue;
            }
            std::string bge_name;
            auto bge_string_iter = read_token(bge_string.begin(), bge_string.end(), [](char c){return(c == ':');}, bge_name);
            if(bge_string_iter == bge_string.end() || bge_name.empty())
            {
                std::cerr << "Error in BGE file " << filename << " at line " << num_line << ", could not read the name.\n";
                continue;
            }
            bge_string_iter = advance_until(bge_string_iter + 1, bge_string.end(), [](const char& c){return(c != ' ');});
            bge_aliases[simplify_name(bge_name)] = std::string{bge_string_iter, bge_string.end()};
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception while parsing the BGE file " << filename;
        if(num_line > 0)
        {
            std::cerr << " at line " << num_line;
        }
        std::cerr << ": " << e.what() << ".\n";
        return(3);
    }
    return(0);
}

