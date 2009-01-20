#include "db.h"
#include "dbUtils.h"

#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		std::cerr << argv[0] << " DatabaseName CardList PowerList" << std::endl;
		return 1;
	}

	std::ifstream t(argv[1]);
	if (t.is_open())
	{
		std::cerr << "Database already exists!" << std::endl;
		return 2;
	}
	
	Game g(argv[1]);
	CreateGame(argv[2],argv[3],g);
	return 0;
}
