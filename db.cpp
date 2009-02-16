#include "db.h"

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/algorithm/string.hpp>

#include <fstream>


namespace boost { namespace serialization {

	template<class Archive>
	void serialize(Archive &ar, Card &c, unsigned int version)
	{
		ar & make_nvp("name",c._name);
		ar & make_nvp("deck",c._deck);
		ar & make_nvp("maxCount",c._maxCount);
		ar & make_nvp("type", c._type);
		ar & make_nvp("image", c._image);
	}
	
	template<class Archive>
	void serialize(Archive &ar, Power &p, unsigned int version)
	{
		ar & make_nvp("name",p._name) & make_nvp("hand",p._hand);
		if (version > 0)
			ar & make_nvp("staging",p._staging);
	}
	
	template <class Archive>
	void serialize(Archive &ar, Player &p, unsigned int version)
	{
		ar & make_nvp("name",p._name);
		ar & make_nvp("password",p._password);
		ar & make_nvp("email",p._email);
	}

	template <class Archive>
	void serialize(Archive &ar, Game &g, unsigned int version)
	{
		ar & make_nvp("name",g._name);
		ar & make_nvp("url", g._url);
		ar & make_nvp("cards", g._cards);
		ar & make_nvp("powers", g._powers);
		ar & make_nvp("decks", g._decks);
		ar & make_nvp("discards", g._discards);
	}	
}}

BOOST_CLASS_VERSION(Power, 1)

bool CardCompare::operator()(const CardP &lhs, const CardP &rhs) const
{
	if (lhs->_deck < rhs->_deck)
		return true;
	if (lhs->_deck > rhs->_deck)
		return false;
	
	if (lhs->_maxCount < rhs->_maxCount)
		return true;
	if (lhs->_maxCount > rhs->_maxCount)
		return false;

	if (lhs->_name < rhs->_name)
		return true;

	return false;
}

void Game::Save(const std::string &filename)
{
	std::ofstream out(filename.c_str(), std::ios::binary);
	boost::archive::xml_oarchive oa(out);
	oa << boost::serialization::make_nvp("Game",*this);
}

void Game::Load(const std::string &filename)
{
	std::ifstream in(filename.c_str(), std::ios::binary);
	if (!in.is_open())
		return;
	boost::archive::xml_iarchive ia(in);
	ia >> boost::serialization::make_nvp("Game",*this);
}

bool Power::Has(const Hand &cards) const
{
	for(Hand::const_iterator i = _hand.begin(); i != _hand.end(); i++)
	{
		if (cards.count(*i) > _hand.count(*i))
			return false;
	}
	
	return true;
}

void Power::Merge()
{
	_hand.insert(_staging.begin(), _staging.end());
	_staging.clear();
}

Powers::const_iterator Game::FindPower(const std::string &powerName) const
{
	for(Powers::const_iterator i = _powers.begin(); i != _powers.end(); i++)
	{
		if (boost::iequals(i->first->_name,powerName))
			return i;
	}
	return _powers.end();
}

Powers::iterator Game::FindPower(const std::string &powerName)
{
	for(Powers::iterator i = _powers.begin(); i != _powers.end(); i++)
	{
		if (boost::iequals(i->first->_name,powerName))
			return i;
	}
	return _powers.end();
}

CardP Game::FindCard(const std::string &cardName) const
{
	for(Cards::iterator i = _cards.begin(); i != _cards.end(); i++)
	{
		if (boost::iequals((*i)->_name, cardName))
			return *i;
	}
	
	return CardP();
}

int CivRand(int n)
{
	static boost::random_device _dev;
	return int(_dev()%n);
}