#include "db.h"
#include "dbUtils.h"

#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

namespace fs = boost::filesystem;

#include <iostream>

#include <readline/readline.h>
#include <readline/history.h>

#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>

#include <boost/spirit/core.hpp>
#include <boost/spirit/actor/push_back_actor.hpp>

boost::scoped_ptr<Game> g;
std::string g_currentPower;
int g_currentDeck(0);

const char *createHelp[] = {"create","CardList","PowerList",NULL};
const char *discardHelp[] = {"discard","Power","[Card]",(const char *)(1)};
const char *drawHelp[] = {"draw","Power","NumCards","[Pick]",(const char *)(1)};
const char *exportHelp[] = {"export","DestinationDir",NULL};
const char *heldHelp[] = {"held","Power/Deck",NULL};
const char *setPlayerHelp[] = {"setPlayer","Power","Name","Password","EMail",NULL};
const char *quitHelp[] = {"quit",NULL};
const char *abortHelp[] = {"abort",NULL};
const char *insertHelp[] = {"insert", "Deck", "Card", NULL};
const char *removeHelp[] = {"remove", "Deck", "Card", NULL};
const char *tradeHelp[] = {"trade", "Power", "Card", "Card", "Card", "Power", "Card", "Card", "Card", (const char *)(1)};
const char *reshuffleHelp[] = {"reshuffle",NULL};
const char *listHelp[] = {"list","Object",NULL};
const char *countHelp[] = {"count","Object",NULL};
const char *giveHelp[] = {"give", "Power", "Power", "Card", NULL};
const char *shuffleInHelp[] = {"shufflein","deck","type","maxCount","cardName",NULL};

const char **topHelps[] = {createHelp,discardHelp,drawHelp,
	exportHelp,heldHelp,setPlayerHelp,
	quitHelp,abortHelp, insertHelp,
	removeHelp,tradeHelp,reshuffleHelp,
	listHelp,countHelp,giveHelp,shuffleInHelp};

const char *objectList[] = {"powers","players","decks","discards"};
const char *typeList[] = {"tradable","nontradable"};

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

char *countryFill(const char *text, int state)
{
	const int matchLen = std::strlen(text);
	
	for(Powers::const_iterator i = g->_powers.begin(); i != g->_powers.end(); i++)
	{
		char *value = NULL;
		if (value = fillBuffer(i->first->_name, text, state))
			return value;
	}
	return NULL;
}

char *typeFill(const char *text, int state)
{
	const char *numbers[] = {"0","-1","-2","-3"};
	const int len = sizeof(numbers)/sizeof(const char *);
	for(int i =0 ; i < len; i++)
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
		matches++;
		pos = text.find(match, pos+1);
	}

	return matches;
}

char *cardFill(const char *text, int state)
{
	Powers::const_iterator power = g->FindPower(g_currentPower);
	const std::string buffer(rl_line_buffer,rl_point);

	if (power != g->_powers.end())
	{
		for(Hand::const_iterator i = power->first->_hand.begin(); i != power->first->_hand.end(); i++)
		{
			if (char *value = fillBuffer((*i)->_name, text, state))
				return value;
		}
	} else
	{
		for(Cards::const_iterator i = g->_cards.begin(); i != g->_cards.end(); i++)
		{
			if (g_currentDeck && (*i)->_deck != g_currentDeck)
				continue;
			char *value = NULL;
			if (value = fillBuffer((*i)->_name, text, state))
				return value;
		}
	}
	return NULL;
}

char *numberFill(const char *text, int state)
{
	const char *numbers[] = {"1","2","3","4","5","6","7","8","9"};
	const int len = sizeof(numbers)/sizeof(const char *);
	for(int i =0 ; i < len; i++)
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
	for(int i = 0; i < len; i++)
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
	
	visits++;
	
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

	visits++;

	if (value = countryFill(text, state))
		return value;
	else if (!oldVisits)
		oldVisits = visits;

	if (value = cardFill(text, state-oldVisits))
		return value;

	oldVisits = visits = 0;
	return NULL;
}

char **completeCreate(const std::vector<std::string> &, const char *text, int depth, int command)
{
	switch (depth)
	{
		case 1:
		case 2:
		{
			return  rl_completion_matches(text, rl_filename_completion_function);
			break;
		}
	}
	return NULL;
}

char **completeDiscard(const std::vector<std::string> &data, const char *text, int depth, int command)
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

char **completeDraw(const std::vector<std::string> &, const char *text, int depth, int command)
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


char **completeExport(const std::vector<std::string> &, const char *text, int depth, int command)
{
	switch(depth)
	{
		case 1:
			return rl_completion_matches(text, rl_filename_completion_function);
	}
	return NULL;
}

char **completeHeld(const std::vector<std::string> &, const char *text, int depth, int command)
{
	switch(depth)
	{
		case 1:
			return rl_completion_matches(text,augmentedHandFill);
	}
	return NULL;
}

char **completeSetPlayer(const std::vector<std::string> &, const char *text, int depth, int command)
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

char **completeInsert(const std::vector<std::string> &data, const char *text, int depth, int command)
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

char **completeTrade(const std::vector<std::string> &data, const char *text, int depth, int command)
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
			Powers::const_iterator p2 = g->_powers.end();
			for(int i = 5; i < depth; i++)
			{
				p2 = g->FindPower(data[i]);
				if (p2 != g->_powers.end())
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

char **completeShuffleIn(const std::vector<std::string> &data, const char *text, int depth, int command)
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

char **completeList(const std::vector<std::string> &data, const char *text, int depth, int command)
{
	switch(depth)
	{
		case 1:
		return rl_completion_matches(text, listFill);
		break;
	}
	return NULL;
}

char **completeGive(const std::vector<std::string> &data, const char *text, int depth, int command)
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

char **completeNULL(const std::vector<std::string> &, const char *, int, int)
{
	return NULL;
}

bool isValidCount(int command, int size)
{
	int i =0;
	for(; topHelps[command][i] > (const char *)(1); i++);
	
	if (topHelps[command][i] == NULL)
		return size == i;
	return size > i-2;
}

int parseHelp(const std::vector<std::string> &names, int command)
{
	int i = 0;
	for(; topHelps[command][i] > (const char *)(1); i++)
	{
		std::cerr << topHelps[command][i] << ' ';
	}		
	if (topHelps[command][i] != NULL)
		std::cerr << "...";
	std::cerr << std::endl;
	return ErrNone;
}

int parseCreate(const std::vector<std::string> &names, int command)
{
	if (!isValidCount(command, names.size()))
		return parseHelp(names, command);
	
	if (!CreateGame(names[1],names[2],*g))
		return ErrGameCreation;

	return ErrNone;
}

int parseGive(const std::vector<std::string> &names, int command)
{
	if (!isValidCount(command, names.size()))
		return parseHelp(names, command);
	
	Powers::iterator from = g->FindPower(names[1]);
	Powers::iterator to = g->FindPower(names[2]);
	
	if (from == g->_powers.end() || to == g->_powers.end())
		return ErrPowerNotFound;
	
	const std::vector<std::string> cards(names.begin()+3, names.end());

	Hand left;
	for(int i = 3; i < names.size(); i++)
	{
		CardP card = g->FindCard(names[i]);
		if (!card)
			return ErrCardNotFound;
		left.insert(card);
	}
	
	if (!from->first->Has(left))
	{
		return ErrCardNotFound;
	}
	
	if (!Stage(from->first->_hand, from->first->_staging, left))
	{
		return ErrCardDeletion;
	}
	
	std::swap(from->first->_staging, to->first->_staging);
	
	from->first->Merge(); to->first->Merge();

	std::cout << from->first->_name << " Gives: \n";
	for(Hand::const_iterator i = left.begin(); i != left.end(); i++)
	{
		std::cout << (*i)->_name << ',';
	}
	std:: cout << std::endl << std::endl;
	
	return ErrNone;
}

int parseQuit(const std::vector<std::string> &names, int command)
{
	return ErrQuit;
}

int parseAbort(const std::vector<std::string> &names, int command)
{
	return ErrAbort;
}

int parseDiscard(const std::vector<std::string> &names, int command)
{
	if (!isValidCount(command, names.size()))
		return parseHelp(names, command);

	const std::vector<std::string> cards(names.begin()+2, names.end());
	
	Powers::iterator power = g->FindPower(names[1]);

	if (power == g->_powers.end())
		return ErrPowerNotFound;
	
	Hand toss;
	if (!FillHand(*g, cards, toss))
		return ErrCardNotFound;
	
	FillCalamities(*g, *power->first, toss);
	
	if (!RemoveHand(power->first->_hand, toss))
		return ErrCardDeletion;

	MergeDiscards(*g, toss);
	RenderHand(std::cout, power->first->_hand);
	
	return ErrNone;
}

int parseDraw(const std::vector<std::string> &names, int command)
{
	if (!isValidCount(command, names.size()))
		return parseHelp(names, command);

	int numCards = boost::lexical_cast<int>(names[2]);
	std::vector<int> picks(names.size()-3);
	for(int i = 0; i < picks.size(); i++)
	{
		picks[i] = boost::lexical_cast<int>(names[3+i]);
	}

	Powers::iterator power;
	if ((power = g->FindPower(names[1])) == g->_powers.end())
	{
		return ErrPowerNotFound;
	}
	
	std::cout << "Held:" << std::endl;
	RenderHand(std::cout, power->first->_hand);
	
	Hand tempHand;
	std::cout << std::endl << "Drawn:" << std::endl;
	DrawCards(*g, tempHand, numCards);
	RenderHand(std::cout, tempHand);
	
	Hand temp2;
	std::cout << std::endl << "Bought:" << std::endl;
	for(int i = 0; i < picks.size(); i++)
	{
		PickCard(*g, temp2, picks[i]);
	}
	RenderHand(std::cout, temp2);
	
	MergeHands(tempHand, temp2);
	MergeHands(power->first->_hand, tempHand);

	return ErrNone;	
}

int parseExport(const std::vector<std::string> &names, int command)
{
	if (!isValidCount(command, names.size()))
		return parseHelp(names, command);

	fs::path base(names[1]);
	fs::ofstream auth(base / "auth", std::ios::binary);
	fs::ofstream contact(base / "contact", std::ios::binary);
	for(Powers::const_iterator i = g->_powers.begin(); i != g->_powers.end(); i++)
	{
		if (!i->second)
			continue;
		fs::ofstream out(base / i->first->_name, std::ios::binary);
		
		auth << i->first->_name << ' ' << i->second->_password << std::endl;
		contact << i->first->_name << '\t' << i->second->_name << '\t' << i->second->_email << std::endl;
		for(Hand::const_iterator j = i->first->_hand.begin(); j != i->first->_hand.end(); j++)
		{
			out << (*j)->_image << std::endl;
		}
	}

	return ErrNone;
}

int parseHeld(const std::vector<std::string> &names, int command)
{
	if (!isValidCount(command, names.size()))
		return parseHelp(names, command);

	try
	{
		int deck = boost::lexical_cast<int>(names[1]);
		if (deck > 0 && deck < g->_decks.size())
		{
			RenderDeck(std::cout, g->_decks[deck]);
			return ErrNone;
		}
	} 
	catch(...)
	{
	}
	if (boost::icontains(names[1],"discard"))
	{
		for(int i = 1; i < g->_discards.size(); i++)
		{
			std::cout << i << ": ";
			RenderHand(std::cout, g->_discards[i]);
		}
		return ErrNone;
	}
	
	Powers::iterator power;
	if ((power = g->FindPower(names[1])) == g->_powers.end())
	{
		return ErrPowerNotFound;
	}
	
	RenderHand(std::cout, power->first->_hand);

	return ErrNone;
}

int parseSetPlayer(const std::vector<std::string> &names, int command)
{
	if (!isValidCount(command, names.size()))
		return parseHelp(names, command);


	Powers::iterator power;
	if ((power = g->FindPower(names[1])) == g->_powers.end())
	{
		return ErrPowerNotFound;
	}

	PlayerP player(new Player);
	player->_name = names[2];
	player->_password = names[3];
	player->_email = names[4];
	
	power->second = player;

	return ErrNone;
}

int parseTrade(const std::vector<std::string> &names, int command)
{
	if (!isValidCount(command, names.size()))
		return parseHelp(names, command);

	Powers::iterator power, power2;
	if ((power = g->FindPower(names[1])) == g->_powers.end())
	{
		return ErrPowerNotFound;
	}

	int breakPoint = 0;
	for(int i = 5; i < names.size()-3; i++)
	{
		if ((power2 = g->FindPower(names[i])) != g->_powers.end())
		{
			breakPoint = i;
			break;
		}
	}

	if (!breakPoint)
		return ErrPowerNotFound;

	Hand left, right;
	for(int i = 2; i < breakPoint; i++)
	{
		CardP card = g->FindCard(names[i]);
		if (!card)
			return ErrCardNotFound;
		left.insert(card);
	}

	for(int i = breakPoint+1; i < names.size(); i++)
	{
		CardP card = g->FindCard(names[i]);
		if (!card)
			return ErrCardNotFound;
		right.insert(card);
	}
	
	if (!power->first->Has(left) || !power2->first->Has(right))
	{
		return ErrCardNotFound;
	}
	
	if (!Stage(power->first->_hand, power->first->_staging, left) ||
		!Stage(power2->first->_hand, power2->first->_staging, right))
	{
		return ErrCardDeletion;
	}
	
	std::swap(power->first->_staging, power2->first->_staging);
	
	power->first->Merge(); power2->first->Merge();

	std::cout << power->first->_name << " Gives: \n";
	for(Hand::const_iterator i = left.begin(); i != left.end(); i++)
	{
		std::cout << (*i)->_name << ',';
	}
	std:: cout << std::endl << std::endl;
	
	std::cout << power2->first->_name << " Gives: \n";
	for(Hand::const_iterator i = right.begin(); i != right.end(); i++)
	{
		std::cout << (*i)->_name << ',';
	}
	std::cout << std::endl;
	
	return ErrNone;
}

int parseShuffleIn(const std::vector<std::string> &names, int command)
{
	if (!isValidCount(command, names.size()))
		return parseHelp(names, command);
	
	CardP card(new Card);
	card->_deck = boost::lexical_cast<int>(names[1]);
	card->_type = static_cast<Card::Type>(boost::lexical_cast<int>(names[2]));
	card->_maxCount = boost::lexical_cast<int>(names[3]);
	card->_name = names[4];

	switch (card->_type)
	{
		case Card::Normal:
			card->_image = card->_name + ".png";
			break;
		case Card::NonTradable:
			card->_image = boost::lexical_cast<std::string>(card->_deck) + "NT.png";
			break;
		case Card::Tradable:
			card->_image = boost::lexical_cast<std::string>(card->_deck) + "T.png";
			break;
		case Card::Minor:
			card->_image = boost::lexical_cast<std::string>(card->_deck) + "M.png";
			break;
	}

	g->_cards.insert(card);

	Deck &deck = g->_decks[card->_deck];
	for(int i = 0; i < card->_maxCount; i++)
	{
		int maxCount = g->_decks[card->_deck].size();
		int location = maxCount?CivRand(maxCount):0;
		std::cerr << "Inserting (" << card->_deck << ")(" << card->_type << ")(" << card->_maxCount << ")(" << card->_name << ") at "<< location << std::endl;
		deck.insert(deck.begin()+location, card);
	}
	
	return ErrNone;
}

int parseReshuffle(const std::vector<std::string> &names, int command)
{
	if (!isValidCount(command, names.size()))
		return parseHelp(names, command);

	for(int i = 0; i < g->_discards.size(); i++)
	{
		ShuffleIn(g->_decks[i], g->_discards[i]);
		g->_discards[i].clear();
	}

	return ErrNone;
}

int parseList(const std::vector<std::string> &names, int command)
{
	if (!isValidCount(command, names.size()))
		return parseHelp(names, command);

	const int numObjects = sizeof(objectList)/sizeof(const char *);
	if (boost::iequals(names[1],"powers"))
	{
		for(Powers::const_iterator i = g->_powers.begin(); i != g->_powers.end(); i++)
		{
			std::cout << i->first->_name << '\t';
			for(Hand::const_iterator j = i->first->_hand.begin(); j != i->first->_hand.end(); j++)
			{
				std::cout << (*j)->_name << ',';
			}
			std::cout << std::endl;
		}
		return ErrNone;	
	}
	if (boost::iequals(names[1],"players"))
	{
		for(Powers::const_iterator i = g->_powers.begin(); i != g->_powers.end(); i++)
		{
			std::cout << i->first->_name << '\t';
			if (i->second)
				std::cout << i->second->_name  << "\t'" << i->second->_password << "'\t'" << i->second->_email << "'"; 
			std::cout << std::endl;
		}
		return ErrNone;
	}
	if (boost::iequals(names[1],"decks"))
	{
		for(int i = 1; i < g->_decks.size(); i++)
		{
			std::cout << i << '\t';
			for(Deck::const_iterator j = g->_decks[i].begin(); j != g->_decks[i].end(); j++)
			{
				std::cout << (*j)->_name << ',';
			}
			std::cout << std::endl;
		}
		return ErrNone;
	}
	if (boost::icontains(names[1],"discard"))
	{
		for(int i = 1; i < g->_discards.size(); i++)
	        {
		        std::cout << i << ": ";
	                RenderHand(std::cout, g->_discards[i]);
	        }
	        return ErrNone;
        }

	return ErrUnableToParse;
}

int parseCount(const std::vector<std::string> &names, int command)
{
	if (!isValidCount(command, names.size()))
		return parseHelp(names, command);

	if (boost::iequals(names[1],"powers"))
	{
		for(Powers::const_iterator i = g->_powers.begin(); i != g->_powers.end(); i++)
		{
			std::cout << i->first->_name << '\t' << i->first->_hand.size() << std::endl;
		}
		return ErrNone;
	}
	if (boost::iequals(names[1],"players"))
	{
		int count = 0;
		for(Powers::const_iterator i = g->_powers.begin(); i != g->_powers.end(); i++)
		{
			if (i->second)
				count++;
		}
		std::cout << "Assigned Player: " << count << std::endl;
		return ErrNone;
	}
	if (boost::iequals(names[1],"decks"))
	{
		for(int i = 1; i < g->_decks.size(); i++)
		{
			std::cout << i << '\t' << g->_decks[i].size() << std::endl;
		}
		return ErrNone;
	}
	if (boost::iequals(names[1],"discards"))
	{
		for(int i = 1; i < g->_discards.size(); i++)
		{
			std::cout << i << '\t' << g->_discards[i].size() << std::endl;
		}
	}
	return ErrUnableToParse;
}

int parseNULL(const std::vector<std::string> &names, int command)
{
	if (!isValidCount(command, names.size()))
		return parseHelp(names, command);
	return ErrNone;
}

typedef char **CompleteFunc(const std::vector<std::string> &text, const char *, int depth, int command);
typedef int ParseFunc(const std::vector<std::string> &text, int command);

struct COMMAND {
	const std::string name;
	CompleteFunc *cf;
	ParseFunc *pf;
};

const COMMAND topCommand[] = {{createHelp[0],completeCreate,parseCreate},
							{discardHelp[0],completeDiscard,parseDiscard},
							{drawHelp[0],completeDraw,parseDraw},
							{exportHelp[0],completeExport,parseExport},
							{heldHelp[0],completeHeld,parseHeld},
							{setPlayerHelp[0],completeSetPlayer,parseSetPlayer},
							{quitHelp[0],completeNULL,parseQuit},
							{abortHelp[0],completeNULL,parseAbort},
							{insertHelp[0],completeInsert,parseNULL},
							{removeHelp[0],completeInsert,parseNULL},
							{tradeHelp[0],completeTrade,parseTrade},
							{reshuffleHelp[0],completeNULL,parseReshuffle},
							{listHelp[0],completeList, parseList},
							{countHelp[0],completeList,parseCount},
							{giveHelp[0],completeGive,parseGive},
							{shuffleInHelp[0],completeShuffleIn,parseShuffleIn},
							};
							
bool splitLine(const std::string &line, std::vector<std::string> &target)
{
	using namespace boost::spirit;
	const std::string &trimmed = boost::trim_copy(line);
	target.clear();

	rule<> word = (+(alnum_p | '(' | ')' | "\\ " | '.' | '/' ))[push_back_a(target)];
	rule<> sentence = *(*space_p >> word);
	
	if (!parse(trimmed.c_str(), sentence).full)
		return false;
	
	for(int i = 0; i < target.size(); i++)
	{
		boost::replace_all(target[i], "\\ ", " ");
	}
	
	return true;
}

char *fillFromArray(const std::string &text, int state, const COMMAND *array, const int numValues)
{
	int skip = state;
	
	for(int i = 0; i < numValues; i++)
	{
		char *value = NULL;
		if (value = fillBuffer(array[i].name, text, skip))
			return value;
	}

	return NULL;
}

char *topFill(const char *text, int state)
{
	const int numValues = sizeof(topCommand)/sizeof(COMMAND);
	return fillFromArray(text, state, topCommand, numValues);
}

char **partialComplete(const std::vector<std::string> &target, const char *text)
{
	const int numValues = sizeof(topCommand)/sizeof(COMMAND);
	
	for(int i = 0; i < numValues; i++)
	{
		if (boost::iequals(topCommand[i].name,target[0]))
			return topCommand[i].cf(target, text, target.size(), i);
	}
	return NULL;
}

char **dbCompletion(const char *text, int start, int end)
{
	char **matches = NULL;

	std::vector<std::string> target;
	if (!splitLine(std::string(rl_line_buffer,start), target))
		return NULL;
	
	if (start == 0)
		matches = rl_completion_matches(text, topFill);
	else
		matches = partialComplete(target, text);
	
	rl_attempted_completion_over = true;
	return matches;
}

int ParseLine(const std::string &line)
{
	std::vector<std::string> target;
	if (!splitLine(line, target))
		return ErrUnableToParse;

	const int numValues = sizeof(topCommand)/sizeof(COMMAND);
	for(int i = 0; i < numValues; i++)
	{
		if (boost::iequals(topCommand[i].name,target[0]))
			return topCommand[i].pf(target, i);
	}
	
	return ErrNone;
}

int main(int argc, char *argv[])
{
	fs::path cmd(argv[0]);

	rl_variable_bind("expand-tilde","on");
	rl_attempted_completion_function = dbCompletion;
	
	if (argc != 2 && argc != 3)
	{
		std::cerr << argv[0] << " dbName [exportDir]" << std::endl;
		return ErrQuit;
	}

	g.reset(new Game(argv[1]));

	while(true)
	{
		char *line(NULL);
		line = readline((cmd.leaf() + " > ").c_str());
		if (line && *line)
		{
			add_history(line);
			int error = ParseLine(line);
			free(line);
			
			if (error == ErrQuit)
				break;
			if (error != ErrNone)
				std::cerr << "Error " << error << std::endl;
			if (error > ErrQuit)
				throw error;
		}
	}
	if (argc == 3)
	{
		int error = ParseLine(std::string("export ")+argv[2]);
		if (error != ErrNone)
			std::cerr << "Export Error " << error << std::endl;
		return error;
	}
	return ErrNone;
}
