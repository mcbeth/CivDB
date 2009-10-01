#include "dbUtils.h"

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		std::cerr << argv[0] << " Game Power Password" << std::endl;
		return ErrUnableToParse;
	}

	const Game g(argv[1],true);
	const std::string power(argv[2]);
	const std::string passwd(argv[3]);

	Powers::const_iterator p = g.FindPower(power);
	if (p == g._powers.end())
		return ErrPowerNotFound;

	if (p->second->_password != passwd)
		return ErrPowerNotFound;

	RenderHand(std::cout, p->first->_hand);
	return ErrNone;
}
