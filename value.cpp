#include "dbUtils.h"
#include <boost/foreach.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
	const Game g(argv[1],true);
	const Game ga(argv[2],true);

	BOOST_FOREACH(auto power, g._powers)
	{
		std::cout << power.first->_name 
			<< '\t' << ValueHand(power.first->_hand) << '\t';

		BOOST_FOREACH(auto p2, ga._powers)
		{
			if (power.first->_name == p2.first->_name)
			{
				std::cout << ValueHand(p2.first->_hand);
				break;
			}
		}
		std::cout << std::endl;
	}

	return ErrNone;
}
