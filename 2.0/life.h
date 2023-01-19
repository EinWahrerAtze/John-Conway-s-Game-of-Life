//
//  life.h
//  John Conway's Game of Life
//
//  Created by Denis Fedorov on 14.01.2023.
//

#pragma once
#include <mutex>
#include <array>
#include <vector>
#include <format>
#include <chrono>
#include <thread>
#include <string>
#include <string_view>
#include <condition_variable>

namespace game
{
	class life
	{
	public:
		life();
		void begin();
		void begin(const std::string_view filename);
	private:
		void run();
		void end();
		void update();
		bool set_layout();
		void write_layout();
		void read_layout();
		void ingame_user_input();
		bool read_file(const std::string_view filename);
		uint32_t random_value(uint32_t min, uint32_t max);
	private:
		enum class colour : uint32_t
		{
			DEFAULT	= 0,
			BLACK   = 30,
			RED	    = 31,
			GREEN   = 32,
			YELLOW  = 33,
			BLUE    = 34,
			MAGENTA = 35,
			CYAN    = 36,
			WHITE   = 37,
		};
		enum class layout : uint32_t
		{
			CUSTOM,
			RANDOM,
			GLIDER_GUN,
			SPACESHIP,
			OSCILLATOR,
			SIX_BITS
		};
		struct cell
		{
			cell();
			colour dead;
			colour alive;
			const std::string symbol;
		};
		struct coordinate
		{
			uint32_t X;
			uint32_t Y;
		};		
		friend std::formatter<life::colour>;
		friend colour operator ++ (colour & c);
		friend bool operator == (colour lhs, colour rhs);
	private:
		bool _done;                                                // flag to set when child thread finishes its work
		bool _hold;                                                // flag to hold back execution of child thread
		bool _quit;                                                // flag to stop execution the programm
		cell _cell;
		layout _layout;                                            // initial cells pattern
		coordinate _coord;
		std::mutex _mutex;
		uint32_t _alive_cells;
		uint32_t _generations;
		std::string _initialization;
		std::condition_variable _interaction;                      // interaction between threads
		std::chrono::milliseconds _sleeping_time;
		std::array<std::vector<uint32_t>, 4> _worlds;
	};
}
