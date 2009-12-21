#include "dbUtils.h"

#include <fstream>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

int ValueHand(const Hand &hand)
{
	int value = 0;
	CardP currentCard;
	BOOST_FOREACH(auto i, hand)
	{
		if (currentCard == i)
			continue;
		currentCard = i;
		const int count = hand.count(currentCard);
		const int deck = currentCard->_deck;
		if (currentCard->_type == Card::Normal)
			value += count*count*deck;
	}
	return value;
}

void RenderHand(std::ostream &out, const Hand &hand)
{
	if (hand.size() == 0)
	{
		out << "None" << std::endl;
		return;
	}
	
	CardP currentCard;
	BOOST_FOREACH(auto i, hand)
	{
		if (currentCard == i)
			continue;
		currentCard = i;
		const int count = hand.count(currentCard);
		const int deck  = currentCard->_deck;
		out << count << "x " << currentCard->_name << " (" << deck <<")"<< std::endl;
	}
	int value = ValueHand(hand);
	if (value > 0)
		out << "Value: " << value << std::endl;
}

void RenderDeck(std::ostream &out, const Deck &deck)
{
	if (deck.size() == 0)
	{
		out << "None" << std::endl;
		return;
	}
	
	BOOST_FOREACH(auto card, deck)
	{
		out << card->_name << std::endl;
	}
}

void RenderCivPortfolio(std::ostream &out, const CivPortfolio &cards)
{
	const std::string groupName[] = {"Craft", "Science", "Art", "Civic", "Religion"};
	BOOST_FOREACH(auto card, cards._cards)
	{
		out << card->_name << std::endl;
	}
	for(int i = 0; i < 5; i++)
	{
		out << groupName[i] << '\t';
	}
	out << std::endl;
	BOOST_FOREACH(auto credits, cards._bonusCredits)
	{
		out << credits << '\t';
	}
	out << std::endl;
}

void MergeHands(Hand &target, const Hand &src)
{
	target.insert(src.begin(), src.end());
}

bool RemoveHand(Hand &src, const Hand &deleted)
{
	BOOST_FOREACH(auto card, deleted)
	{
		Hand::iterator v = src.find(card);
		if (v == src.end())
			return false;
		src.erase(v);
	}
	return true;
}

bool Stage(Hand &src, Hand &dest, const Hand &cards)
{
	if (!RemoveHand(src, cards))
		return false;
	dest = cards;
	return true;
}

void MergeDiscards(Game &g, const Hand &toss)
{
	BOOST_FOREACH(auto card, toss)
	{
		g._discards[card->_deck].insert(card);
	}
}

void ShowCard(CivCardP c)
{
	const std::string groups[] = {"Craft", "Science", "Art", "Civic", "Religion"};
	std::cout << c->_cost << '\t' << c->_name << " (" << c->_abbreviation << ')' << '\t';
	for(int i = 0; i < CivCard::GroupSize; i++)
	{
		if (c->_groups[i])
			std::cout << groups[i] << '\t';
	}
	for(int i = 0; i < CivCard::GroupSize; i++)
	{
		if (c->_groupCredits[i] > 0)
			std::cout << groups[i] << " (" << c->_groupCredits[i] << ')' << '\t';
	}
	BOOST_FOREACH(auto i, c->_cardCredits)
	{
		std::cout << i.first->_name << " (" << i.second << ")" << '\t';
	}
	if (c->_evil)
		std::cout << "EVIL";
	std::cout << std::endl;
}

bool ParseCivCards(const std::string &filename, Game &g)
{
	std::cout << "Parsing " << filename << std::endl;
	std::ifstream in(filename.c_str(), std::ios::binary);
	std::string line;
	std::getline(in, line);
	std::map<CivCardP, std::pair<std::string, int>> bonus;
	while(in.good())
	{
		std::vector<std::string> values;
		std::getline(in, line);
		if (!in.good())
				break;

		boost::algorithm::split(values, line, boost::algorithm::is_any_of("\t"));
		
		CivCardP card(new CivCard);

		card->_cost = boost::lexical_cast<int>(values[0]);
		card->_name = values[1];
		for(int i = 2; i < 7; i++)
		{
			card->_groups[i-2] = values[i].size()>0;
		}
		for(int i = 7; i < 12; i++)
		{
			if (values[i].size())
				card->_groupCredits[i-7] = boost::lexical_cast<int>(values[i]);
			else
				card->_groupCredits[i-7] = 0;
		}
		if (values[12].size())
		{
			bonus[card] = std::make_pair(values[12],boost::lexical_cast<int>(values[13]));
		}
		card->_abbreviation = values[14];
		card->_evil = values[15][0] == 'Y';
		card->_image = "";
		g._civcards.insert(card);
	}
	BOOST_FOREACH(auto card, bonus)
	{
		BOOST_FOREACH(auto match, g._civcards)
		{
			if (card.second.first == match->_abbreviation)
				card.first->_cardCredits[match] = card.second.second;
		}
	}

	BOOST_FOREACH(auto card, g._civcards)
		ShowCard(card);
}

bool ParseCardLists(const std::string &filename, Game &g)
{
	int lastRead = 0;
	std::ifstream in(filename.c_str(), std::ios::binary);
	
	while(in.good())
	{
		CardP card(new Card);
		in >> card->_deck >> card->_maxCount;
		if (g._vars["ruleset"] != "AdvCiv")
			in >> card->_supplement;
		else
			card->_supplement = false;
		if (card->_deck < 0)
		{
			card->_type = static_cast<Card::Type>(card->_deck);
			card->_deck = lastRead;
		}
		else
		{
			card->_type = Card::Normal;
			lastRead = card->_deck;
		}
		
		std::getline(in, card->_name);
		boost::trim(card->_name);
		
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
		
		if (in.good())
			g._cards.insert(card);
	}
	
	return g._cards.size();
}

void ShuffleIn(Deck &d, Hand &hand)
{
	Deck shuffled;
	Deck unshuffled;

	BOOST_FOREACH(auto i, hand)
	{
		if (i->_type == Card::NonTradable)
			unshuffled.push_back(i);
		else
			shuffled.push_back(i);
	}

	std::random_shuffle(shuffled.begin(), shuffled.end(), CivRand);
	std::random_shuffle(unshuffled.begin(), unshuffled.end(), CivRand);

	d.insert(d.end(), shuffled.begin(), shuffled.end());
	d.insert(d.end(), unshuffled.begin(), unshuffled.end());
}

bool CreateDecksCivProject30(Game &g)
{
	g._decks.resize(10);
	g._discards.resize(10);

	for(int i = 1; i < g._decks.size(); ++i)
	{
		std::vector<CardP> holding;
		std::vector<CardP> supplement;
		std::vector<CardP> nonTrade;
		g._decks[i].clear();
		BOOST_FOREACH(auto j, g._cards)
		{
			if (j->_deck != i)
				continue;
			if (j->_supplement)
			{
				supplement.insert(supplement.end(), j->_maxCount, j);
				continue;
			}
			if (j->_type == Card::NonTradable)
			{
				nonTrade.push_back(j);
				continue;
			}
			if (j->_type == Card::Tradable)
			{
				g._decks[i].insert(g._decks[i].end(), j);
				continue;
			}
			if (j->_type == Card::Normal || j->_type == Card::Minor)
			{
				holding.insert(holding.end(), j->_maxCount, j);
			}
		}
		std::random_shuffle(holding.begin(), holding.end(), CivRand);

		g._decks[i].insert(g._decks[i].begin(),holding.begin(), holding.end());
		g._decks[i].insert(g._decks[i].end(), supplement.begin(), supplement.end());
		g._decks[i].insert(g._decks[i].end(), nonTrade.begin(), nonTrade.end());
	}
	return true;
}

bool CreateDecksAdvCiv(Game &g)
{
	const int numPlayers = g._powers.size();
	std::vector<CardP> holding;
	g._decks.resize(10);
	g._discards.resize(10);
	
	for(int i = 1; i < g._decks.size(); ++i)
	{
		holding.clear();

		BOOST_FOREACH(auto card, g._cards)
		{
			if (card->_deck == i && (card->_type == Card::Normal || card->_type == Card::Minor))

			{
				holding.insert(holding.end(), card->_maxCount, card);
			}
		}

		std::random_shuffle(holding.begin(), holding.end(), CivRand);
		
		for(int j = 0; j < numPlayers && holding.size(); ++j)
		{
			CardP b = holding.back();
			g._decks[i].push_back(b);
			holding.pop_back();
		}
		
		BOOST_FOREACH(auto card, g._cards)
		{
			if (card->_deck == i && (card->_type == Card::Tradable))
			{
				holding.push_back(card);
			}
		}
		
		std::random_shuffle(holding.begin(), holding.end(), CivRand);
		
		for(Cards::const_iterator j = g._cards.begin(); j != g._cards.end(); j++)
		BOOST_FOREACH(auto card, g._cards)
		{
			if (card->_deck == i && card->_type == Card::NonTradable)
			{
				holding.push_back(card);
			}			
		}
	
		if (!holding.size())
			return false;
		
		g._decks[i].insert(g._decks[i].end(), holding.begin(), holding.end());
	}
	
	return true;
}

bool CreateDecks(Game &g)
{
	if (g._vars["ruleset"] == "AdvCiv")
		return CreateDecksAdvCiv(g);
	if (g._vars["ruleset"] == "CivProject30")
		return CreateDecksCivProject30(g);
	return false;
}

bool ParsePowerList(const std::string &filename, Game &g)
{
	PlayerP player;
	std::ifstream in(filename.c_str(), std::ios::binary);
	while(in.good())
	{
		PowerP power(new Power);
		std::getline(in, power->_name);
		if (in.good())
			g._powers[power] = player;
	}
	return g._powers.size() ;
}

bool CreateGame(const std::string &cards, const std::string &powers, const std::string &ruleset, Game &g)
{
	g._vars["name"] = "";
	g._vars["url"] = "";
	g._vars["ruleset"] = ruleset;
	g._vars["export"] = "";
	g._powers.empty();
	g._decks.empty();
	g._discards.empty();
	g._cards.empty();

	return ParseCardLists(cards, g) &&
		ParsePowerList(powers,g) &&
		CreateDecks(g);
}

bool FillHand(const Game &g, const std::vector<std::string> &cardNames, Hand &hand)
{
	for(int i = 0; i < cardNames.size(); ++i)
	{
		CardP c = g.FindCard(cardNames[i]);
		if (!c)
			return false;
		
		hand.insert(c);
	}
	
	return true;
}

void FillCalamities(const Game &g, const Power &p, Hand &hand)
{
	BOOST_FOREACH(auto card, p._hand)
	{
		if (card->_type != Card::Normal)
		{
			hand.insert(card);
		}
	}
}

void PickCard(Game &g, Hand &hand, int i)
{
	if (g._decks[i].size() == 0)
		return;
	hand.insert(g._decks[i].front());
	g._decks[i].pop_front();	
}

void DrawCards(Game &g, Hand &hand, int numCards)
{
	for(int i = 1; i <= numCards; ++i)
	{
		PickCard(g, hand, i);
	}
}
