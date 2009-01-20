#include "db.h"
#include "dbUtils.h"

#include <iostream>

int main(int argc, char *argv[])
{
	int numPicks = argc-4;

	if (numPicks < 0)
	{
		std::cerr << argv[0] << " Database Power NumCards [Pick1] [Pick2] ..." << std::endl;
		return 1;
	}

	int numCards = boost::lexical_cast<int>(argv[3]);
	std::vector<int> picks(numPicks);
	for(int i = 0; i < numPicks; i++)
	{
		picks[i] = boost::lexical_cast<int>(argv[4+i]);
	}

	Game g(argv[1]);
	
	Powers::iterator power;
	if ((power = g.FindPower(argv[2])) == g._powers.end())
	{
		return 2;
	}
	
	std::cout << "Held:" << std::endl;
	RenderHand(std::cout, power->first->_hand);
	
	Hand tempHand;
	std::cout << std::endl << "Drawn:" << std::endl;
	DrawCards(g, tempHand, numCards);
	RenderHand(std::cout, tempHand);
	
	Hand temp2;
	std::cout << std::endl << "Bought:" << std::endl;
	for(int i = 0; i < numPicks; i++)
	{
		PickCard(g, temp2, picks[i]);
	}
	RenderHand(std::cout, temp2);
	
	MergeHands(tempHand, temp2);
	MergeHands(power->first->_hand, tempHand);
	
	return 0;
}
