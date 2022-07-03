#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

#define EXIT_CONDITION answer != 'e' && answer != 'E'
#define CHOICE_CONDITION answer != '1' && answer != '2' && answer != '3'
#define END_CONDITION answer != 'n' && answer != 'N'
#define ADD_CONDITION answer != 'y' && answer != 'Y'
#define RESTART_CONDITION answer != 'r' && answer != 'R'

int check_input()
{
	int temp = 0;

	while (!(std::cin >> temp) || (temp < 0))
	{

		std::cin.clear();
		while (std::cin.get() != '\n')
			continue;
		std::cout << "Please enter a positive value: ";
	}

	return temp;
}

int** create_world(const int rows, const int columns)
{
	int** game_field = new int* [rows];

	for (int i = 0; i < rows; ++i)
	{
		game_field[i] = new int[columns]();
	}

	return game_field;
}

int** manual_input(int& rows, int& columns)
{
	std::system("cls");
	std::cout << "Enter amount of rows: ";
	rows = check_input() + 2;
	std::cout << "Enter amount of columns: ";
	columns = check_input() + 2;

	while (rows - 2 < 2 || columns - 2 < 2)
	{

		std::cout << "Game field must contain more that one row and column, please repeat input.\n";
		std::cout << "Enter amount of rows: ";
		rows = check_input() + 2;
		std::cout << "Enter amount of columns: ";
		columns = check_input() + 2;
	}

	int** first_world = create_world(rows, columns);

	int i = 0;
	int j = 0;
	short count = 1;
	char answer = '0';

	while (END_CONDITION)
	{
		std::cout << "Enter position (row) of " << count << " alive cell: ";
		i = check_input() + 1;

		while (i > rows - 2)
		{
			std::cout << "Coordinate is out of range, repeat input: ";
			i = check_input() + 1;
		}

		std::cout << "Enter position (column) of " << count << " alive cell: ";
		j = check_input() + 1;

		while (j > columns - 2)
		{
			std::cout << "Coordinate is out of range, repeat input: ";
			j = check_input() + 1;
		}

		if (first_world[i][j] == 1)
		{
			std::cout << "This cell is already created!";
		}
		else
		{
			first_world[i][j] = 1;
		}

		std::cout << "\nWant to add another cell (" << count << " cell(s) created)? [Y] to enter, [N] to end: ";
		++count;
		std::cin >> answer;
		std::cout << std::endl;

		while (END_CONDITION && ADD_CONDITION)
		{
			std::cout << "Wrong input! Repeat: ";
			std::cin >> answer;
		}
	}

	return first_world;
}

int** auto_input(const char filename[], int& rows, int& columns)
{
	std::system("cls");
	std::ifstream fin(filename);

	if (!fin.is_open())
	{
		std::cout << "Could not open \"" << filename << "\" for reading!\n";
		std::cout << "Programm terminating.\n";
		exit(EXIT_FAILURE);
	}

	fin >> rows;
	rows = rows + 2;
	fin >> columns;
	columns = columns + 2;

	int** first_world = create_world(rows, columns);
	
	int i = 0;
	int j = 0;
	int count = 0;

	while (fin.good())
	{
		fin >> i;
		i = i + 1;

		if (i > rows - 2)
		{
			std::cout << "One of the values in file is out of starting coordinates!\n";
			break;
		}

		fin >> j;
		j = j + 1;

		if (j > columns - 2)
		{
			std::cout << "One of the values in file is out of starting coordinates!\n";
			break;
		}

		first_world[i][j] = 1;
		++count;
	}

	if (fin.eof())
	{
		std::cout << "End of file reached.\n";
		std::cout << "Cells created: " << count << std::endl;
	}
	else if (fin.fail())
	{
		std::cout << "Input terminated by data mismatch.\n";
	}
	else
	{
		std::wcout << "Input terminated by unknown reason.\n";
	}

	fin.close();

	std::cout << "Games starts in:\n";

	for (int i = 5; i > 0; --i)
	{
		std::cout << i << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return first_world;
}

int** random_input(int& rows, int& columns)
{
	std::system("cls");
	std::cout << "Enter amount of rows: ";
	rows = check_input() + 2;
	std::cout << "Enter amount of columns: ";
	columns = check_input() + 2;

	while (rows - 2 < 2 || columns - 2 < 2)
	{
		std::cout << "Game field must contain more that one row and column, please repeat input.\n";
		std::cout << "Enter amount of rows: ";
		rows = check_input() + 2;
		std::cout << "Enter amount of columns: ";
		columns = check_input() + 2;
	}
	
	std::ofstream fout("out.txt");

	if (!fout.is_open())
	{
		std::cout << "Could not open \"out.txt\" file for output.\n";
		std::cout << "The initial starting position will not be saved!\n";
	}
	
	fout << rows - 2 << " " << columns - 2 << std::endl;

	int** first_world = create_world(rows, columns);
	
	srand(time(NULL));
	int count = rand() % (rows - 2) * (columns - 2) + 1;

	for (int n = count; n >= 0; --n)
	{
		int i = rand() % (rows - 2);
		int j = rand() % (columns - 2);
		first_world[i + 1][j + 1] = 1;
	}
	
	std::cout << "Cells created: " << count << ".\n";
	
	if (fout.good())
	{
		std::cout << "The initial configuration located in \"out.txt\" file.\n";
	}
	else if (fout.fail())
	{
		std::cout << "Output terminated by data mismatch.\n";
	}
	else
	{
		std::cout << "Output terminated by unknown reason.\n";
	}

	fout.close();
	std::cout << "Games starts in:\n";

	for (int i = 5; i > 0; --i)
	{
		std::cout << i << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return first_world;
}

int check_cell(int** game_field, int i, int j)
{
	int weight = 0;

	if (game_field[i][j] == 0)
	{
		game_field[i][j] < game_field[i][j - 1] ? ++weight : weight;
		game_field[i][j] < game_field[i - 1][j - 1] ? ++weight : weight;
		game_field[i][j] < game_field[i - 1][j] ? ++weight : weight;
		game_field[i][j] < game_field[i - 1][j + 1] ? ++weight : weight;
		game_field[i][j] < game_field[i][j + 1] ? ++weight : weight;
		game_field[i][j] < game_field[i + 1][j + 1] ? ++weight : weight;
		game_field[i][j] < game_field[i + 1][j] ? ++weight : weight;
		game_field[i][j] < game_field[i + 1][j - 1] ? ++weight : weight;

		return weight == 3 ? 1 : 0;
	}

	else
	{
		game_field[i][j] + game_field[i][j - 1] == 2 ? ++weight : weight;
		game_field[i][j] + game_field[i - 1][j - 1] == 2 ? ++weight : weight;
		game_field[i][j] + game_field[i - 1][j] == 2 ? ++weight : weight;
		game_field[i][j] + game_field[i - 1][j + 1] == 2 ? ++weight : weight;
		game_field[i][j] + game_field[i][j + 1] == 2 ? ++weight : weight;
		game_field[i][j] + game_field[i + 1][j + 1] == 2 ? ++weight : weight;
		game_field[i][j] + game_field[i + 1][j] == 2 ? ++weight : weight;
		game_field[i][j] + game_field[i + 1][j - 1] == 2 ? ++weight : weight;

		return weight == 2 || weight == 3 ? 1 : 0;
	}
}

bool check_if_dead(int** first_world, int& alive_cells, const int rows, const int columns)
{
	alive_cells = 0;

	for (int i = 1; i < rows - 1; ++i)
	{
		for (int j = 1; j < columns - 1; ++j)
		{

			if (first_world[i][j] == 1)
			{
				++alive_cells;
			}
		}
	}
	return alive_cells == 0 ? true : false;
}

bool check_if_stagnated(int** first_world, int** second_world, const int rows, const int columns)
{
	for (int i = 1; i < rows - 1; ++i)
	{
		for (int j = 1; j < columns - 1; ++j)
		{

			if (first_world[i][j] != second_world[i][j])
			{
				return false;
			}
		}
	}
	
	return true;
}

void delete_world(int** game_field, const int rows)
{

	for (int i = 0; i < rows; ++i)
	{
		delete[] game_field[i];
	}

	delete[] game_field;
}

int main()
{
	std::cout << "Welcome to John Conway's Game of Life!\n\n";

	char answer = '0';
	char filename[] = "in.txt";

	while (EXIT_CONDITION)
	{
		std::cout << "[1] to set up starting condition manually.\n";
		std::cout << "[2] to read from " << filename << " file.\n";
		std::cout << "[3] to generate random starting configuration.\n";
		std::cout << "[E] to exit: ";

		std::cin >> answer;

		while (EXIT_CONDITION && CHOICE_CONDITION)
		{
			std::cout << "Wrong input! Repeat: ";
			std::cin.ignore(32767, '\n');
			std::cin >> answer;
		}

		int rows = 0;
		int columns = 0;
		int **first_world = nullptr;

		switch (answer)
		{
			case '1': first_world = manual_input(rows, columns);
				break;
			case '2': first_world = auto_input(filename, rows, columns);
				break;
			case '3': first_world = random_input(rows, columns);
				break;
			case 'e':
			case 'E': 
				std::cout << "Bye!\n";
				exit(EXIT_SUCCESS);
		}

		int** second_world = nullptr;
		second_world = create_world(rows, columns);
		int generation = 1;
		int alive_cells = 0;
		bool are_dead = false;
		bool has_stagnated = false;

		while (!are_dead && !has_stagnated)
		{
			std::system("cls");

			for (int i = 1; i < rows - 1; ++i)
			{
				for (int j = 1; j < columns - 1; ++j)
				{
					second_world[i][j] = check_cell(first_world, i, j);
				}
			}

			for (int i = 1; i < rows - 1; ++i)
			{
				for (int j = 1; j < columns - 1; ++j)
				{
					first_world[i][j] == 1 ? std::cout.put(42) : std::cout.put(45); // 42 = '*', 45 = '-'
					std::cout << " ";
				}
				std::cout << std::endl;
			}

			are_dead = check_if_dead(first_world, alive_cells, rows, columns);
			has_stagnated = check_if_stagnated(first_world, second_world, rows, columns);
			
			std::cout << "Generation: " << generation << "; " << "Alive cells: " << alive_cells << ";\n";
			++generation;
			
			for (int i = 1; i < rows - 1; ++i)
			{
				for (int j = 1; j < columns - 1; ++j)
				{
					first_world[i][j] = second_world[i][j];
				}
			}
			
			if (are_dead)
			{
				std::cout << "All cells are dead. Game over.\n";
			}
			else if (has_stagnated)
			{
				std::cout << "The world has stagnated. Game over.\n";
			}

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		delete_world(first_world, rows);
		delete_world(second_world, rows);

		std::cout << "To restart the game press [R] or [E] to exit: ";
		std::cin >> answer;

		while (EXIT_CONDITION && RESTART_CONDITION)
		{
			std::cin.ignore(32767, '\n');
			std::cout << "Wrong input! Repeat: ";
			std::cin >> answer;
		}

		std::system("cls");
	}

	std::cout << "Bye!\n";
	return 0;
}