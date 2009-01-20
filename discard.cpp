#include "db.h"
#include "dbUtils.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		std::cerr << argv[0] << " Database Power Card [Card] ..." << std::endl;
		return 1;
	}

	Game g(argv[1]);
	
	Powers::iterator power;
	if ((power = g.FindPower(argv[2])) == g._powers.end())
	{
		return 2;
	}

	const std::vector<std::string> cardList(&argv[3],&argv[argc]);

	Hand toss;
	FillHand(g,cardList, toss);
	FillCalamities(g,*power->first,toss);
	
	//RemoveHand(power->first->_hand, toss);
	//MergeDiscards(g, toss);
	
	//RenderHand(std::cout, power->first->_hand);
	return 0;
}
