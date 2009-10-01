#include "dbUtils.h"

int main(int argc, char *argv[])
{
	const Game g(argv[1],true);
	const Game ga(argv[2],true);

	for(Powers::const_iterator i = g._powers.begin(); i != g._powers.end(); i++)
	{
		std::cout << i->first->_name 
			<< '\t' << ValueHand(i->first->_hand) << '\t';

		for(Powers::const_iterator j = ga._powers.begin(); i != ga._powers.end(); j++)
		{
			if (i->first->_name == j->first->_name)
			{
				std::cout << ValueHand(j->first->_hand);
				break;
			}
		}
		std::cout << std::endl;
	}

	return ErrNone;
}
