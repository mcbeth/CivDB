#include "db.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>

namespace fs = boost::filesystem;

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr << argv[0] << " Database Destination" << std::endl;
		return 1;
	}

	fs::path base(argv[2]);
	
	Game g(argv[1]);
	
	fs::ofstream auth(base / "auth", std::ios::binary);
	fs::ofstream contact(base / "contact", std::ios::binary);
	for(Powers::const_iterator i = g._powers.begin(); i != g._powers.end(); i++)
	{
		fs::ofstream out(base / i->first->_name, std::ios::binary);
		
		auth << i->first->_name << ' ' << i->second->_password << std::endl;
		contact << i->first->_name << '\t' << i->second->_name << '\t' << i->second->_email << std::endl;
		for(Hand::const_iterator j = i->first->_hand.begin(); j != i->first->_hand.end(); j++)
		{
			out << (*j)->_image << std::endl;
		}
	}
	
	return 0;
}
