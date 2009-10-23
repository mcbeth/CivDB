#include "dbUtils.h"

#include <fstream>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

int ValueHand(const Hand &hand)
{
	int value = 0;
	CardP currentCard;
	for(Hand::const_iterator i = hand.begin(); i != hand.end(); i++)
	{
		if (currentCard == (*i))
			continue;
		currentCard = *i;
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
	for(Hand::const_iterator i = hand.begin(); i != hand.end(); i++)
	{
		if (currentCard == (*i))
			continue;
		currentCard = *i;
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
	
	for(Deck::const_iterator i = deck.begin(); i != deck.end(); i++)
	{
		out << (*i)->_name << std::endl;
	}
}

void MergeHands(Hand &target, const Hand &src)
{
	target.insert(src.begin(), src.end());
}

bool RemoveHand(Hand &src, const Hand &deleted)
{
	for(Hand::const_iterator i = deleted.begin(); i != deleted.end(); i++)
	{
		Hand::iterator v = src.find(*i);
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
	for(Hand::const_iterator i = toss.begin(); i != toss.end(); i++)
	{
		g._discards[(*i)->_deck].insert(*i);
	}
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

	for(Hand::const_iterator i = hand.begin(); i != hand.end(); i++)
	{
		if ((*i)->_type == Card::NonTradable)
			unshuffled.push_back(*i);
		else
			shuffled.push_back(*i);
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

	for(int i = 1; i < g._decks.size(); i++)
	{
		std::vector<CardP> holding;
		std::vector<CardP> supplement;
		std::vector<CardP> nonTrade;
		g._decks[i].clear();
		for(Cards::const_iterator j = g._cards.begin(); j != g._cards.end(); j++)
		{
			if ((*j)->_deck != i)
				continue;
			if ((*j)->_supplement)
			{
				supplement.insert(supplement.end(), (*j)->_maxCount, *j);
				continue;
			}
			if ((*j)->_type == Card::NonTradable)
			{
				nonTrade.push_back(*j);
				continue;
			}
			if ((*j)->_type == Card::Tradable)
			{
				g._decks[i].insert(g._decks[i].end(), *j);
				continue;
			}
			if ((*j)->_type == Card::Normal || (*j)->_type == Card::Minor)
			{
				holding.insert(holding.end(), (*j)->_maxCount, *j);
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
	
	for(int i =1; i < g._decks.size(); i++)
	{
		holding.clear();

		for(Cards::const_iterator j = g._cards.begin(); j != g._cards.end(); j++)
		{
			if ((*j)->_deck == i && ((*j)->_type == Card::Normal || (*j)->_type == Card::Minor))

			{
				holding.insert(holding.end(), (*j)->_maxCount, *j);
			}
		}

		std::random_shuffle(holding.begin(), holding.end(), CivRand);
		
		for(int j = 0; j < numPlayers && holding.size(); j++)
		{
			CardP b = holding.back();
			g._decks[i].push_back(b);
			holding.pop_back();
		}
		
		for(Cards::const_iterator j = g._cards.begin(); j != g._cards.end(); j++)
		{
			if ((*j)->_deck == i && ((*j)->_type == Card::Tradable))
			{
				holding.push_back(*j);
			}
		}
		
		std::random_shuffle(holding.begin(), holding.end(), CivRand);
		
		for(Cards::const_iterator j = g._cards.begin(); j != g._cards.end(); j++)
		{
			if ((*j)->_deck == i && (*j)->_type == Card::NonTradable)
			{
				holding.push_back(*j);
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
	for(int i = 0; i < cardNames.size(); i++)
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
	for(Hand::const_iterator i = p._hand.begin(); i != p._hand.end(); i++)
	{
		if ((*i)->_type != Card::Normal)
		{
			hand.insert(*i);
		}
	}
	
//	for(Hand::const_iterator i = hand.begin(); i != hand.end(); i++)
//	{
//		std::cerr << (*i)->_name << '\t';
//	}
//	std::cerr << std::endl;	
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
	for(int i = 1; i <= numCards; i++)
	{
		PickCard(g, hand, i);
	}
}
