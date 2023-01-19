//
//  life.cpp
//  John Conway's Game of Life
//
//  Created by Denis Fedorov on 14.01.2023.
//

#include "life.h"
#include <random>
#include <fstream>
#include <iostream>

// ANSI escape sequences used in this code:
// unicode prefix: \u001b
// cursor control: ESC[H  - move cursor to home position
//                 ESC[2J - erase entire screen
//                 ESC[0m ... ESC[38m - change colour of output character

// std::formatter specialization for colour class
// must be defined out of game namespace
template <>
struct std::formatter<game::life::colour> : std::formatter<std::string_view>
{
	template <typename T>
	auto format(game::life::colour c, T & t)
	{
		return std::formatter<std::string_view>::format("\u001b[" + std::to_string(static_cast<uint32_t>(c)) + "m", t);
	}
};

// print() function from C++23 to interact with std::format;
// will be replaced with std::print() later when C++23 comes
constexpr void print(const std::string_view string, auto && ... args)
{
	fputs(std::vformat(string, std::make_format_args(args...)).c_str(), stdout);
}

namespace game
{
	life::colour operator ++ (life::colour & c)
	{
		uint32_t n {static_cast<uint32_t>(c)};
		return (n == 37 ? c = life::colour::BLACK : c = static_cast<life::colour>(n + 1));
	}
	
	bool operator == (life::colour lhs, life::colour rhs)
	{
		return static_cast<uint32_t>(lhs) == static_cast<uint32_t>(rhs);
	}
	
	life::cell::cell() : dead(colour::BLACK), alive(colour::CYAN), symbol("██")
	{
		
	}
	
	life::life() : _done(false),
				   _hold(false),
	               _quit(false),
	               _cell(),
	               _layout(layout::RANDOM),
	               _coord(),
	               _alive_cells(),
	               _generations(1),
	               _sleeping_time(500)
	{
		
	}
	
	void life::begin()
	{
		std::string input;
		bool quit {false};
		while (!quit)
		{
			print("\u001b[2J\u001b[H");
			print(std::format("{0}{1}{2}{3}\n: ",
							  "[1] Generate random configuration\n",
							  "[2] Load preset configuration\n",
							  "[3] Read .txt file\n",
							  "[X] Exit\n"));
			std::getline(std::cin, input);
			if (!std::cin)
			{
				std::cin.clear();
			}
			else if (!input.empty())
			{
				switch (input.front())
				{
					case '1':
					{	// random initial pattern is already set
						quit = true;
						break;
					}
					case '2':
					{
						quit = set_layout();
						break;
					}
					case '3':
					{
						print("\u001b[2J\u001b[H");
						print("Enter filename\n");
						input.clear();
						while (input.empty())
						{
							print(": ");
							std::getline(std::cin, input);
							if (!std::cin) { std::cin.clear(); }
						}
						quit = read_file(input);
						break;
					}
					case 'x':
					case 'X':
					{
						end();
					}
					default:
					{
						input.clear();
					}
				}
			}
			else
			{
				print(": ");
			}
		}
		if (!_quit)
		{
			run();
		}
	}
	
	void life::begin(std::string_view filename)
	{
		if (read_file(filename))
		{
			run();
		}
	}
	
	void life::run()
	{
		write_layout();
		read_layout();
		// main game logic (output and math) runs in separate thread,
		// while parent thread deals with user input and controls the flow of the game
		std::thread main_logic {[this]()
			{
				// insert all output data to string before printing to avoid screen flickering on windows
				std::string output_string;
				while (!_quit)
				{
					{
						std::scoped_lock<std::mutex> lk(_mutex);
						print("\u001b[2J\u001b[H");
						for (uint32_t y {}; y < _coord.Y; ++y)
						{
							for (uint32_t x {}; x < _coord.X; ++x)
							{
								if (_worlds.back().at(y * _coord.X + x))
								{
									std::format_to(std::back_inserter(output_string), "{}{}", _cell.alive, _cell.symbol);
								}
								else
								{
									std::format_to(std::back_inserter(output_string), "{}{}", _cell.dead, _cell.symbol);
								}
							}
							std::format_to(std::back_inserter(output_string), "\n");
						}
						print("{}{}", output_string, colour::DEFAULT);
						output_string.clear();
					}
					update();
					if (_hold)
					{
						std::unique_lock<std::mutex> lk(_mutex);
						_interaction.wait(lk, [this]() -> bool
						{
							return !_hold;
						});
					}
				}
				_done = true;
				_interaction.notify_one();
			}};
		main_logic.detach();
		ingame_user_input();
		// wait for main_logic thread to finish
		std::unique_lock<std::mutex> lk(_mutex);
		_interaction.wait(lk, [this]() -> bool
						  {
			return _done;
		});
	}
	
	void life::end()
	{
		_quit = true;
		if (_hold)
		{
			_hold = false;
			_interaction.notify_one();
		}
	}
	
	void life::update()
	{
		{
			std::scoped_lock<std::mutex> lk (_mutex);
			// shift all the worlds left by one, world[2] becomes equal to world[3]
			for (std::size_t i {}; i < _worlds.size() - 1; ++i)
			{
				_worlds.at(i) = _worlds.at(i + 1);
			}
			// lambda function to count all neighbours around given cell at X and Y coordinates
			// world[2] is equal to world[3] at this point, so cell dies or borns later in the world[3]
			// expression ((y + j + _coord.Y) % _coord.Y) * _coord.X + ((x + i + _coord.X) % _coord.X))
			// makes the world toroidal so all the cells have 8 neighbours
			auto count_neigbours {[this](uint32_t x, uint32_t y) -> uint32_t
				{
					uint32_t count {};
					for (int32_t i {-1}; i < 2; ++i)
					{
						for (int32_t j {-1}; j < 2; ++j)
						{
							if (_worlds.at(2).at(((y + j + _coord.Y) % _coord.Y) * _coord.X + ((x + i + _coord.X) % _coord.X)) == 1)
							{
								if (!(i == 0 && j == 0))
								{
									++count;
								}
							}
						}
					}
					return count;
				}};
			// main nested loop to check if cell in the world[3] at given coordinates X and Y alive or not
			// if lambda returns '2' or '3' for a living cell, it remains alive
			// if lambda returns '3' for a dead cell, it becomes alive
			for (uint32_t y {}; y < _coord.Y; ++y)
			{
				for (uint32_t x {}; x < _coord.X; ++x)
				{
					uint32_t neigbours {count_neigbours(x, y)};
					if (_worlds.at(3).at(y * _coord.X + x) == 1)
					{
						_worlds.at(3).at(y * _coord.X + x) = neigbours == 3 || neigbours == 2;
					}
					else
					{
						_worlds.at(3).at(y * _coord.X + x) = neigbours == 3;
					}
				}
			}
			// check for extinction
			if (std::find(_worlds.at(2).cbegin(), _worlds.at(2).cend(), 1) == _worlds.at(2).cend())
			{
				print("All cells are dead. 'X' quit, 'R' restart\n: ");
				_hold = true;
			}
			// check if world[2] is equal to world[1], if so, it is stagnated
			else if (std::equal(_worlds.at(2).cbegin(), _worlds.at(2).cend(), _worlds.at(1).cbegin(), _worlds.at(1).cend()))
			{
				print("The world has stagnated. 'X' quit, 'R' restart\n: ");
				_hold = true;
			}
			// check if there is situation where cells die and born at the same place so endless state appears
			else if (std::equal(_worlds.at(3).cbegin(), _worlds.at(3).cend(), _worlds.at(1).cbegin(), _worlds.at(1).cend()) ||
					 std::equal(_worlds.at(3).cbegin(), _worlds.at(3).cend(), _worlds.at(0).cbegin(), _worlds.at(0).cend()))
			{
				print("The species will live forever! 'X' quit, 'R' restart\n: ");
			}
			// otherwise update current state
			else
			{
				_alive_cells = static_cast<uint32_t>(std::count(_worlds.back().cbegin(), _worlds.back().cend(), 1));
				print(std::format("Generation: {:>3} Cells: {:>3} {:>3} ms\n: ", _generations, _alive_cells, _sleeping_time.count()));
				++_generations;
			}
		}
		std::this_thread::sleep_for(_sleeping_time);
	}
	
	bool life::set_layout()
	{
		print("\u001b[2J\u001b[H");
		print(std::format("{0}{1}{2}{3}{4}{5}",
						  "[1] Random start\n",
						  "[2] Glider gun\n",
						  "[3] Spaceship\n",
						  "[4] Oscillator\n",
						  "[5] 6 bits\n",
						  "[X] Exit\n"));
		if (!_initialization.empty())
		{
			print("[R] Restart current game\n");
		}
		std::string input;
		while (input.empty())
		{
			print("\n: ");
			std::getline(std::cin, input);
			if (std::cin.eof()) { std::cin.clear(); }
		}
		if (input.front() == 'x' || input.front() == 'X')
		{
			return false;
		}
		else if (input.front() == 'r' || input.front() == 'R')
		{
			read_layout();
			return true;
		}
		else
		{	
			try
			{
				_layout = static_cast<life::layout>(std::stoul(input));
				// clear previous pattern written and game progress
				if (!_initialization.empty())
				{
					_initialization.clear();
					for (auto & world : _worlds)
					{
						world.clear();
					}
				}
				write_layout();
				read_layout();
				return true;
			}
			catch (const std::invalid_argument &)
			{
				return false;
			}
			catch (const std::out_of_range &)
			{
				return false;
			}
		}
	}
	
	void life::write_layout()
	{
		switch (_layout)
		{	// user defined patter from file
			case layout::CUSTOM:
			{
				break;
			}
			case layout::GLIDER_GUN:
			{
				_coord.X = 50;
				_coord.Y = 26;
				_initialization = "--------------------------------------------------"
				                  "--------------------------------------------------"
				                  "--------------------------------------------------"
				                  "--------------------------------------------------"
				                  "--------------------------------------------------"
				                  "--------------------------------------------------"
				                  "--------------------------------------------------"
				                  "--------------------------------------------------"
				                  "------------------------------X-------------------"
				                  "----------------------------X-X-------------------"
				                  "------------------XX------XX------------XX--------"
				                  "-----------------X---X----XX------------XX--------"
				                  "------XX--------X-----X---XX----------------------"
				                  "------XX--------X---X-XX----X-X-------------------"
				                  "----------------X-----X-------X-------------------"
				                  "-----------------X---X----------------------------"
				                  "------------------XX------------------------------"
				                  "--------------------------------------------------"
				                  "--------------------------------------------------"
								  "--------------------------------------------------"
				                  "--------------------------------------------------"
				                  "--------------------------------------------------"
				                  "--------------------------------------------------"
				                  "--------------------------------------------------"
				                  "--------------------------------------------------"
				                  "--------------------------------------------------";
				break;
			}
			case layout::SPACESHIP:
			{
				_coord.X = 50;
				_coord.Y = 21;
				_initialization = "--------------------------------------------------"
								  "--------------------------------------X-----------"
								  "---------------------X---------------X-X----------"
								  "-----------X-X------X-----XX--------X-------------"
								  "-----------X----X----X-XXXXXX----XX---------------"
								  "-----------X-XXXXXXXX----------X--X-XXX-----------"
								  "--------------X-----X-------XXXX----XXX-----------"
								  "---------XX-----------------XXX-X-----------------"
								  "------X--XX-------XX--------XX--------------------"
								  "------X--X----------------------------------------"
								  "-----X--------------------------------------------"
								  "------X--X----------------------------------------"
								  "------X--XX-------XX--------XX--------------------"
								  "---------XX-----------------XXX-X-----------------"
								  "--------------X-----X-------XXXX----XXX-----------"
								  "-----------X-XXXXXXXX----------X--X-XXX-----------"
								  "-----------X----X----X-XXXXXX----XX---------------"
								  "-----------X-X------X-----XX--------X-------------"
								  "---------------------X---------------X-X----------"
								  "--------------------------------------X-----------"
								  "--------------------------------------------------";
				break;
			}
			case layout::OSCILLATOR:
			{
				_coord.X = 39;
				_coord.Y = 39;
				_initialization = "---------------------------------------"
				                  "------------XX-----------XX------------"
				                  "------------XX-----------XX------------"
				                  "---------------------------------------"
				                  "---------------------------------------"
				                  "-------X-----------------------X-------"
				                  "------X-X-----X---------X-----X-X------"
				                  "-----X--X-----X-XX---XX-X-----X--X-----"
				                  "------XX----------X-X----------XX------"
				                  "----------------X-X-X-X----------------"
				                  "-----------------X---X-----------------"
				                  "---------------------------------------"
				                  "-XX---------------------------------XX-"
								  "-XX---------------------------------XX-"
								  "------XX-----------------------XX------"
								  "---------------------------------------"
								  "-------X-X-------------------X-X-------"
								  "-------X--X-----------------X--X-------"
								  "--------XX-------------------XX--------"
								  "---------------------------------------"
								  "--------XX-------------------XX--------"
								  "-------X--X-----------------X--X-------"
								  "-------X-X-------------------X-X-------"
								  "---------------------------------------"
								  "------XX-----------------------XX------"
								  "-XX---------------------------------XX-"
								  "-XX---------------------------------XX-"
								  "---------------------------------------"
								  "-----------------X---X-----------------"
								  "----------------X-X-X-X----------------"
								  "------XX----------X-X----------XX------"
								  "-----X--X-----X-XX---XX-X-----X--X-----"
								  "------X-X-----X---------X-----X-X------"
								  "-------X-----------------------X-------"
								  "---------------------------------------"
								  "---------------------------------------"
								  "------------XX-----------XX------------"
								  "------------XX-----------XX------------"
								  "---------------------------------------";
				break;
			}
			case layout::SIX_BITS:
			{
				_coord.X = 50;
				_coord.Y = 28;
				_initialization = "--------------------------------------------------"
								  "--------------------------------------------------"
								  "-------------------------X------------------------"
								  "-------------------------X------------------------"
								  "------------------------X-X-----------------------"
								  "-------------------------X------------------------"
								  "-------------------------X------------------------"
								  "-------------------------X------------------------"
								  "-------------------------X------------------------"
								  "------------------------X-X-----------------------"
								  "-------------------------X------------------------"
								  "-------------------------X------------------------"
								  "--------------------------------------------------"
								  "--------------------------------------------------"
								  "--------------------------------------------------"
								  "--------------------------------------------------"
								  "------X--X----X--X--------------------------------"
								  "----XXX--XXXXXX--XXX------------------------------"
								  "------X--X----X--X--------------------------------"
								  "--------------------------XX----------------------"
								  "-------------------------XX-----------------------"
								  "---------------------------X----------------------"
								  "------------------------------------X----X--------"
								  "----------------------------------XX-XXXX-XX------"
								  "------------------------------------X----X--------"
								  "--------------------------------------------------"
								  "--------------------------------------------------"
								  "--------------------------------------------------";
				break;
			}
			// random pattern
			default:
			{
				_coord.X = random_value(5, 50);
				_coord.Y = random_value(4, 40);
				_initialization.resize(_coord.X * _coord.Y);
				for (uint32_t y {}; y < _coord.Y; ++y)
				{
					for (uint32_t x {}; x < _coord.X; ++x)
					{
						// (0, 6) < 2 gives 30% chance for cell to become alive
						if (random_value(0, 6) < 2)
						{
							_initialization.at(y * _coord.X + x) = 'X';
							++_alive_cells;
						}
					}
				}
				break;
			}
		}
	}
	
	void life::read_layout()
	{
		_worlds.fill(std::vector<uint32_t>(_coord.X * _coord.Y));
		for (uint32_t y {}; y < _coord.Y; ++y)
		{
			for (uint32_t x {}; x < _coord.X; ++x)
			{
				if (_initialization.at(y * _coord.X + x) == 'X')
				{
					_worlds.back().at(y * _coord.X + x) = 1;
				}
			}
		}
	}
	
	void life::ingame_user_input()
	{
		std::string input;
		while (!_quit)
		{
			std::getline(std::cin, input);
			if (std::cin.eof()) { std::cin.clear(); }
			if (!input.empty())
			{
				std::scoped_lock<std::mutex> lk(_mutex);
				switch (input.front())
				{	// restart or exit game
					case 'r':
					case 'R':
					{
						if (set_layout())
						{
							_generations = 1;
							if (_hold)
							{
								_hold = false;
								_interaction.notify_one();
							}
						}
						else
						{
							end();
						}
						break;
					}
					// pause game for a while
					case 'k':
					case 'K':
					{
						if (_hold)
						{
							_hold = false;
							_interaction.notify_one();
						}
						else
						{
							_hold = true;
						}
						break;
					}
					// change output colour of alive cells
					case 'c':
					case 'C':
					{
						++ _cell.alive;
						if (_cell.alive == _cell.dead) { ++_cell.alive; }
						break;
					}
					// change colour of dead cells
					case 'v':
					case 'V':
					{
						++ _cell.dead;
						if (_cell.dead == _cell.alive) { ++_cell.dead; }
						break;
					}
					case 'x':
					case 'X':
					{
						end();
						break;
					}
					default:
					{
						auto is_number {[](const std::string_view string) -> bool
							{
								for (unsigned char c : string)
								{
									if (!std::isdigit(c)) { return false; }
								}
								return true;
							}
						};
						if (is_number(input))
						{
							try
							{
								_sleeping_time = static_cast<std::chrono::milliseconds>(std::stoul(input));
							}
							// catch blocks are empty because there is no reason to notify user if last input was wrong
							// because screen will clear immideately and user won't notice anything
							catch (const std::invalid_argument &)
							{
								
							}
							catch (const std::out_of_range &)
							{
								
							}
						}
					}
				}
			}
		}
	}
	
	bool life::read_file(const std::string_view filename)
	{
		std::ifstream fin {filename.data(), std::ios_base::in};
		if (!fin.is_open())
		{
			print("\u001b[2J\u001b[H");
			print("Could not open \"{}\"\n\n", filename);
			return false;
		}
		else
		{
			fin >> _coord.Y >> _coord.X;
			_initialization.resize(_coord.X * _coord.Y);
			uint32_t x {};
			uint32_t y {};
			try
			{
				while (fin.good())
				{
					fin >> y >> x;
					_initialization.at(y * _coord.X + x) = 'X';
					++_alive_cells;
				}
				if (!fin.eof())
				{
					print("\u001b[2J\u001b[H");
					print("Could not read \"{}\"\n\n", filename);
					return false;
				}
				else
				{
					_layout = layout::CUSTOM;
					return true;
				}
			}
			catch (const std::out_of_range & ex)
			{
				print("\u001b[2J\u001b[H");
				print("Out of range coordinates at \"{}\" and \"{}\"\n\n", x, y);
				return false;
			}
		}
	}
	
	uint32_t life::random_value(uint32_t min, uint32_t max)
	{
		std::random_device rd {};
		std::mt19937 engine {rd()};
		std::uniform_int_distribution<uint32_t>distribution {min, max};
		return distribution(engine);
	}
}
