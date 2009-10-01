#ifndef DB_H__
#define DB_H__

#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/deque.hpp>

#include <boost/nondet_random.hpp>

#include <string>

class Card
{
	public:
	enum Type{
		Normal,
		Tradable = -1,
		NonTradable = -2,
		Minor = -3,
	};	
	
	std::string _name;
	int _deck;
	int _maxCount;
	bool _supplement;
	Type _type;
	std::string _image;
};

typedef boost::shared_ptr<Card> CardP;

class CardCompare
{
	public:
	bool operator()(const CardP &lhs, const CardP &rhs) const;
};

typedef std::set<CardP,CardCompare> Cards;
typedef std::multiset<CardP,CardCompare> Hand;
typedef std::vector<Hand> Hands;
typedef boost::shared_ptr<Hand> HandP;

typedef std::deque<CardP> Deck;
typedef boost::shared_ptr<Deck> DeckP;

class Power
{
	public:
	std::string _name;  
	Hand _hand;
	Hand _staging;
	
	public:
		void Merge();
 		bool Has(const Hand &cards) const;
		bool Stage(const Hand &cards);
};

typedef boost::shared_ptr<Power> PowerP;

class Player
{
	public:
	std::string _name;
	std::string _password;
	std::string _email;
};

typedef boost::shared_ptr<Player> PlayerP;

typedef std::map<PowerP, PlayerP> Powers;
typedef std::vector<Deck> Decks;

class Game
{
	public:
	Game(const std::string &f, bool abandon=false):_fn(f),_abandon(abandon){Load(_fn);}
	~Game(){Save(_fn);}
	
	std::string _name;
	std::string _url;
	std::string _ruleset;
	Powers _powers;
	Decks _decks;
	Hands _discards;
	Cards _cards;
	
	Powers::iterator FindPower(const std::string &);
	Powers::const_iterator FindPower(const std::string &) const;

	CardP FindCard(const std::string &) const;
	void Abandon();
	
	private:
		std::string _fn;
		bool _abandon; // We don't want this state saveable
		void Load(const std::string &);
		void Save(const std::string &);
};

int CivRand(int n);
#endif
