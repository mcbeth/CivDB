#include "db.h"
#include "dbUtils.h"
#include "parser.h"
#include "factory.h"

#include <boost/algorithm/string.hpp>
#include <boost/serialization/singleton.hpp> // has nothing to do with s11n

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>

namespace fs = boost::filesystem;

#include <readline/readline.h>
#include <readline/history.h>

#include <boost/scoped_ptr.hpp>
#include <iostream>

const std::string version("0.36");
//typedef std::map<std::string, std::string> HelpText;

boost::scoped_ptr<Game> s_g;
//boost::scoped_ptr<HelpText> ht(new HelpText);
std::string g_currentPower;
int g_currentDeck(0);

typedef boost::function<char **(const std::vector<std::string> &, const char *, int)> CompleteFunc;
typedef FactoryOwner<CompleteFunc> Completer;
typedef boost::serialization::singleton<Completer> CompleteFactory;

#define REG_COMP(_trigger, _func) \
	static bool _trigger ## _complete_registered = \
		CompleteFactory::get_mutable_instance().Register(#_trigger, _func)

const char *objectList[] = {"powers","players","decks","discards","calamities"};
const char *rulesList[] = {"AdvCiv","CivProject30"};

char *fillBuffer(const std::string &name, const std::string &text, int &state)
{
	const std::string escapedName = boost::replace_all_copy(name, " ", "\\ ");
	const int len = escapedName.size()+1;
	
	if (boost::istarts_with(name, text))
	{
		if (state <= 0)
		{
			char *c = (char *)std::malloc(len);
			std::strncpy(c, escapedName.c_str(),len);
			return c;		
		}
		else
		{
			state--;
		}
	}
	return NULL;
}

char *fillFromArray(const std::string &text, int state, const char **list, const int numValues)
{
	int skip = state;

	for(int i = 0; i < numValues; ++i)
	{
		char *value = NULL;
		if (value = fillBuffer(list[i], text, skip))
			return value;
	}
	return NULL;
}

char *fillFromComplete(const char *text, int state)
{
	int skip = state;

	const Completer &c = CompleteFactory::get_const_instance();
	
	BOOST_FOREACH(auto i, c)
	{
		char *value = NULL;
		if (value = fillBuffer(i.first, text, skip))
			return value;
	}

	return NULL;
}

char *countryFill(const char *text, int state)
{
	const int matchLen = std::strlen(text);
	
	BOOST_FOREACH(auto i, s_g->_powers)
	{
		char *value = NULL;
		if (value = fillBuffer(i.first->_name, text, state))
			return value;
	}
	return NULL;
}

char *setFill(const char *text, int state)
{
	BOOST_FOREACH(auto i, s_g->_vars)
	{
		char *value = NULL;
		if (value = fillBuffer(i.first, text, state))
			return value;
	}
	return NULL;
}

char *typeFill(const char *text, int state)
{
	const char *numbers[] = {"0","-1","-2","-3"};
	const int len = sizeof(numbers)/sizeof(const char *);
	for(int i =0 ; i < len; ++i)
	{
		char *value = NULL;
		if (value = fillBuffer(numbers[i], text, state))
			return value;
	}
	return NULL;
}

int countMatches(const std::string &text, const std::string &match)
{
	int matches = 0;
	int pos = text.find(match);
	while (pos != std::string::npos)
	{
		++matches;
		pos = text.find(match, pos+1);
	}

	return matches;
}

char *cardFill(const char *text, int state)
{
	Powers::const_iterator power = s_g->FindPower(g_currentPower);
	const std::string buffer(rl_line_buffer,rl_point);

	if (power != s_g->_powers.end())
	{
		BOOST_FOREACH(auto i, power->first->_hand)
		{
			if (char *value = fillBuffer(i->_name, text, state))
				return value;
		}
	} else
	{
		BOOST_FOREACH(auto i, s_g->_cards)
		{
			if (g_currentDeck && i->_deck != g_currentDeck)
				continue;
			char *value = NULL;
			if (value = fillBuffer(i->_name, text, state))
				return value;
		}
	}
	return NULL;
}

char *numberFill(const char *text, int state)
{
	const char *numbers[] = {"1","2","3","4","5","6","7","8","9"};
	const int len = sizeof(numbers)/sizeof(const char *);
	for(int i =0 ; i < len; ++i)
	{
		char *value = NULL;
		if (value = fillBuffer(numbers[i], text, state))
			return value;
	}
	return NULL;
}

char *listFill(const char *text, int state)
{
	const int len = sizeof(objectList)/sizeof(const char *);
	for(int i = 0; i < len; ++i)
	{
		char *value = NULL;
		if (value = fillBuffer(objectList[i], text, state))
			return value;
	}
	return NULL;
}

char *augmentedHandFill(const char *text, int state)
{
	static int oldVisits = 0;
	static int visits = -1;
	char *value = NULL;
	
	++visits;
	
	if (value = countryFill(text, state))
		return value;
	else
		if (!oldVisits)
			oldVisits = visits;
	
	if (value = numberFill(text, state-oldVisits))
		return value;
	
	oldVisits = visits = 0;
	return NULL;
}

char *augmentedTradeFill(const char *text, int state)
{
	static int oldVisits = 0;
	static int visits = -1;
	char *value = NULL;

	++visits;

	if (value = countryFill(text, state))
		return value;
	else if (!oldVisits)
		oldVisits = visits;

	if (value = cardFill(text, state-oldVisits))
		return value;

	oldVisits = visits = 0;
	return NULL;
}

char **completeHelp(const std::vector<std::string> &, const char *text, int depth)
{
	switch (depth)
	{
		case 1:
		{
			return rl_completion_matches(text, fillFromComplete);
			break;
		}
	}
	return NULL;
}

char *rulesFill(const char *text, int state)
{
	const int numValues = sizeof(rulesList)/sizeof(const char *);
	return fillFromArray(text, state, rulesList, numValues);
}

char **completeCreate(const std::vector<std::string> &, const char *text, int depth)
{
	switch (depth)
	{
		case 1:
		case 2:
		{
			return  rl_completion_matches(text, rl_filename_completion_function);
			break;
		}
		case 3:
		{
			return rl_completion_matches(text, rulesFill);
			break;
		}
	}
	return NULL;
}

char **completeDiscard(const std::vector<std::string> &data, const char *text, int depth)
{
	switch(depth)
	{
		case 1:
			return rl_completion_matches(text, countryFill);
			break;
		default:
		{
			g_currentPower = data[1];
			char **value = rl_completion_matches(text, cardFill);
			g_currentPower = std::string();
			return value;
		}
			break;
	}
	return NULL;
}

char **completeDraw(const std::vector<std::string> &, const char *text, int depth)
{
	switch(depth)
	{
		case 1:
			return rl_completion_matches(text, countryFill);
			break;
		default:
			return rl_completion_matches(text, numberFill);
			break;
	}
	return NULL;
}


char **completeExport(const std::vector<std::string> &, const char *text, int depth)
{
	switch(depth)
	{
		case 1:
			return rl_completion_matches(text, rl_filename_completion_function);
	}
	return NULL;
}

char **completeHeld(const std::vector<std::string> &, const char *text, int depth)
{
	switch(depth)
	{
		case 1:
			return rl_completion_matches(text,augmentedHandFill);
	}
	return NULL;
}

char **completeSetPlayer(const std::vector<std::string> &, const char *text, int depth)
{
	switch(depth)
	{
		case 1:
			return rl_completion_matches(text,countryFill);
			break;
		case 2:
			break;
	}
	return NULL;
}

char **completeSet(const std::vector<std::string> &, const char *text, int depth)
{
	switch(depth)
	{
		case 1:
			return rl_completion_matches(text,setFill);
			break;
	}
	return NULL;
}

char **completeInsert(const std::vector<std::string> &data, const char *text, int depth)
{
	switch(depth)
	{
		case 1:
		{
			return rl_completion_matches(text, numberFill);
		}
		case 2:
		{
			g_currentDeck = boost::lexical_cast<int>(data[1]);
			char **value = rl_completion_matches(text, cardFill);
			g_currentDeck = 0;
			return value;
		}
	}
	return NULL;
}

char **completeTrade(const std::vector<std::string> &data, const char *text, int depth)
{
	switch(depth)
	{
		case 1:
		{
			return rl_completion_matches(text, countryFill);
		}
		case 2:
		case 3:
		case 4:
		{
			g_currentPower = data[1];
			char **value = rl_completion_matches(text, cardFill);
			g_currentPower = std::string();
			return value;
		}
		default:
		{
			Powers::const_iterator p2 = s_g->_powers.end();
			for(int i = 5; i < depth; ++i)
			{
				p2 = s_g->FindPower(data[i]);
				if (p2 != s_g->_powers.end())
				{
					g_currentPower = p2->first->_name;
					char **value = rl_completion_matches(text, cardFill);
					g_currentPower = std::string();
					return value;
				}
			}

			g_currentPower = data[1];
			char **value = rl_completion_matches(text, augmentedTradeFill);
			g_currentPower = std::string();
			return value;
		}
	}

	return NULL;
}

char **completeValue(const std::vector<std::string> &data, const char *text, int)
{
	return rl_completion_matches(text, cardFill);
}

char **completeShuffleIn(const std::vector<std::string> &data, const char *text, int depth)
{
	switch(depth)
	{
		case 1:
		case 3:
			return rl_completion_matches(text, numberFill);
		case 2:
			return rl_completion_matches(text, typeFill);
		case 4:
			return NULL;
	}
	
	return NULL;
}

char **completeList(const std::vector<std::string> &data, const char *text, int depth)
{
	switch(depth)
	{
		case 1:
		return rl_completion_matches(text, listFill);
		break;
	}
	return NULL;
}

char **completeGive(const std::vector<std::string> &data, const char *text, int depth)
{
	switch(depth)
	{
		case 1:
		case 2:
			return rl_completion_matches(text, countryFill);
			break;
		case 3:
		{
			g_currentPower = data[1];
			char **value = rl_completion_matches(text, cardFill);
			g_currentPower = std::string();
			return value;
		}
	}
}

char **completeNULL(const std::vector<std::string> &, const char *, int)
{
	return NULL;
}

REG_COMP(Create, completeCreate);
REG_COMP(Discard, completeDiscard);
REG_COMP(Draw, completeDraw);
REG_COMP(Dump, completeExport);
REG_COMP(Export, completeExport);
REG_COMP(Held, completeHeld);
REG_COMP(SetPlayer, completeSetPlayer);
REG_COMP(Set, completeSet);
REG_COMP(Save, completeNULL);
REG_COMP(Quit, completeNULL);
REG_COMP(Abort, completeNULL);
REG_COMP(Trade, completeTrade);
REG_COMP(Reshuffle, completeNULL);
REG_COMP(Value, completeValue);
REG_COMP(List, completeList);
REG_COMP(Count, completeList);
REG_COMP(Give, completeGive);
REG_COMP(ShuffleIn, completeShuffleIn);
REG_COMP(Help, completeHelp);

char **partialComplete(const std::vector<std::string> &target, const char *text)
{
	const Completer &c = CompleteFactory::get_const_instance();
	if (c.Exists(target[0]))
		return c[target[0]](target, text, target.size());
	return NULL;
}

char **dbCompletion(const char *text, int start, int end)
{
	char **matches = NULL;

	std::vector<std::string> target;
	if (!splitLine(std::string(rl_line_buffer,start), target))
		return NULL;
	
	if (start == 0)
		matches = rl_completion_matches(text, fillFromComplete);
	else
		matches = partialComplete(target, text);
	
	rl_attempted_completion_over = true;
	return matches;
}

int main(int argc, char *argv[])
{
	fs::path cmd(argv[0]);

	rl_variable_bind("expand-tilde","on");
	rl_attempted_completion_function = dbCompletion;
	
	if (argc != 2 && argc != 3)
	{
		std::cerr << argv[0] << " dbName [exportDir]" << std::endl;
		std::cerr << "Version: " << version << std::endl;
		return ErrQuit;
	}

	s_g.reset(new Game(argv[1]));
	//loadHelpText(*ht);

	while(true)
	{
		char *line(NULL);
		line = readline((cmd.leaf() + " > ").c_str());
		if (line && *line)
		{
			add_history(line);
			int error = ParseLine(line,*s_g, std::cout);
			free(line);
			
			if (error == ErrQuit)
				break;
			if (error == ErrSave)
			{
				s_g.reset();
				s_g.reset(new Game(argv[1]));
				continue;
			}
			if (error != ErrNone)
				std::cerr << "Error " << error << std::endl;
			if (error > ErrQuit)
			{
				s_g->Abandon();
				return error;
			}
		}
	}
	if (argc == 3 || !s_g->_vars["export"].empty())
	{
		std::string d = s_g->_vars["export"].empty()?argv[2]:"default";
		int error = ParseLine(std::string("export ")+d,*s_g, std::cout);
		if (error != ErrNone)
			std::cerr << "Export Error " << error << std::endl;
		return error;
	}
	return ErrNone;
}
