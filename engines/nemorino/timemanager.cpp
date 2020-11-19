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

#include <iostream>
#include <sstream>
#include "timemanager.h"
#include "utils.h"

Timemanager::Timemanager() noexcept {
	_starttime = now();
	_time = INT_MAX;
	_inc = 0;
	_movestogo = 0;
	_mode = UNDEF;
	_maxDepth = MAX_DEPTH - 1;
	_maxNodes = INT64_MAX;
	_hardStopTime = INT64_MAX;
	_stopTime = INT64_MAX;
	std::memset(_iterationTimes, 0, MAX_DEPTH * sizeof(Time_t));
	std::memset(_bestMoves, 0, MAX_DEPTH * sizeof(ValuatedMove));
	std::memset(_nodeCounts, 0, MAX_DEPTH * sizeof(int64_t));
}

Timemanager::Timemanager(const Timemanager &tm) noexcept {
	_starttime = tm._starttime;
	_mode = tm._mode;
	_time = tm._time;
	_inc = tm._inc;
	_movestogo = tm._movestogo;
	_maxDepth = tm._maxDepth;
	_maxNodes = tm._maxNodes;

	_hardStopTime.store(tm._hardStopTime);
	_stopTime.store(tm._stopTime);
	std::memcpy(_iterationTimes, tm._iterationTimes, MAX_DEPTH * sizeof(Time_t));
	std::memcpy(_bestMoves, tm._bestMoves, MAX_DEPTH * sizeof(ValuatedMove));
	std::memcpy(_nodeCounts, tm._nodeCounts, MAX_DEPTH * sizeof(int64_t));
}

Timemanager::~Timemanager() { }

void Timemanager::initialize(int time, int inc, int movestogo) {
	_starttime = now();
	_time = time;
	_inc = inc;
	_movestogo = movestogo;
	_mode = UNDEF;
	_maxDepth = MAX_DEPTH - 1;
	_maxNodes = INT64_MAX;
    _hardStopTime = INT64_MAX;
	_stopTime = INT64_MAX;
	_nodestime = 0;
	init();
}

void Timemanager::initialize(TimeMode mode, int depth, int64_t nodes, int time, int inc, int movestogo, Time_t starttime, bool ponder, int nodestime) {
	_starttime = starttime;
	_time = time;
	_inc = inc;
	_movestogo = movestogo;
	_maxNodes = nodes;
	_mode = UNDEF;
	_maxDepth = std::min(MAX_DEPTH - 1, depth);
	_hardStopTime = INT64_MAX;
	_stopTime = INT64_MAX;
	_nodestime = nodestime;
	_lastCompletedDepth = _completedDepth;
	if (mode == FIXED_TIME_PER_MOVE) {
		if (settings::parameter.EmergencyTime == 0) settings::parameter.EmergencyTime = std::max(100, _time / 1000);
		_mode = mode;
		_hardStopTime = _starttime + _time - settings::parameter.EmergencyTime;
	}
	else if (mode == FIXED_DEPTH) {
		_mode = mode;
		_maxDepth = std::min(MAX_DEPTH - 1, depth);
	}
	else if (mode == NODES) {
		_mode = mode;
		_maxNodes = nodes;
	}
	else if (mode == INFINIT) {
		_mode = mode;
		_hardStopTime = _stopTime = INT64_MAX;
	}
	else init();
	if (ponder) {
		_hardStopTimeSave = _hardStopTime;
		_stopTimeSave = _stopTime;
		_hardStopTime.store(INT64_MAX);
		_stopTime.store(INT64_MAX);
	}
	//utils::debugInfo(print());
}

void Timemanager::init() noexcept {
	if (_time == 0) {	
		if (_maxDepth < MAX_DEPTH - 1)
			_mode = FIXED_DEPTH; else _mode = INFINIT;
	}
	else {
		_failLowDepth = 0;
		int remainingMoves;
		if (_movestogo == 0){
			if (_inc == 0) _mode = SUDDEN_DEATH; else _mode = SUDDEN_DEATH_WITH_INC;
			remainingMoves = 30;
		}
		else {
			if (_inc == 0) _mode = CLASSICAL; else _mode = CLASSICAL_WITH_INC;
			remainingMoves = _movestogo;
		}
		//Leave in any case some emergency time for remaining moves
		const int64_t remainingTime = _time + (_movestogo * _inc);
		if (settings::parameter.EmergencyTime == 0) settings::parameter.EmergencyTime = std::max(100, (int)(remainingTime / 1000));
		//int64_t emergencySpareTime = _movestogo > 1 ? remainingTime / 3 : EmergencyTime;
		//_hardStopTime = std::min(_starttime + _time - EmergencyTime, _starttime + _time - emergencySpareTime);
		//Give at least 10 ms
		_hardStopTime = std::max(_starttime + _time - settings::parameter.EmergencyTime, _starttime + 10);

		if (_movestogo > 1)
		_stopTime = std::min(int64_t(_starttime + remainingTime / remainingMoves), Time_t((_hardStopTime + _starttime)/2));
		else _stopTime.store(_hardStopTime.load());
		//_hardStopTime is now avoiding time forfeits, nevertheless it's still possible that an instable search uses up all time, so that
		//all further moves have to be played a tempo. To avoid this _hardStopTime is reduced so that for further move a reasonable amount of time
		//is left
		if (_movestogo > 1) {
			const int64_t spareTime = (_time - 2 * _inc * (_movestogo - 1)) / 3 ;
			if (spareTime > 0) _hardStopTime -= spareTime;
		}
		if (_nodestime) _maxNodes = (_hardStopTime - _starttime) * _nodestime;
	}
}

void Timemanager::PonderHit() {
	const int64_t tnow = now();
	const int64_t pondertime = tnow - _starttime;
	_hardStopTime.store(std::min(Time_t(_hardStopTimeSave + pondertime - settings::parameter.EmergencyTime), Time_t(_hardStopTime.load())));
	_stopTime.store(_stopTimeSave);
	//check if current iteration will be finished 
	if (_mode != FIXED_TIME_PER_MOVE) {
		const Time_t usedForCompletedIteration = _completionTimeOfLastIteration - _starttime;
		const Time_t usedInLastIteration = tnow - _completionTimeOfLastIteration;
		if (usedInLastIteration < 2 * usedForCompletedIteration) { //If current iteration is not about to be finished, let's check if it's better to return immediately
			const bool stable = _completedDepth > 3 && _bestMoves[_completedDepth - 1].move == _bestMoves[_completedDepth - 2].move && _bestMoves[_completedDepth - 1].move == _bestMoves[_completedDepth - 3].move
				&& std::abs(int16_t(_bestMoves[_completedDepth - 1].score - _bestMoves[_completedDepth - 2].score)) < 0.1;
			//if stable continue iteration if there is 3 times more time available than already spent - if unstable continue next iteration even if only 2 times the spent time is left
			double factor = stable ? 3 : 2;
			//If a fail low has occurred assign even more time
			if (_failLowDepth > 0) factor = factor / 2;
			if (_starttime + Time_t(factor * usedForCompletedIteration) >= _stopTime) {
				_hardStopTime.store(tnow); //stop search by adjusting hardStopTime
			}
		}
	}
	if (_nodestime) _maxNodes = (_hardStopTime - _starttime) * _nodestime;
	//utils::debugInfo(print());
}

// This method is called from search after the completion of each iteration. It returns true when a new iteration shall be started and false if not
bool Timemanager::ContinueSearch(int currentDepth, ValuatedMove bestMove, int64_t nodecount, Time_t tnow) {
	_completedDepth = currentDepth;
	_completionTimeOfLastIteration = tnow;
	_bestMoves[currentDepth - 1] = bestMove;
	_nodeCounts[currentDepth - 1] = nodecount;
	_iterationTimes[currentDepth - 1] = tnow - _starttime;
	//if (ponderMode) return true;
	if (ExitSearch(nodecount, tnow)) return false;
	switch (_mode)
	{
	case INFINIT:
		return true;
	case FIXED_DEPTH:
		return currentDepth < _maxDepth;
	case FIXED_TIME_PER_MOVE:
		if (_nodestime) return nodecount < (_hardStopTime - _starttime) * _nodestime;
		else return _hardStopTime > tnow;
	default:

		const bool stable = currentDepth > 3 && _bestMoves[currentDepth - 1].move == _bestMoves[currentDepth - 2].move && _bestMoves[currentDepth - 1].move == _bestMoves[currentDepth - 3].move
			&& std::abs(int16_t(_bestMoves[currentDepth - 1].score - _bestMoves[currentDepth - 2].score)) < 0.1;
		//if stable only start iteration if there is 3 times more time available than already spent - if unstable start next iteration even if only 2 times the spent time is left
		double factor = stable ? 3 : 2;
		if (_completedDepth > _lastCompletedDepth) factor += _lastCompletedDepth > 0 ? (_completedDepth - _lastCompletedDepth) / 2.0 : 0.5;
		//If a fail low has occurred assign more time
		if (_failLowDepth > 0) factor = factor / 2;
		if (_nodestime > 0) {
			return (nodecount < (_stopTime - _starttime) * _nodestime)
				&& ((factor * (tnow - _starttime)) * _nodestime) >= nodecount;
		} else return tnow < _stopTime && (_starttime + Time_t(factor * (tnow - _starttime))) <= _stopTime;
	}

}

void Timemanager::switchToInfinite() noexcept {
	_mode = INFINIT;
}

int Timemanager::estimatedDepth() noexcept
{
	Time_t stime = _stopTime;
	if (stime == INT64_MAX) stime = _stopTimeSave;
	for (int i = 1; i < MAX_DEPTH; ++i) {
		if (_iterationTimes[i] > stime) return i;
	}
	return 0;
}

void Timemanager::updateTime(int64_t time) {
	const Time_t tnow = now();
	_hardStopTime.store(std::min(int64_t(_hardStopTime.load()), tnow + Time_t(time - settings::parameter.EmergencyTime)));
	//std::cout << "Hardstop time updated: " << _hardStopTime - _starttime;
}

double Timemanager::GetEBF(int depth) const noexcept {
	int d = 0;
	double wbf = 0;
	int64_t n = 0;
	for (d = 2; d < depth; ++d) {
		const double bf = sqrt(1.0 * _nodeCounts[d] / _nodeCounts[d - 2]);
		wbf += bf * _nodeCounts[d - 1];
		n += _nodeCounts[d - 1];
	}
	return wbf / n;
}

std::string Timemanager::print() {
	std::ostringstream s;
	s << "Start: " << _starttime << "  Time: " << _time << "  MTG: " << _movestogo << "  INC: " << _inc;
	s << " Stop:  " << _stopTime - _starttime << "  Hardstop: " << _hardStopTime - _starttime << "  SavedStop:  " << _stopTimeSave - _starttime << "  SavedHardstop: " << _hardStopTimeSave -_starttime;
	return s.str();
}
