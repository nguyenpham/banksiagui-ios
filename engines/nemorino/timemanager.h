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

#include <vector>
#include <atomic>
#include "types.h"
#include "settings.h"
#include "position.h"

/* The timemanager class manages the time assignment for the engine. To do this it needs the current time information, as well as the
   mode (sudden death, classical, ...). Further it keeps track of branching factor to predict, whether a further iteration can be completed
   within the assigned time */

//Mode of the current game
enum TimeMode  { UNDEF,                   //Not yet defined
	             SUDDEN_DEATH,            //Time is available time for whole game (e.g. 5 min blitz)
	             SUDDEN_DEATH_WITH_INC,   //Time is available time for whole game, but with increment (e.g. 3 min + 2sec increment per move)
	             CLASSICAL,               //Time is available time for a number of moves (e.g. 40 moves / 2h)
	             CLASSICAL_WITH_INC,      //Time is available time for a number of moves but increment is added for each move (e.g. 40 moves / 90 min + 30 sec increment per move)
	             FIXED_TIME_PER_MOVE,     //Time is available for each move, saving time isn't possible
	             INFINIT,                 //Think until stopped (analysis mode)
	             FIXED_DEPTH,             //Always think until the given depth is reached (time is not relevant)
	             NODES                    //Nodetime, convert nodes into time and measure based on node count (not yet implemented)
               };

	class Timemanager
	{
	public:
		Timemanager() noexcept;
		Timemanager(const Timemanager &tm) noexcept;
		~Timemanager();
		Timemanager(Timemanager&&) = default;
		Timemanager& operator=(Timemanager&&) = default;
		/* Timemanager initialization. Mode is determined based on the values given (movestogo = 0 => Sudden death, ..)
		Initialization has to be done before every move
		*/
		void initialize(int time = 0, int inc = 0, int movestogo = 0);
		/* Timemanager initialization with full information
		Initialization is done before every move
		*/
		void initialize(TimeMode mode, int depth = MAX_DEPTH, int64_t nodes = INT64_MAX, int time = 0, int inc = 0, int movestogo = 0, Time_t starttime = now(), bool ponder = false, int nodestime = 0);
		//Checks whether Search has to be exited even within an iteration
		inline bool ExitSearch(int64_t nodes = 0, Time_t tnow = now()) const { return tnow >= _hardStopTime || nodes >= _maxNodes; }
		//Checks whether a new iteration at next higher depth shall be started
		bool ContinueSearch(int currentDepth, ValuatedMove bestMove, int64_t nodecount, Time_t tnow = now());
		//returns the effective branching factor, which is based on the node counts needed for the different depths
		double GetEBF(int depth = MAX_DEPTH) const noexcept;
		//Informs the timemanager that a ponderhit has occured. The timemanager will then adjust the assigned time for the move
		void PonderHit();
		//returns the time when the current search started
		inline Time_t GetStartTime() const noexcept { return _starttime; }
		//Returns the depth at which search will be stopped
		inline int GetMaxDepth() const noexcept { return _maxDepth; }
		//Informs the timemanager that a fail low at root has happened - timemanager will assign more time
		inline void reportFailLow() noexcept { _failLowDepth = _completedDepth + 1; }
		//Utility method: returns a string, with the current time values to store it in a log
		std::string print();
		//Updating available time when pondering (only relevant for xboard mode when pondering, where the GUI sends a time update when the opponent moves)
		void updateTime(Time_t time);
		//In xboard pprotocol it's possible, that engine can be switched to analysis mode while thinking. When this method is called timemanager switches to 
		//mode=INFINIT
		void switchToInfinite() noexcept;
		//calculates the depth, which will probably be reached with current time
		int estimatedDepth() noexcept;

		inline TimeMode Mode() noexcept { return _mode; }

	private:
		TimeMode _mode = UNDEF;
		int _time;
		int _inc;
		int _movestogo;
		Time_t _starttime = 0;
		int _maxDepth = MAX_DEPTH - 1;
		int64_t _maxNodes = INT64_MAX;
		int _completedDepth = 0;
		Time_t _completionTimeOfLastIteration = 0;
		int _nodestime = 0;
		int _lastCompletedDepth = 0;

		//The time when search has to be aborted to avoid time loss
		std::atomic<long long> _hardStopTime;
		//The time where the assigned time for the move is expired
		std::atomic<long long> _stopTime;

		Time_t _hardStopTimeSave = INT64_MAX;
		Time_t _stopTimeSave = INT64_MAX;
		int _failLowDepth = 0;

		/*To decide whether or not a new iteration shall be started, the timemanager tries stores the best moves from already finished iterations to check if
		  the search is stable or not. Further it collects the times and nodecounts allowing to predict the time needed for next iteration (currently these 
		  information isn't used). 
		*/
		Time_t _iterationTimes[MAX_DEPTH];
		ValuatedMove _bestMoves[MAX_DEPTH];
		int64_t _nodeCounts[MAX_DEPTH];

		void init() noexcept;
	};

