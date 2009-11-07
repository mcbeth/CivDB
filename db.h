#ifndef DB_H__
#define DB_H__

#include <boost/nondet_random.hpp>

#include <boost/algorithm/string.hpp>

#include <string>
#include <queue>
#include <bitset>
#include <boost/array.hpp>
#include <map>
#include <set>
#include <boost/shared_ptr.hpp>

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

class CivCard;
typedef boost::shared_ptr<CivCard> CivCardP;
class CivCard
{
	public:
		std::string _name;
		std::string _abbreviation;
		std::string _image;
		int _cost;
		enum Group{
			Craft,
			Science,
			Art,
			Civic,
			Religion,
			GroupSize,
		};
		typedef boost::array<int, GroupSize> GroupCredits;
		typedef std::bitset<GroupSize> Groups;

		Groups _groups;
		std::map<CivCardP, int> _cardCredits;
		GroupCredits _groupCredits;
		CivCard():_groups(0),_cost(0)
		{
			std::fill(_groupCredits.begin(), _groupCredits.end(), 0);
		}
};

class CivPortfolio
{
	public:
		int Cost(CivCardP card);
		std::set<CivCardP> _cards;
		CivCard::GroupCredits _bonusCredits;
		CivPortfolio()
		{
			std::fill(_bonusCredits.begin(), _bonusCredits.end(), 0);
		}
};

class CivCardCompare
{
	public:
	bool operator()(const CivCardP &lhs, const CivCardP &rhs) const;
};
typedef std::set<CivCardP, CivCardCompare> CivCards;

class Power
{
	public:
	std::string _name;  
	Hand _hand;
	Hand _staging;
	CivPortfolio _civCards;
	
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
	private:
	struct less
	{
		bool operator()(const std::string &l, const std::string &r) const
		{
			return boost::algorithm::ilexicographical_compare(l,r);
		}
	};
	typedef std::map<std::string, std::string, less> Variables;
	
	public:
	Game(const std::string &f, bool abandon=false):_fn(f),_abandon(abandon){Load(_fn);}
	~Game(){Save(_fn);}
	
	Powers _powers;
	Decks _decks;
	Hands _discards;
	Cards _cards;
	CivCards _civcards;
	std::queue<std::string> _queue;
	Variables _vars;
	
	Powers::iterator FindPower(const std::string &);
	Powers::const_iterator FindPower(const std::string &) const;

	CardP FindCard(const std::string &) const;
	CivCardP FindCivCard(const std::string &) const;
	void Abandon();
	
	private:
		std::string _fn;
		bool _abandon; // We don't want this state saveable
		void Load(const std::string &);
		void Save(const std::string &);
};

int CivRand(int n);
#endif
