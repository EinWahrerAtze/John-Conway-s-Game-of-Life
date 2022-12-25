//
//  game.h
//  John Conway's Game of Life
//
//  Created by Denis Fedorov on 17.12.2022.
//

#pragma once
#include <array>
#include <vector>
#include <memory>
#include <chrono>
#include <iostream>
#include <cinttypes>
#include <string>
#include <string_view>

class Game
{
public:
	Game();
	void run();
	void run(std::string_view filename);
private:
	enum class Colour
	{
		DEFAULT	 = 0,
		BLACK	 = 30,
		RED	     = 31,
		GREEN	 = 32,
		YELLOW   = 33,
		BLUE	 = 34,
		MAGENTA  = 35,
		CYAN	 = 36,
		WHITE	 = 37,
		GREY	 = 38,
		BLACK_BG = 40,
		WHITE_BG = 107
	};
	friend std::ostream & operator<<(std::ostream & os, const Colour & colour);
private:
	bool update();
	uint32_t generate_value(uint32_t min, uint32_t max) const;
	uint32_t count_neigborhoods(uint32_t column, uint32_t row) const;
	void read_file(std::string_view filename);
	void output() const;
	void clear() const;
	template<typename T>
	void check_input(T & variable);
private:
	Colour _colour;
	uint32_t _rows;
	uint32_t _columns;
	uint32_t _cells_alive;
	uint32_t _generations;
	std::string _output_message;
	std::chrono::milliseconds _sleeping_time;
	std::array<std::vector<uint32_t>, 4> _worlds;
};
