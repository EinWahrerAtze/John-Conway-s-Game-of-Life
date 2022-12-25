//
//  game.cpp
//  John Conway's Game of Life
//
//  Created by Denis Fedorov on 17.12.2022.
//

#include <thread>
#include <random>
#include <fstream>
#include <stdexcept>
#include "game.h"

Game::Game() : _colour(Colour::CYAN),
               _rows(),
               _columns(),
               _cells_alive(),
               _generations(1),
               _output_message("Welcome to John Conway's Game of Life!"),
               _sleeping_time(1000) {}

std::ostream & operator<<(std::ostream & os, const Game::Colour & colour)
{
	return os << "\033[" << static_cast<uint32_t>(colour) << "m";
}

void Game::run()
{
	bool quit {false};
	std::string input;
	while(!quit)
	{
		clear();
		std::cout << _output_message << "\n\n";
		std::cout << "[1] generate random configuration\n";
		std::cout << "[2] read .txt file\n";
		std::cout << "[3] set colour and game speed values\n";
		std::cout << "[X] exit\n\n";
		std::cout << ": ";
		check_input(input);
		clear();
		_output_message.clear();
		switch(input.front())
		{
			case '1':
			{
				_rows = generate_value(5, 30);
				_columns = generate_value(7, 50);
				_worlds.fill(std::vector<uint32_t>(_rows * _columns));
				for (uint32_t y {1}; y < _rows - 1; ++y)
				{
					for (uint32_t x {1}; x < _columns - 1; ++x)
					{
						if (generate_value(0, 6) < 2)
						{
							_worlds.back().at(y * _columns + x) = 1;
							++_cells_alive;
						}
					}
				}
				break;
			}
			case '2':
			{
				std::cout << "Enter filename: ";
				check_input(input);
				try
				{
					read_file(input);
				}
				catch (const std::runtime_error & ex)
				{
					_output_message = ex.what();
					continue;
				}
				break;
			}
			case '3':
			{
				std::cout << "Enter cells colour. Current: ";
				switch (_colour)
				{
					case Colour::RED:
					{
						std::cout << "RED";
						break;
					}
					case Colour::GREEN:
					{
						std::cout << "GREEN";
						break;
					}
					case Colour::YELLOW:
					{
						std::cout << "YELLOW";
						break;
					}
					case Colour::BLUE:
					{
						std::cout << "BLUE";
						break;
					}
					case Colour::MAGENTA:
					{
						std::cout << "MAGENTA";
						break;
					}
					case Colour::CYAN:
					{
						std::cout << "CYAN";
						break;
					}
					case Colour::WHITE:
					{
						std::cout << "WHITE";
						break;
					}
					case Colour::GREY:
					{
						[[fallthrough]];
						
					}
					case Colour::BLACK:
					{
						
					}
					case Colour::WHITE_BG:
					{
						
					}
					case Colour::BLACK_BG:
					{
						
					}
					default:
					{
						
					}
				}
				std::cout << "\n\n";
				std::cout << "RED:     " << static_cast<int>(Colour::RED) << '\n';
				std::cout << "GREEN:   " << static_cast<int>(Colour::GREEN) << '\n';
				std::cout << "YELLOW:  " << static_cast<int>(Colour::YELLOW) << '\n';
				std::cout << "BLUE:    " << static_cast<int>(Colour::BLUE) << '\n';
				std::cout << "MAGENTA: " << static_cast<int>(Colour::MAGENTA) << '\n';
				std::cout << "CYAN:    " << static_cast<int>(Colour::CYAN) << '\n';
				std::cout << "WHITE:   " << static_cast<int>(Colour::WHITE) << "\n\n: ";
				uint32_t value {};
				check_input(value);
				_colour = static_cast<Colour>(value);
				clear();
				std::cout << "Enter game speed. Current: " << _sleeping_time.count() << " ms.\n\n: ";
				check_input(value);
				_sleeping_time = std::chrono::milliseconds(value);
				switch (_colour)
				{
					case Colour::RED:
					{
						_output_message = "Colour set to \"RED\"";
						break;
					}
					case Colour::GREEN:
					{
						_output_message = "Colour set to \"GREEN\"";
						break;
					}
					case Colour::YELLOW:
					{
						_output_message = "Colour set to \"YELLOW\"";
						break;
					}
					case Colour::BLUE:
					{
						_output_message = "Colour set to \"DARK BLUE\"";
						break;
					}
					case Colour::MAGENTA:
					{
						_output_message = "Colour set to \"DARK MAGENTA\"";
						break;
					}
					case Colour::CYAN:
					{
						_output_message = "Colour set to \"DARK CYAN\"";
						break;
					}
					case Colour::WHITE:
					{
						_output_message = "Colour set to \"DARK WHITE\"";
						break;
					}
					case Colour::GREY:
					{
						[[fallthrough]];
					}
					case Colour::BLACK:
					{
						
					}
					case Colour::WHITE_BG:
					{
						
					}
					case Colour::BLACK_BG:
					{
						
					}
					default:
					{
						_output_message = "Unknown colour";
					}
				}
				_output_message += " and game speed to \"" + std::to_string(_sleeping_time.count()) + "\" ms";
				continue;
			}
			case 'x':
			case 'X':
			{
				std::exit(0);
				break;
			}
			default:
			{
				_output_message = "Unknown input";
				continue;
			}
		}
		do
		{
			output();
			std::this_thread::sleep_for(_sleeping_time);
		}
		while(update());
		quit = true;
	}
}

void Game::run(std::string_view filename)
{
	try
	{
		read_file(filename);
	}
	catch (const std::runtime_error & ex)
	{
		std::cout << ex.what();
	}
	do
	{
		output();
		std::this_thread::sleep_for(_sleeping_time);
	}
	while(update());
}

bool Game::update()
{
	for (uint32_t i {}; i < _worlds.size() - 1; ++i)
	{
		_worlds.at(i) = _worlds.at(i + 1);
	}
	uint32_t neigborhoods {};
	for (uint32_t y {1}; y < _rows - 1; ++y)
	{
		for (uint32_t x {1}; x < _columns - 1; ++x)
		{
			neigborhoods = count_neigborhoods(x, y);			
			if (_worlds.back().at(y * _columns + x) == 1)
			{
				if (neigborhoods == 3 || neigborhoods == 2)
				{
					continue;
				}
				else
				{
					_worlds.back().at(y * _columns + x) = 0;
					--_cells_alive;
				}
			}
			else
			{
				if (neigborhoods == 3)
				{
					_worlds.back().at(y * _columns + x) = 1;
					++_cells_alive;
				}
				else
				{
					continue;
				}
			}
		}
	}
	++_generations;
	if (std::find(_worlds.back().cbegin(), _worlds.back().cend(), 1) == _worlds.back().cend())
	{
		std::cout << "All cells are dead. Game over.\n\n";
		return false;
	}
	else if (std::equal(_worlds.at(2).cbegin(), _worlds.at(2).cend(), _worlds.at(1).cbegin(), _worlds.at(1).cend()))
	{
		std::cout << "The world has stagnated. Game over.\n\n";
		return false;
	}
	else if (std::equal(_worlds.at(3).cbegin(), _worlds.at(3).cend(), _worlds.at(1).cbegin(), _worlds.at(1).cend()))
	{
		std::cout << "Your species will live forever! Congratulations!\n\n";
		return false;
	}
	else if (std::equal(_worlds.at(3).cbegin(), _worlds.at(3).cend(), _worlds.at(0).cbegin(), _worlds.at(0).cend()))
	{
		std::cout << "Your species will live forever! Congratulations!\n\n";
		return false;
	}
	else
	{
		return true;
	}
}

[[nodiscard]] uint32_t Game::generate_value(uint32_t min, uint32_t max) const
{
	std::random_device rd {};
	std::mt19937 engine {rd()};
	std::uniform_int_distribution<uint32_t>distribution (min, max);
	return distribution(engine);
}

[[nodiscard]] uint32_t Game::count_neigborhoods(uint32_t row, uint32_t column) const
{
	uint32_t neigborhoods {};
	auto past {_worlds.cend() - 2};
	for (int i {-1}; i < 2; ++i)
	{
		for (int j {-1}; j < 2; ++j)
		{
			if (past->at((column + j) * _columns + (row + i)) == 1)
			{
				if (i == 0 && j == 0)
				{
					continue;
				}
				else
				{
					++neigborhoods;
				}
			}
		}
	}
	return neigborhoods;
}

void Game::read_file(std::string_view filename)
{
	std::ifstream fin {filename.data(), std::ios_base::in};
	if (!fin.is_open())
	{
		throw std::runtime_error {"Could not open \"" + static_cast<std::string>(filename) + "\" for reading"};
	}
	else
	{
		fin >> _rows >> _columns;
		_worlds.fill(std::vector<uint32_t>((_rows + 2) * (_columns + 2)));
		uint32_t x {};
		uint32_t y {};
		while (fin.good())
		{
			fin >> x >> y;
			_worlds.back().at((x + 1) * _columns + y + 1) = 1;
			++_cells_alive;
		}
		if (!fin.eof())
		{
			throw std::runtime_error {"Could not read \"" + static_cast<std::string>(filename) + "\""};
		}
	}
}

void Game::output() const
{
	clear();
	for (uint32_t y {1}; y < _rows - 1; ++y)
	{
		for (uint32_t x {1}; x < _columns - 1; ++x)
		{
			std::cout << (_worlds.back().at(y * _columns + x) == 1 ? _colour : Colour::BLACK) << "▓▓";
		}
		std::cout << '\n';
	}
	std::cout << Colour::DEFAULT;
	std::cout << "Generation: " << _generations << " Cells: " << _cells_alive << '\n';
}

void Game::clear() const
{
#if defined(_WIN32)
	system("cls");
#else
	system("clear");
#endif
}

template <typename T>
void Game::check_input(T & variable)
{
	if constexpr (std::is_same_v<T, std::string>)
	{
		std::getline(std::cin, variable);
		while (variable.empty())
		{
			std::cout << ": ";
			std::getline(std::cin, variable);
		}
	}
	else
	{
		while (!(std::cin >> variable) || variable > std::numeric_limits<T>::max() / 2)
		{
			std::cin.clear();
			while(std::cin.get() != '\n') { continue; }
			std::cout << ": ";
		}
	}
}
