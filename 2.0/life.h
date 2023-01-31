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
		~life();
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
			BLACK	= 30,
			RED		= 31,
			GREEN	= 32,
			YELLOW	= 33,
			BLUE	= 34,
			MAGENTA = 35,
			CYAN	= 36,
			WHITE	= 37
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
		bool m_hold;													// flag to hold back execution of child thread
		bool m_quit;													// flag to stop execution the programm
		cell m_cell;
		layout m_layout;												// initial cells pattern
		coordinate m_coord;
		std::mutex m_mutex;
		uint32_t m_alive_cells;
		uint32_t m_generations;
		std::thread m_update_thread;
		std::string m_initialization;
		std::condition_variable m_interaction;							// interaction between threads
		std::chrono::milliseconds m_sleeping_time;
		std::array<std::vector<uint32_t>, 4> m_worlds;
	};
}
