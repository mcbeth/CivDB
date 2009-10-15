#ifndef PARSER_H__
#define PARSER_H__

#include "factory.h"
#include "db.h"

#include <boost/function.hpp>
#include <boost/serialization/singleton.hpp>
#include <iostream>
#include <vector>

typedef boost::function<int (const std::vector<std::string> &, Game &, std::ostream &)> ParseFunc;

typedef FactoryOwner<ParseFunc> Parser;
typedef boost::serialization::singleton<Parser> ParseFactory;

#define REG_PARSE(_trigger, _str) \
	static bool _trigger ## _registered = \
		ParseFactory::get_mutable_instance().Register(#_trigger, parse##_trigger); \
	static bool _trigger ## _help_registered = \
		HelpFactory::get_mutable_instance().Register(#_trigger, \
				boost::bind(GenericHelp,#_trigger " " _str,_1))



#endif
