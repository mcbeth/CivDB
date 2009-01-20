#include "db.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
	if (argc != 6)
	{
		std::cerr << argv[0] << " Database Power Name Password EMail" << std::endl;
		return 1;
	}

	Game g(argv[1]);
	
	Powers::iterator power;
	if ((power = g.FindPower(argv[2])) == g._powers.end())
	{
		return 2;
	}

	PlayerP player(new Player);
	player->_name = argv[3];
	player->_password = argv[4];
	player->_email = argv[5];
	
	power->second = player;

	return 0;
}
