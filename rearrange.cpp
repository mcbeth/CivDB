#include "db.h"
#include "dbUtils.h"

//#include <boost/algorithm/string.hpp>
//#include <boost/serialization/singleton.hpp> // has nothing to do with s11n
//
//#include <boost/lexical_cast.hpp>
//#include <boost/filesystem.hpp>
//#include <boost/filesystem/fstream.hpp>
//#include <boost/foreach.hpp>
//
//#include <boost/bind.hpp>
//#include <boost/ref.hpp>
//
//#include <boost/assign.hpp>
//using namespace boost::assign;
//
//namespace fs = boost::filesystem;
//
//#include <boost/scoped_ptr.hpp>
#include <iostream>
#include <sstream>
#include <string>

bool FixDeck(Game &g, std::deque<int> &cards)
{
	int d = cards[0];
	cards.pop_front();
	Deck deck;
	Hand discard;

	for(int i = 0; i < cards.size(); i++)
	{
		deck.push_back(g._decks[d][cards[i]]);	
	}
	for(int i = 0; i < g._decks[d].size(); i++)
	{
		if (std::find(cards.begin(), cards.end(), i) == cards.end())
		{
			std::cout << "Discarding " << i << " " << g._decks[d][i]->_name << std::endl;
			discard.insert(g._decks[d][i]);
		}
	}

	g._decks[d] = deck;
	g._discards[d].insert(discard.begin(), discard.end());

	return true;
}

int main(int argc, char *argv[])
{
	std::string cmd(argv[0]);
	std::cout << cmd << std::endl;
	if (argc != 2)
	{
		std::cerr << argv[0] << " dbName" << std::endl;
		return ErrQuit;
	}
	Game g(argv[1]);
	while(std::cin.good())
	{
		std::string line;
		std::cout << "> ";
		std::getline(std::cin, line);
		
		if (line.size())
		{
			if (line[0] == 'p')
			{
				std::stringstream ss(line);
				char c;
				ss >> c;
				if (line.size() == 2)
				{
					int deck = line[1]-'0';
					if (deck > 9)
					{
						deck = line[1]-'a'+1;
						std::cout << deck << std::endl;
						if (deck > 9)
							continue;
						for(auto i = g._discards[deck].begin(); i != g._discards[deck].end(); i++)
						{
							std::cout << (*i)->_name << std::endl;
						}
					}
					else
					for(int i = 0; i < g._decks[deck].size(); i++)
					{
						std::cout << i << '\t' << g._decks[deck][i]->_name << std::endl;
					}
				}
			}
			if (line[0] == 'r')
			{
				if (line.size() > 2)
				{
					std::stringstream ss(line);
					char c;
					ss >> c;
					std::deque<int> cards;
					while(ss.good())
					{
						int v = -1;
						ss >> v;
						if (v > -1 || ss.good())
							cards.push_back(v);
					}	
					if (!FixDeck(g, cards))
					{
						g.Abandon();
						std::cerr << "I can't understand you, collapsing\n";
						return ErrQuit;
					}
				}
			}
			if (line[0] == 'q')
			{
				return ErrQuit;
			}
			if (line[0] == 'a')
			{
				g.Abandon();
				return ErrQuit;
			}
		}
	}
	return ErrNone;
}
