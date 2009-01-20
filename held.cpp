#include "db.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr << argv[0] << " Database Power" << std::endl;
		return 1;
	}

	Game g(argv[1]);
	
	Powers::iterator power;
	
	if (boost::icontains(argv[2],"discard"))
	{
		for(int i = 1; i < g._discards.size(); i++)
		{
			std::cout << i << ": ";
			RenderHand(std::cout, g._discards[i]);
		}
		return 0;
	}
	
	if ((power = g.FindPower(argv[2])) == g._powers.end())
	{
		return 2;
	}
	
	RenderHand(std::cout, power->first->_hand);
	return 0;
}
