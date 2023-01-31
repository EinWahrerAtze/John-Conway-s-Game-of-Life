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
	
	life::life() : m_hold(false),
	               m_quit(false),
	               m_cell(),
	               m_layout(layout::RANDOM),
	               m_coord(),
	               m_alive_cells(),
	               m_generations(1),
	               m_sleeping_time(500)
	{
		
	}
	
	life::~life()
	{
		{
			std::lock_guard<std::mutex> lk(m_mutex);
			m_quit = true;
		}
		m_update_thread.join();
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
		if (!m_quit)
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
		// launch game output and update in a separate thread
		m_update_thread = std::thread {[this]()
			{
				// insert all output data to string before printing to avoid screen flickering on windows
				std::string output_string;
				while (true)
				{
					{
						std::lock_guard<std::mutex> lk(m_mutex);
						print("\u001b[2J\u001b[H");
						for (uint32_t y {}; y < m_coord.Y; ++y)
						{
							for (uint32_t x {}; x < m_coord.X; ++x)
							{
								if (m_worlds.back().at(y * m_coord.X + x))
								{
									std::format_to(std::back_inserter(output_string), "{}{}", m_cell.alive, m_cell.symbol);
								}
								else
								{
									std::format_to(std::back_inserter(output_string), "{}{}", m_cell.dead, m_cell.symbol);
								}
							}
							std::format_to(std::back_inserter(output_string), "\n");
						}
						print("{}{}", output_string, colour::DEFAULT);
						output_string.clear();
					}
					update();
					if (m_hold)
					{
						print(": ");
						std::unique_lock<std::mutex> lk(m_mutex);
						m_interaction.wait(lk, [this]() -> bool
						{
							return !m_hold;
						});
					}
					if (m_quit)
					{
						break;
					}
				}
			}};
		ingame_user_input();
	}
	
	void life::end()
	{
		m_quit = true;
		if (m_hold)
		{
			m_hold = false;
			m_interaction.notify_one();
		}
	}
	
	void life::update()
	{
		{
			std::lock_guard<std::mutex> lk (m_mutex);
			// shift all the worlds left by one, world[2] becomes equal to world[3]
			for (std::size_t i {}; i < m_worlds.size() - 1; ++i)
			{
				m_worlds.at(i) = m_worlds.at(i + 1);
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
							if (m_worlds.at(2).at(((y + j + m_coord.Y) % m_coord.Y) * m_coord.X + ((x + i + m_coord.X) % m_coord.X)) == 1)
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
			for (uint32_t y {}; y < m_coord.Y; ++y)
			{
				for (uint32_t x {}; x < m_coord.X; ++x)
				{
					uint32_t neigbours {count_neigbours(x, y)};
					if (m_worlds.at(3).at(y * m_coord.X + x) == 1)
					{
						m_worlds.at(3).at(y * m_coord.X + x) = neigbours == 3 || neigbours == 2;
					}
					else
					{
						m_worlds.at(3).at(y * m_coord.X + x) = neigbours == 3;
					}
				}
			}
			// check for extinction
			if (std::find(m_worlds.at(2).cbegin(), m_worlds.at(2).cend(), 1) == m_worlds.at(2).cend())
			{
				print("All cells are dead. 'X' quit, 'R' restart\n");
				m_hold = true;
			}
			// check if world[2] is equal to world[1], if so, it is stagnated
			else if (std::equal(m_worlds.at(2).cbegin(), m_worlds.at(2).cend(), m_worlds.at(1).cbegin(), m_worlds.at(1).cend()))
			{
				print("The world has stagnated. 'X' quit, 'R' restart\n");
				m_hold = true;
			}
			// check if there is situation where cells die and born at the same place so endless state appears
			else if (std::equal(m_worlds.at(3).cbegin(), m_worlds.at(3).cend(), m_worlds.at(1).cbegin(), m_worlds.at(1).cend()) ||
					 std::equal(m_worlds.at(3).cbegin(), m_worlds.at(3).cend(), m_worlds.at(0).cbegin(), m_worlds.at(0).cend()))
			{
				print("The species will live forever! 'X' quit, 'R' restart\n: ");
			}
			// otherwise update current state
			else
			{
				m_alive_cells = static_cast<uint32_t>(std::count(m_worlds.back().cbegin(), m_worlds.back().cend(), 1));
				print(std::format("Generation: {:>3} Cells: {:>3} {:>3} ms\n: ", m_generations, m_alive_cells, m_sleeping_time.count()));
				++m_generations;
			}
		}
		std::this_thread::sleep_for(m_sleeping_time);
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
		if (!m_initialization.empty())
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
				m_layout = static_cast<life::layout>(std::stoul(input));
				// clear previous pattern written and game progress
				if (!m_initialization.empty())
				{
					m_initialization.clear();
					for (auto & world : m_worlds)
					{
						world.clear();
					}
				}
				write_layout();
				read_layout();
				return true;
			}
			catch (const std::invalid_argument & ex)
			{
				return false;
			}
			catch (const std::out_of_range & ex)
			{
				return false;
			}
		}
	}
	
	void life::write_layout()
	{
		switch (m_layout)
		{	// user defined patter from file
			case layout::CUSTOM:
			{
				break;
			}
			case layout::GLIDER_GUN:
			{
				m_coord.X = 50;
				m_coord.Y = 26;
				m_initialization = "--------------------------------------------------"
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
				m_coord.X = 50;
				m_coord.Y = 21;
				m_initialization = "--------------------------------------------------"
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
				m_coord.X = 39;
				m_coord.Y = 39;
				m_initialization = "---------------------------------------"
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
				m_coord.X = 50;
				m_coord.Y = 28;
				m_initialization = "--------------------------------------------------"
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
				m_coord.X = random_value(5, 50);
				m_coord.Y = random_value(4, 40);
				m_initialization.resize(m_coord.X * m_coord.Y);
				for (uint32_t y {}; y < m_coord.Y; ++y)
				{
					for (uint32_t x {}; x < m_coord.X; ++x)
					{
						// (0, 6) < 2 gives 30% chance for cell to become alive
						if (random_value(0, 6) < 2)
						{
							m_initialization.at(y * m_coord.X + x) = 'X';
							++m_alive_cells;
						}
					}
				}
				break;
			}
		}
	}
	
	void life::read_layout()
	{
		m_worlds.fill(std::vector<uint32_t>(m_coord.X * m_coord.Y));
		for (uint32_t y {}; y < m_coord.Y; ++y)
		{
			for (uint32_t x {}; x < m_coord.X; ++x)
			{
				if (m_initialization.at(y * m_coord.X + x) == 'X')
				{
					m_worlds.back().at(y * m_coord.X + x) = 1;
				}
			}
		}
	}
	
	void life::ingame_user_input()
	{
		std::string input;
		while (!m_quit)
		{
			std::getline(std::cin, input);
			if (std::cin.eof()) { std::cin.clear(); }
			if (!input.empty())
			{
				std::lock_guard<std::mutex> lk(m_mutex);
				switch (input.front())
				{	// restart or exit game
					case 'r':
					case 'R':
					{
						if (set_layout())
						{
							m_generations = 1;
							if (m_hold)
							{
								m_hold = false;
								m_interaction.notify_one();
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
						if (m_hold)
						{
							m_hold = false;
							m_interaction.notify_one();
						}
						else
						{
							m_hold = true;
						}
						break;
					}
					// change output colour of alive cells
					case 'c':
					case 'C':
					{
						++m_cell.alive;
						if (m_cell.alive == m_cell.dead) { ++m_cell.alive; }
						break;
					}
					// change colour of dead cells
					case 'v':
					case 'V':
					{
						++m_cell.dead;
						if (m_cell.dead == m_cell.alive) { ++m_cell.dead; }
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
								m_sleeping_time = static_cast<std::chrono::milliseconds>(std::stoul(input));
							}
							// catch blocks are empty because there is no reason to notify user if last input was wrong
							// because screen will clear immideately and user won't notice anything
							catch (const std::invalid_argument & ex)
							{
								
							}
							catch (const std::out_of_range & ex)
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
			fin >> m_coord.Y >> m_coord.X;
			m_initialization.resize(m_coord.X * m_coord.Y);
			uint32_t x {};
			uint32_t y {};
			try
			{
				while (fin.good())
				{
					fin >> y >> x;
					m_initialization.at(y * m_coord.X + x) = 'X';
					++m_alive_cells;
				}
				if (!fin.eof())
				{
					print("\u001b[2J\u001b[H");
					print("Could not read \"{}\"\n\n", filename);
					return false;
				}
				else
				{
					m_layout = layout::CUSTOM;
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
