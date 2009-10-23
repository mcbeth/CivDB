#include "parser.h"
#include "dbUtils.h"
#include "factory.h"

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_push_back_actor.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem/fstream.hpp>
namespace fs = boost::filesystem;

#include <boost/function.hpp>
#include <boost/serialization/singleton.hpp>
#include <iostream>
#include <vector>

typedef boost::function<int (const std::vector<std::string> &, Game &, std::ostream &)> ParseFunc;

typedef FactoryOwner<ParseFunc> Parser;
typedef boost::serialization::singleton<Parser> ParseFactory;

#define REG_PARSE(_trigger, _str) \
	static bool _trigger ## _parse_registered = \
		ParseFactory::get_mutable_instance().Register(#_trigger, parse##_trigger); \
	static bool _trigger ## _help_registered = \
		HelpFactory::get_mutable_instance().Register(#_trigger, \
				boost::bind(GenericHelp,#_trigger " " _str,_1))


typedef boost::function<void (std::ostream &)> HelpFunc;
typedef FactoryOwner<HelpFunc> Helper;
typedef boost::serialization::singleton<Helper> HelpFactory;

void GenericHelp(const std::string &d, std::ostream &out)
{
	out << d << std::endl;
}

int parseHelpC(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	const Helper &h = HelpFactory::get_const_instance();

	if (!h.Exists(names[0]))
		return ErrUnableToParse;

	h[names[0]](out);
	return ErrNone;
}

int parseCreate(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 4)
		return parseHelpC(names, g, out);
	
	if (!CreateGame(names[1],names[2],names[3],g))
		return ErrGameCreation;

	return ErrNone;
}
REG_PARSE(Create,"CardList PowerList RuleSet");

int parseGive(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 4)
		return parseHelpC(names, g, out);
	
	Powers::iterator from = g.FindPower(names[1]);
	Powers::iterator to = g.FindPower(names[2]);
	
	if (from == g._powers.end() || to == g._powers.end())
		return ErrPowerNotFound;
	
	Hand left;
	CardP card = g.FindCard(names[3]);
	if (!card)
	{
		if (boost::algorithm::iequals(names[3],"Random"))
		{
			Deck d(from->first->_hand.begin(), from->first->_hand.end());
			std::random_shuffle(d.begin(), d.end(), CivRand);
			card = d[0];
		}
		else
		{
			return ErrCardNotFound;
		}
	}
	left.insert(card);
	
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

	out << from->first->_name << " Gives: \n";
	for(Hand::const_iterator i = left.begin(); i != left.end(); i++)
	{
		out << (*i)->_name << ',';
	}
	std:: cout << std::endl << std::endl;
	
	return ErrNone;
}
REG_PARSE(Give, "FromPower ToPower Card");

int parseSave(const std::vector<std::string> &names, Game &, std::ostream &)
{
	return ErrSave;
}
REG_PARSE(Save, "");

int parseSet(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() > 3)
		return parseHelpC(names, g, out);

	if (names.size() == 1)
	{
		for(auto i = g._vars.begin(); i != g._vars.end(); ++i)
		{
			out << i->first << ": '" << i->second << "'" << std::endl;	
		}
		return ErrNone;
	}

	auto i = g._vars.find(names[1]);
	if (i == g._vars.end())
	{
		out << "Undefined Variable (" << names[1] << ")" << std::endl;
		return ErrUnableToParse;
	}

	if (names.size() == 2)
	{
		out << i->first << ": '" << i->second << "'" << std::endl;
		return ErrNone;
	}

	if (names.size() == 3)
	{
		i->second = names[2];
		return ErrNone;
	}

	return ErrUnableToParse;
}
REG_PARSE(Set, "Variable Value");

int parseQuit(const std::vector<std::string> &names, Game &, std::ostream &)
{
	return ErrQuit;
}
REG_PARSE(Quit, "");

int parseAbort(const std::vector<std::string> &names, Game &, std::ostream &)
{
	return ErrAbort;
}
REG_PARSE(Abort, "");

int parseDiscard(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() < 2)
		return parseHelpC(names, g, out);

	const std::vector<std::string> cards(names.begin()+2, names.end());
	
	Powers::iterator power = g.FindPower(names[1]);

	if (power == g._powers.end())
		return ErrPowerNotFound;
	
	Hand toss;
	if (!FillHand(g, cards, toss))
		return ErrCardNotFound;
	
	FillCalamities(g, *power->first, toss);
	RenderHand(out, toss);
	out << std::endl;
	
	if (!RemoveHand(power->first->_hand, toss))
		return ErrCardDeletion;

	MergeDiscards(g, toss);
	RenderHand(out, power->first->_hand);
	
	return ErrNone;
}
REG_PARSE(Discard, "Power [Card] ...");

int parseDraw(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() < 3)
		return parseHelpC(names, g, out);

	int numCards = boost::lexical_cast<int>(names[2]);
	std::vector<int> picks(names.size()-3);
	for(int i = 0; i < picks.size(); i++)
	{
		picks[i] = boost::lexical_cast<int>(names[3+i]);
	}

	Powers::iterator power;
	if ((power = g.FindPower(names[1])) == g._powers.end())
	{
		return ErrPowerNotFound;
	}
	
	out << "Held:" << std::endl;
	RenderHand(out, power->first->_hand);
	
	Hand tempHand;
	out << std::endl << "Drawn:" << std::endl;
	DrawCards(g, tempHand, numCards);
	RenderHand(out, tempHand);
	
	Hand temp2;
	out << std::endl << "Bought:" << std::endl;
	for(int i = 0; i < picks.size(); i++)
	{
		PickCard(g, temp2, picks[i]);
	}
	RenderHand(out, temp2);
	
	MergeHands(tempHand, temp2);
	MergeHands(power->first->_hand, tempHand);

	return ErrNone;	
}
REG_PARSE(Draw,"Power #Cards [Pick] ...");

int parseDump(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 2)
		return parseHelpC(names, g, out);

	fs::path base(names[1]);
	fs::ofstream cardList(base/"cardList", std::ios::binary);
	fs::ofstream powerList(base/"powerList", std::ios::binary);
	for(int i = 0; i < g._decks.size(); i++)
	{
		for(Cards::const_iterator j = g._cards.begin(); j != g._cards.end(); j++)
		{
			if (i == (*j)->_deck && (*j)->_type == Card::Normal)
				cardList << i << '\t' << (*j)->_maxCount << '\t' << (*j)->_name << std::endl;
		}	

		for(Cards::const_iterator j = g._cards.begin(); j != g._cards.end(); j++)
		{
			if (i == (*j)->_deck && (*j)->_type != Card::Normal)
			{
				cardList << (*j)->_type << '\t' << 1 << '\t' << (*j)->_name << std::endl;
			}
		}
	}

	for(Powers::const_iterator i = g._powers.begin(); i != g._powers.end(); i++)
	{
		powerList << i->first->_name << std::endl;
	}

	return ErrNone;
}
REG_PARSE(Dump,"DestinationDir");

int parseExport(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 2)
		return parseHelpC(names, g, out);

	fs::path base;
	if (boost::iequals(names[1], "default"))
		base = g._vars["export"];
	else
		base = names[1];

	fs::ofstream auth(base / "auth", std::ios::binary);
	fs::ofstream contact(base / "contact", std::ios::binary);
	for(Powers::const_iterator i = g._powers.begin(); i != g._powers.end(); i++)
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

	out << "Exported to: " << base << std::endl;
	g._vars["export"] = base.string();
	return ErrNone;
}
REG_PARSE(Export, "DestinationDir");

int parseHeld(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 2)
		return parseHelpC(names, g, out);

	try
	{
		int deck = boost::lexical_cast<int>(names[1]);
		if (deck > 0 && deck < g._decks.size())
		{
			RenderDeck(out, g._decks[deck]);
			return ErrNone;
		}
	} 
	catch(...)
	{
	}
	if (boost::icontains(names[1],"discard"))
	{
		for(int i = 1; i < g._discards.size(); i++)
		{
			out << i << ": ";
			RenderHand(out, g._discards[i]);
		}
		return ErrNone;
	}
	
	Powers::iterator power;
	if ((power = g.FindPower(names[1])) == g._powers.end())
	{
		return ErrPowerNotFound;
	}
	
	RenderHand(out, power->first->_hand);

	return ErrNone;
}
REG_PARSE(Held,"Power/Deck/discard");

int parseSetPlayer(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 5)
		return parseHelpC(names, g, out);


	Powers::iterator power;
	if ((power = g.FindPower(names[1])) == g._powers.end())
	{
		out << "Can't find '"<< names[1] << "'" << std::endl;
		return ErrPowerNotFound;
	}

	PlayerP player(new Player);
	player->_name = names[2];
	player->_password = names[3];
	player->_email = names[4];
	
	power->second = player;

	return ErrNone;
}
REG_PARSE(SetPlayer,"Power Name Password EMail");

int parseTrade(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() < 9)
		return parseHelpC(names, g, out);

	Powers::iterator power, power2;
	if ((power = g.FindPower(names[1])) == g._powers.end())
	{
		return ErrPowerNotFound;
	}

	int breakPoint = 0;
	for(int i = 5; i < names.size()-3; i++)
	{
		if ((power2 = g.FindPower(names[i])) != g._powers.end())
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
		CardP card = g.FindCard(names[i]);
		if (!card)
			return ErrCardNotFound;
		left.insert(card);
	}

	for(int i = breakPoint+1; i < names.size(); i++)
	{
		CardP card = g.FindCard(names[i]);
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

	out << power->first->_name << " Gives: \n";
	for(Hand::const_iterator i = left.begin(); i != left.end(); i++)
	{
		out << (*i)->_name << ',';
	}
	out << std::endl << std::endl;
	
	out << power2->first->_name << " Gives: \n";
	for(Hand::const_iterator i = right.begin(); i != right.end(); i++)
	{
		out << (*i)->_name << ',';
	}
	out << std::endl;
	
	return ErrNone;
}
REG_PARSE(Trade,"Power Card Card Card ... Power Card Card Card ...");
/*
bool loadHelpText(HelpText &ht)
{
boost::assign::insert(ht)
("abort","Quits the program without saving any work")
("discard","(+) Throws the cards selected and all calamities the power has in the appropriate discard piles")
("export","Writes hand information and authorization information appropriate for hand.php to the selected directory")
("help","Gives you access to longer helps strings for each command")
("quit","Saves the current state and quites")
("save","Saves the current state and doesn't quit")
("trade","(+) Trades the cards between the two players.  You are responsible to verify that their expectations meet, the software will verify that the trade is completeable and legal (at least three cards and the players do have them and the calamities are not tradeable)")
("count","Counts the number of cards in the selected category (except players, which counts the players)")
("draw","(+) The first number is the number of decks to draw from (does not support the Imperial variant (yet).  Each additional optional number is a single card drawn from the stack.  So, for example, if your rules allow drawing and buying at teh same time. 'draw Assyria 4 9' would give Assyria cards from 1-4 and a 9. 'draw Assyria 0 3 3' would just give Assyria two 3 cards")
("give","(+) Give hands a card from the first power to the second power (you are responsible for your own random selection for now)")
("setPlayer","(+) Assigns a player,password, and e-mail addres to a power")
("create","(+) Accepts a list of cards, powers, and an ruleset to create a new game (see example)")
("dump","Creates a list of cards and powers that would allow creating a game just like the current one (but in the base state) to the selected directory")
("held","Provides a single list of the contents of a power's hand or a single deck")
("list","Provides a list of all of the features of the selected category (same categories as count), the calamity list should be in resolution order")
("reshuffle","(+) Perform the appropriate shuffling of the discard stacks into the trade stacks (should only happen once a turn)")
("shufflein","(+) a disaster recovery command, allows you to add a whole new type of card to the deck");
}

int parseHelp(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 2)
		return parseHelpC(names, g, out);

	HelpText::const_iterator i = ht->find(names[1]);
	if (i != ht->end())
		out << i->second << std::endl;
	return ErrNone;
}
REG_PARSE(Help,"Command");
*/

int parseShuffleIn(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 5)
		return parseHelpC(names, g, out);
	
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

	g._cards.insert(card);

	Deck &deck = g._decks[card->_deck];
	for(int i = 0; i < card->_maxCount; i++)
	{
		int maxCount = g._decks[card->_deck].size();
		int location = maxCount?CivRand(maxCount):0;
		out << "Inserting (" << card->_deck << ")(" << card->_type << ")(" << card->_maxCount << ")(" << card->_name << ") at "<< location << std::endl;
		deck.insert(deck.begin()+location, card);
	}
	
	return ErrNone;
}
REG_PARSE(ShuffleIn,"deck type maxCount cardName");

int parseReshuffle(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 1)
		return parseHelpC(names, g, out);

	for(int i = 0; i < g._discards.size(); i++)
	{
		ShuffleIn(g._decks[i], g._discards[i]);
		g._discards[i].clear();
	}

	return ErrNone;
}
REG_PARSE(Reshuffle,"");

int parseList(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 2)
		return parseHelpC(names, g, out);

	if (boost::iequals(names[1],"powers"))
	{
		for(Powers::const_iterator i = g._powers.begin(); i != g._powers.end(); i++)
		{
			out << i->first->_name << '\t';
			for(Hand::const_iterator j = i->first->_hand.begin(); j != i->first->_hand.end(); j++)
			{
				out << (*j)->_name << ',';
			}
			out << std::endl;
		}
		return ErrNone;	
	}
	if (boost::iequals(names[1],"players"))
	{
		for(Powers::const_iterator i = g._powers.begin(); i != g._powers.end(); i++)
		{
			out << i->first->_name << '\t';
			if (i->second)
				out << i->second->_name  << "\t'" << i->second->_password << "'\t'" << i->second->_email << "'"; 
			out << std::endl;
		}
		return ErrNone;
	}
	if (boost::iequals(names[1],"decks"))
	{
		for(int i = 1; i < g._decks.size(); i++)
		{
			out << i << '\t';
			for(Deck::const_iterator j = g._decks[i].begin(); j != g._decks[i].end(); j++)
			{
				out << (*j)->_name << ',';
			}
			out << std::endl;
		}
		return ErrNone;
	}
	if (boost::icontains(names[1],"discard"))
	{
		for(int i = 1; i < g._discards.size(); i++)
	        {
		        out << i << ": ";
	                RenderHand(out, g._discards[i]);
	        }
	        return ErrNone;
        }
	if (boost::icontains(names[1],"calamities"))
	{
		typedef std::multimap<CardP, PowerP> RevMap;
		RevMap calamities;
		for(Powers::const_iterator i = g._powers.begin(); i != g._powers.end(); i++)
		{
			for(Hand::const_iterator j = i->first->_hand.begin(); j != i->first->_hand.end(); j++)
			{
				if ((*j)->_type != Card::Normal)
					calamities.insert(std::make_pair(*j,i->first));
			}
		}

		for(RevMap::const_iterator i = calamities.begin(); i != calamities.end(); i++)
		{
			out << i->first->_name << ": " << i->second->_name << std::endl;
		}
		return ErrNone;
	}
	return ErrUnableToParse;
}
REG_PARSE(List,"powers/players/decks/discard/calamities");

int parseCount(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 2)
		return parseHelpC(names, g, out);

	if (boost::iequals(names[1],"calamities"))
	{
		int bigCount(0); 
		for(Powers::const_iterator i = g._powers.begin(); i != g._powers.end(); i++)
		{
			int count(0);
			for(Cards::const_iterator j = i->first->_hand.begin(); j != i->first->_hand.end(); j++)
			{
				if ((*j)->_type != Card::Normal)
					count++;
			}
			out << i->first->_name << '\t' << count << std::endl;
			bigCount+=count;
		}
		out << "Total:\t" << bigCount << std::endl;
		return ErrNone;
	}
	if (boost::iequals(names[1],"powers"))
	{
		int bigCount(0);
		for(Powers::const_iterator i = g._powers.begin(); i != g._powers.end(); i++)
		{
			out << i->first->_name << '\t' << i->first->_hand.size() << std::endl;
			bigCount += i->first->_hand.size();
		}
		out << "Total:\t" << bigCount << std::endl;
		return ErrNone;
	}
	if (boost::iequals(names[1],"players"))
	{
		int count = 0;
		for(Powers::const_iterator i = g._powers.begin(); i != g._powers.end(); i++)
		{
			if (i->second)
				count++;
		}
		out << "Assigned Player: " << count << std::endl;
		return ErrNone;
	}
	if (boost::iequals(names[1],"decks"))
	{
		int bigCount(0);
		for(int i = 1; i < g._decks.size(); i++)
		{
			out << i << '\t' << g._decks[i].size() << std::endl;
			bigCount += g._decks[i].size();
		}
		out << "Total:\t" << bigCount << std::endl;
		return ErrNone;
	}
	if (boost::iequals(names[1],"discards"))
	{
		int bigCount(0);
		for(int i = 1; i < g._discards.size(); i++)
		{
			out << i << '\t' << g._discards[i].size() << std::endl;
			bigCount += g._discards[i].size();
		}
		out << "Total:\t" << bigCount << std::endl;
		return ErrNone;
	}
	return ErrUnableToParse;
}
REG_PARSE(Count,"powers/players/decks/discard/calamities");

bool splitLine(const std::string &line, std::vector<std::string> &target)
{
	using namespace boost::spirit::classic;
	const std::string &trimmed = boost::trim_copy(line);
	target.clear();

	rule<> word = (+(alnum_p | '~' | ':' | '_' | '(' | ')' | "\\ " | '.' | '/' | '@'))[push_back_a(target)];
	rule<> sentence = *(*space_p >> word);
	
	if (!parse(trimmed.c_str(), sentence).full)
		return false;
	
	for(int i = 0; i < target.size(); i++)
	{
		boost::replace_all(target[i], "\\ ", " ");
	}
	
	return true;
}

int ParseLine(const std::string &line, Game &g, std::ostream &out)
{
	const Parser &p = ParseFactory::get_const_instance();
	std::vector<std::string> target;
	if (!splitLine(line, target))
		return ErrUnableToParse;

	if (p.Exists(target[0]))
		return p[target[0]](target, g, out);
	else
		return ErrUnableToParse;
}
