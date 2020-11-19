/*
This file is part of Nemorino.

Nemorino is free software : you can redistribute it and /or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Nemorino is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Nemorino.If not, see < http://www.gnu.org/licenses/>.
*/

#pragma once
#include <string>
#include <vector>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>
#include "search.h"

class UCIInterface {
public:
	void loop();
  bool dispatch(std::string line);

private:
	std::unique_ptr<NemorinoSearch> Engine = std::make_unique<NemorinoSearch>();
	Position* _position = nullptr;
	int64_t ponderStartTime = 0;
	bool ponderActive = false;
	bool initialized = false;


	std::condition_variable cvStartEngine;
	std::mutex mtxEngineRunning;
	std::thread main_thread;
	std::atomic<bool> engine_active{ false };
	std::atomic<bool> exiting{ false };

	// UCI command handlers
	void uci();
	void isready();
	void setoption(std::vector<std::string> &tokens);
	void ucinewgame();
	void setPosition(std::vector<std::string> &tokens);
	void go(std::vector<std::string> &tokens);
	void perft(std::vector<std::string> &tokens);
	void divide(std::vector<std::string> &tokens);
	void setvalue(std::vector<std::string> &tokens) noexcept;
	void quit();
	void stop() noexcept;
	void thinkAsync();
	void ponderhit();
	void deleteThread();
	void see(std::vector<std::string> &tokens);
	void dumpTT(std::vector<std::string> &tokens);
	void updateFromOptions();
	void copySettings(NemorinoSearch * source, NemorinoSearch * destination);
#ifdef DBG_TT
	void dbg(std::vector<std::string>& tokens);
	void dbg_tt(std::vector<std::string>& tokens);
	void dbg_tts(std::vector<std::string>& tokens);
	void dbg_eval(std::vector<std::string>& tokens);
	void dbg_moves(std::vector<std::string>& tokens);
#endif
	void setTunedParams();
};

