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
#include <boost/foreach.hpp>
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

int parseImport(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 3)
		return parseHelpC(names, g, out);

	if (boost::iequals(names[1],"Civ"))
	{
		if (ParseCivCards(names[2], g))
			return ErrNone;
	}

	return ErrUnableToParse;
}
REG_PARSE(Import, "Civ file");

int parseBuy(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() < 3)
		return parseHelpC(names, g, out);

	Powers::iterator power;
	if ((power = g.FindPower(names[1])) == g._powers.end())
	{
		return ErrPowerNotFound;
	}

	int breakPoint = 0;
	for(int i = 3; i < names.size(); ++i)
	{
		if (g.FindCivCard(names[i]))
		{
			breakPoint = i;
			break;
		}
	}

	if (!breakPoint)
		return ErrCardNotFound;

	Hand left;
	CivCards right;
	bool free = false;
	int tokens = 0;
	for(int i = 2; i < breakPoint; ++i)
	{
		CardP card = g.FindCard(names[i]);
		if (!card)
		{
			if (boost::iequals(names[i],"free"))
				free = true;
			else if (boost::all(names[i],boost::is_digit()))
				tokens += boost::lexical_cast<int>(names[i]);
			else
				return ErrCardNotFound;

			continue;
		}
		left.insert(card);
	}

	for(int i = breakPoint; i < names.size(); ++i)
	{
		CivCardP card = g.FindCivCard(names[i]);
		if (!card)
			return ErrCardNotFound;
		right.insert(card);
	}
	
	if (!power->first->Has(left))
	{
		return ErrCardNotFound;
	}
	if (power->first->Has(right))
	{
		return ErrCivCardDuplicate;
	}

	int cost = 0;
	BOOST_FOREACH(auto i, right)
	{
		cost += power->first->_civCards.Cost(i);	
	}
	
	int value = ValueHand(left);
	if (tokens+value < cost && !free)
	{
		out << "Missing " << cost-tokens-value << std::endl;
		return ErrInsufficientFunds;	
	}

	if (!RemoveHand(power->first->_hand, left))
		return ErrCardDeletion;

	MergeDiscards(g, left);

	BOOST_FOREACH(auto i, right)
	{
		power->first->_civCards._cards.insert(i);
		std::cout << "Adding: " << i->_name << std::endl;
	}

	out << power->first->_name << " Gives: \n";
	if (free)
		out << "FREE,";
	if (tokens)
		out << tokens << ',';
	BOOST_FOREACH(auto i, left)
	{
		out << i->_name << ',';
	}
	out << "\tTotal: " << value+tokens;
	out << std::endl << std::endl;
	
	out << "For: \n";
	BOOST_FOREACH(auto i, right)
	{
		out << i->_name << ',';
	}
	out << "\tTotal: " << cost;
	out << std::endl;
	
	return ErrNone;
}
REG_PARSE(Buy, "Power Card/Token#/Free ... CivCard ...");

int parseCost(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 2 && names.size() != 3)
		return parseHelpC(names, g, out);

	if (names.size() == 2)
	{
		auto card = g.FindCivCard(names[1]);
		if (!card)
			return ErrUnableToParse;
		ShowCard(card);
		return ErrNone;
	}
	else if (names.size() == 3)
	{
		auto power = g.FindPower(names[1]);
		if (power == g._powers.end())
			return ErrPowerNotFound;

		auto card = g.FindCivCard(names[2]);
		if (!card)
			return ErrUnableToParse;

		int cost = power->first->_civCards.Cost(card);	
		std::cout << card->_name << " costs " << power->first->_name << " " << cost << std::endl;
		return ErrNone;
	}
	return ErrUnableToParse;
	
}
REG_PARSE(Cost, "CivCard");

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

	BOOST_FOREACH(auto i, left)
	{
		out << i->_name << ',';
	}
	std:: cout << std::endl << std::endl;
	
	return ErrNone;
}
REG_PARSE(Give, "FromPower ToPower Card/Random");

int parseGrant(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 4)
		return parseHelpC(names, g, out);

	auto i = g.FindPower(names[1]);
	if (i == g._powers.end())
		return ErrPowerNotFound;

	int group = CivCard::groupFromString(names[2]);
	if (group == CivCard::GroupSize)
		return ErrGroupNotFound;

	int quantity = 0;
	if (boost::algorithm::all(names[3], boost::algorithm::is_digit()))
			quantity = boost::lexical_cast<int>(names[3]);

	i->first->_civCards._bonusCredits[group] += quantity;
	
	out << "Granted: " << quantity << " to " << i->first->_name << std::endl;
	out << "Total now, " << i->first->_civCards._bonusCredits[group] << std::endl;
	return ErrNone;
}
REG_PARSE(Grant, "");

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
		BOOST_FOREACH(auto i, g._vars)
		{
			out << i.first << ": '" << i.second << "'" << std::endl;	
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
	for(int i = 0; i < picks.size(); ++i)
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
	BOOST_FOREACH(auto card, picks)
	{
		PickCard(g, temp2, card);
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
	fs::ofstream civcardList(base/"civcardList", std::ios::binary);
	for(int i = 0; i < g._decks.size(); ++i)
	{
		BOOST_FOREACH(auto j, g._cards)
		{
			if (i == j->_deck && j->_type == Card::Normal)
				cardList << i << '\t' << j->_maxCount << '\t' << j->_name << std::endl;
		}	

		BOOST_FOREACH(auto j, g._cards)
		{
			if (i == j->_deck && j->_type != Card::Normal)
			{
				cardList << j->_type << '\t' << 1 << '\t' << j->_name << std::endl;
			}
		}
	}

	BOOST_FOREACH(auto i, g._powers)
	{
		powerList << i.first->_name << std::endl;
	}

	civcardList << "COST\t"
		<< "TECHNOLOGY\t"
		<< "C\tS\tA\tV\tR\t"
		<< "Craft\tScience\tArt\tCivic\tReligion\t"
		<< "Extra\tCredit\tAbbreviation\tEvil\t"
		<< std::endl;

	BOOST_FOREACH(auto i, g._civcards)
	{
		civcardList << i->_cost << '\t';
		civcardList << i->_name << '\t';
		for(int group = 0; group < i->_groups.size(); ++group)
		{
			civcardList << (i->_groups[group]?'1':' ') << '\t';
		}
		BOOST_FOREACH(auto group, i->_groupCredits)
		{
			civcardList << group << '\t';
		}
		if (i->_cardCredits.size())
		{
			const auto &bonus = *i->_cardCredits.begin();
			civcardList << bonus.first->_abbreviation << '\t';
			civcardList << bonus.second << '\t';
		}
		else
		{
			civcardList << '\t' << '\t';
		}
		civcardList << i->_abbreviation << '\t';
		civcardList << (i->_evil?'Y':'N') << std::endl;
	}
	return ErrNone;
}
REG_PARSE(Dump,"DestinationDir");

void ExportCivCards(std::ostream &out, const Game &g)
{
	out << "<table id='civcard'>" << std::endl;
	out << "<tr>";
	out << "<th>Cost</th>";
	out << "<th colspan='3'></th>";
	BOOST_FOREACH(auto power, g._powers)
	{
		out << "<th>" << power.first->_name << "</th>";
	}
	out << "</tr>" << std::endl;
	out << "<tbody>" << std::endl;
	BOOST_FOREACH(auto card, g._civcards)
	{
		out << "<tr>";
		out << "<td>";
		out << card->_cost;
		out << "</td>";
		out << "<td>";
		out << card->_name;
		out << "</td>";
		out << "<td colspan='2'>";
		out << "<table height=20 width=40 cellspacing=0>";
		out << "<tr>";
		std::string color;

		for(int i = 0; i < CivCard::GroupSize; ++i)
		{
			if (card->_groups[i])
			{
				out << "<td class='" << CivCard::_groupList[i].first 
					<< "'></td>";
			}
		}
		out << "</tr>";
		out << "</table>";
		out << "</td>";
		BOOST_FOREACH(auto power, g._powers)
		{
			out << "<td class='num'>";
			if (power.first->Has(card))
				out << "&#x2713";
			else
				out << power.first->_civCards.Cost(card);
			out << "</td>";
		}
		out << "</tr>" << std::endl;

	}

	out << "<tr>";
	for(int i = 0; i != CivCard::GroupSize; ++i)
	{
		out << "<td colspan='4' class='" << CivCard::_groupList[i].first 
			<< "'>Bonus " << CivCard::_groupList[i].first << "</td>";
		BOOST_FOREACH(auto power, g._powers)
		{
			out << "<td class='num'>";
			if (power.first->_civCards._bonusCredits[i])
				out << power.first->_civCards._bonusCredits[i];
			else out << "&nbsp;";
			out << "</td>";
		}
		out << "</tr>" << std::endl;
	}

	out << "<tr></tr>";
	for(int i = 0; i != CivCard::GroupSize; ++i)
	{
		out << "<tr>";
		out << "<td colspan='4' class='" << CivCard::_groupList[i].first
		   	<< "'> " << CivCard::_groupList[i].first << " Total</td>";
		BOOST_FOREACH(auto power, g._powers)
		{
			int groupTotal = 0;
			out << "<td class='num'>";
			BOOST_FOREACH(auto card, power.first->_civCards._cards)
			{
				if (card->_groupCredits[i])
					groupTotal += card->_groupCredits[i];
			}
			groupTotal += power.first->_civCards._bonusCredits[i];
			out << groupTotal << "</td>";
		}
		out << "</tr>" << std::endl;
	}
	out << "</tbody>" << std::endl;
}

int parseExport(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 2)
		return parseHelpC(names, g, out);

	fs::path base;
	if (boost::iequals(names[1], "default"))
		base = g._vars["export"];
	else
		base = names[1];

	fs::ofstream auth(base / "auth");
	fs::ofstream contact(base / "contact");
	fs::ofstream civCards(base / "civcards");

	ExportCivCards(civCards, g);

	BOOST_FOREACH(auto i, g._powers)
	{
		if (!i.second)
			continue;
		fs::ofstream out(base / i.first->_name, std::ios::binary);
		fs::ofstream civOut(base / ("civ" + i.first->_name), std::ios::binary);
		
		auth << i.first->_name << ' ' << i.second->_password << std::endl;
		contact << i.first->_name << '\t' << i.second->_name << '\t' << i.second->_email << std::endl;
		BOOST_FOREACH(auto j, i.first->_hand)
		{
			out << j->_image << std::endl;
		}
		BOOST_FOREACH(auto j, i.first->_civCards._cards)
		{
			civOut << j->_image << std::endl;
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
		for(int i = 1; i < g._discards.size(); ++i)
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
	RenderCivPortfolio(out, power->first->_civCards);

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
	for(int i = 5; i < names.size()-3; ++i)
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
	for(int i = 2; i < breakPoint; ++i)
	{
		CardP card = g.FindCard(names[i]);
		if (!card)
			return ErrCardNotFound;
		left.insert(card);
	}

	for(int i = breakPoint+1; i < names.size(); ++i)
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
	BOOST_FOREACH(auto i, left)
	{
		out << i->_name << ',';
	}
	out << std::endl << std::endl;
	
	out << power2->first->_name << " Gives: \n";
	BOOST_FOREACH(auto i, right)
	{
		out << i->_name << ',';
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
	for(int i = 0; i < card->_maxCount; ++i)
	{
		int maxCount = g._decks[card->_deck].size();
		int location = maxCount?CivRand(maxCount):0;
		out << "Inserting (" << card->_deck << ")(" << card->_type << ")(" << card->_maxCount << ")(" << card->_name << ") at "<< location << std::endl;
		deck.insert(deck.begin()+location, card);
	}
	
	return ErrNone;
}
REG_PARSE(ShuffleIn,"deck type maxCount cardName");

int parseRandom(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 2)
		return parseHelpC(names, g, out);

	out << CivRand(boost::lexical_cast<int>(names[1]))+1 << std::endl;
	return ErrNone;
}
REG_PARSE(Random, "max");

int parseReshuffle(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 1)
		return parseHelpC(names, g, out);

	for(int i = 0; i < g._discards.size(); ++i)
	{
		ShuffleIn(g._decks[i], g._discards[i]);
		g._discards[i].clear();
	}

	return ErrNone;
}
REG_PARSE(Reshuffle,"");

int parseValue(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() < 2)
		return parseHelpC(names, g, out);

	Hand h;
	int tokens = 0;
	for(auto i = names.begin()+1; i != names.end(); ++i)
	{
		CardP c = g.FindCard(*i);
		if (c)
			h.insert(c);
		else if (boost::algorithm::all(*i, boost::algorithm::is_digit()))
			tokens += boost::lexical_cast<int>(*i);
		else
			return ErrUnableToParse;
	}

	int v = ValueHand(h);
	RenderHand(out, h);
	out << v << '+' << tokens << " = " << v+tokens << std::endl;
	return ErrNone;
}
REG_PARSE(Value,"card/token# [card/token#] ...");

int parseList(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 2 && names.size() != 3)
		return parseHelpC(names, g, out);

	if (boost::iequals(names[1],"powers"))
	{
		BOOST_FOREACH(auto i, g._powers)
		{
			out << i.first->_name << '\t';
			BOOST_FOREACH(auto j, i.first->_hand)
			{
				out << j->_name << ',';
			}
			out << std::endl;
		}
		return ErrNone;	
	}
	if (boost::iequals(names[1],"players"))
	{
		BOOST_FOREACH(auto i, g._powers)
		{
			out << i.first->_name << '\t';
			if (i.second)
				out << i.second->_name  << "\t'" << i.second->_password << "'\t'" << i.second->_email << "'"; 
			out << std::endl;
		}
		return ErrNone;
	}
	if (boost::iequals(names[1],"decks"))
	{
		for(int i = 1; i < g._decks.size(); ++i)
		{
			out << i << '\t';
			BOOST_FOREACH(auto j, g._decks[i])
			{
				out << j->_name << ',';
			}
			out << std::endl;
		}
		return ErrNone;
	}
	if (boost::icontains(names[1],"discard"))
	{
		for(int i = 1; i < g._discards.size(); ++i)
	        {
		        out << i << ": ";
	                RenderHand(out, g._discards[i]);
	        }
	        return ErrNone;
        }
	if (boost::icontains(names[1],"calamities"))
	{
		typedef std::multimap<CardP, PowerP, CardCompare> RevMap;
		RevMap calamities;
		BOOST_FOREACH(auto i, g._powers)
		{
			BOOST_FOREACH(auto j, i.first->_hand)
			{
				if (j->_type != Card::Normal)
					calamities.insert(std::make_pair(j,i.first));
			}
		}

		BOOST_FOREACH(auto i, calamities)
		{
			out << i.first->_name << ": " << i.second->_name << std::endl;
		}
		return ErrNone;
	}

	if (boost::icontains(names[1],"evil"))
	{
		BOOST_FOREACH(auto power, g._powers)
		{
			bool output = false;
			BOOST_FOREACH(auto card, power.first->_civCards._cards)
			{
				if (card->_evil)
				{
					if (!output)
					{
						out << power.first->_name << ": ";
						output = true;
					}
					out << card->_name << ' ';	
				}
			}
			if (output)
				out << std::endl;
		}
		return ErrNone;
	}
	if (boost::icontains(names[1],"points"))
	{
		BOOST_FOREACH(auto power, g._powers)
		{
			std::vector<std::pair<std::string,int>> points;
			points = CountPoints(g, *power.first);
			out << power.first->_name << ": ";
			BOOST_FOREACH(auto t, points)
			{
				out << t.first << '=' << t.second << ' ';
			}
			out << std::endl;
		}
		return ErrNone;
	}
/*
	if (boost::icontains(names[1],"civcards"))
	{
		auto power = g.FindPower(names[2]);
		if (power != g._powers.end())
		{
			return RenderPortfolio(power->_civCards);
		}
	}
*/	
	return ErrUnableToParse;
}
REG_PARSE(List,"powers/players/decks/discard/calamities/evil");

int parseCount(const std::vector<std::string> &names, Game &g, std::ostream &out)
{
	if (names.size() != 2)
		return parseHelpC(names, g, out);

	if (boost::iequals(names[1],"calamities"))
	{
		int bigCount(0); 
		BOOST_FOREACH(auto i, g._powers)
		{
			int count(0);
			int smallCount(0);
			BOOST_FOREACH(auto j, i.first->_hand)
			{
				if (j->_type != Card::Normal)
					++count;
				if (j->_type == Card::Minor)
					++smallCount;
			}
			out << i.first->_name << '\t' << count-smallCount << '\t' << smallCount << std::endl;
			bigCount+=count;
		}
		out << "Total:\t" << bigCount << std::endl;
		return ErrNone;
	}
	if (boost::iequals(names[1],"powers"))
	{
		int bigCount(0);
		BOOST_FOREACH(auto i, g._powers)
		{
			out << i.first->_name << '\t' << i.first->_hand.size() << std::endl;
			bigCount += i.first->_hand.size();
		}
		out << "Total:\t" << bigCount << std::endl;
		return ErrNone;
	}
	if (boost::iequals(names[1],"players"))
	{
		int count = 0;
		BOOST_FOREACH(auto i, g._powers)
		{
			if (i.second)
				++count;
		}
		out << "Assigned Player: " << count << std::endl;
		return ErrNone;
	}
	if (boost::iequals(names[1],"decks"))
	{
		int bigCount(0);
		for(int i = 1; i < g._decks.size(); ++i)
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
		for(int i = 1; i < g._discards.size(); ++i)
		{
			out << i << '\t' << g._discards[i].size() << std::endl;
			bigCount += g._discards[i].size();
		}
		out << "Total:\t" << bigCount << std::endl;
		return ErrNone;
	}
	if (boost::iequals(names[1],"evil"))
	{
		int bigCount(0);
		BOOST_FOREACH(auto i, g._powers)
		{
			int count(0);
			BOOST_FOREACH(auto j, i.first->_civCards._cards)
			{
				if (j->_evil)
					++count;
			}
			out << i.first->_name << '\t' << count << std::endl;
			bigCount+=count;
		}
		out << "Total:\t" << bigCount << std::endl;
		return ErrNone;
	}
	if (boost::iequals(names[1],"points"))
	{
		BOOST_FOREACH(auto power, g._powers)
		{
			std::vector<std::pair<std::string,int>> points;
			points = CountPoints(g, *power.first);
			out << power.first->_name << ": ";
			int total = 0;
			BOOST_FOREACH(auto t, points)
			{
				total += t.second;
			}
			out << total;
			out << std::endl;
		}
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

	rule<> word = (+(alnum_p | '&' | '-' | '~' | ':' | '_' | '(' | ')' | "\\ " | '.' | '/' | '@'))[push_back_a(target)];
	rule<> sentence = *(*space_p >> word);
	
	if (!parse(trimmed.c_str(), sentence).full)
		return false;
	
	BOOST_FOREACH(auto &word, target)
	{
		boost::replace_all(word, "\\ ", " ");
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
