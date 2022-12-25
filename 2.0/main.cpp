//
//  main.cpp
//  John Conway's Game of Life
//
//  Created by Denis Fedorov on 17.12.2022.
//

#if defined(_WIN32)
#include <Windows.h>
#endif
#include "game.h"

int main(int argc, const char * argv[])
{
#if defined(_WIN32)
	SetConsoleOutputCP(CP_UTF8);
#endif
	Game game;
	if (argc == 2)
	{
		game.run(argv[1]);
	}
	else
	{
		game.run();
	}
	return 0;
}
