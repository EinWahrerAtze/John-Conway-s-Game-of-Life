//
//  main.cpp
//  John Conway's Game of Life
//
//  Created by Denis Fedorov on 14.01.2023.
//

#include "life.h"
#ifdef _WIN32
#include <Windows.h>
#endif

int main(int argc, const char * argv[])
{
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#endif
	game::life life;
	if (argc == 2)
	{
		life.begin(argv[1]);
	}
	else
	{
		life.begin();
	}
	return 0;
}
