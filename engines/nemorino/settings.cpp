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
#include <fstream>
#include <algorithm>
#include <regex>
#include <thread>
#include "settings.h"
#include "utils.h"
#include "hashtables.h"
#include "material.h"

namespace settings {

	Parameters parameter;
	Options options;

	Parameters::Parameters() noexcept
	{
		Initialize();
	}

	void Parameters::Initialize() {
		for (int depth = 1; depth < 64; depth++) {
			for (int moves = 1; moves < 64; moves++) {
				const double reduction = std::log(moves) * std::log(depth) / 2; //F
				if (reduction < 0.8) LMR_REDUCTION[depth][moves] = 0;
				else LMR_REDUCTION[depth][moves] = int(std::round(reduction));
				assert(LMR_REDUCTION[depth][moves] < depth);
			}
		}
		//Make PSQT symmetric
		for (int i = 1; i < 12; i+=2) {
			for (int s = 0; s < 64; ++s) {
				PSQT[i][s ^ 56] = -PSQT[i - 1][s];
			}
		}
		LMR_REDUCTION[0][0] = 0;
		LMR_REDUCTION[1][0] = 0;
		LMR_REDUCTION[0][1] = 0;
	}

	int Parameters::LMRReduction(int depth, int moveNumber) noexcept
	{
		return LMR_REDUCTION[std::min(depth, 63)][std::min(moveNumber, 63)];
	}

	void Parameters::UCIExpose()
	{
		sync_cout << "option name USE_TT_IN_QSEARCH type check default " << USE_TT_IN_QSEARCH << sync_endl;
		sync_cout << "option name BETA_PRUNING_FACTOR type spin default " << BETA_PRUNING_FACTOR << " min 0 max 200" << sync_endl;
		sync_cout << "option name PAWN_SHELTER_2ND_RANK_MG type spin default " << PAWN_SHELTER_2ND_RANK.mgScore << " min 0 max 100" << sync_endl;
		sync_cout << "option name PAWN_SHELTER_2ND_RANK_EG type spin default " << PAWN_SHELTER_2ND_RANK.egScore << " min -100 max 0" << sync_endl;
		sync_cout << "option name PAWN_SHELTER_3RD_RANK_MG type spin default " << PAWN_SHELTER_3RD_RANK.mgScore << " min 0 max 100" << sync_endl;
		sync_cout << "option name PAWN_SHELTER_3RD_RANK_EG type spin default " << PAWN_SHELTER_3RD_RANK.egScore << " min -100 max 100" << sync_endl;
		sync_cout << "option name PAWN_SHELTER_4TH_RANK_MG type spin default " << PAWN_SHELTER_4TH_RANK.mgScore << " min 0 max 100" << sync_endl;
		sync_cout << "option name PAWN_SHELTER_4TH_RANK_EG type spin default " << PAWN_SHELTER_4TH_RANK.egScore << " min -80 max 10" << sync_endl;
		for (int i = 0; i < 4; ++i) sync_cout << "option name SAFE_CHECK_" << i << " type spin default " << SAFE_CHECK[i] << " min 0 max 2000" << sync_endl;
		for (int i = 0; i < 4; ++i) sync_cout << "option name ATTACK_WEIGHT_" << i << " type spin default " << ATTACK_WEIGHT[i] << " min 0 max " << 3 * ATTACK_WEIGHT[i] << sync_endl;
		for (int i = 0; i < 6; ++i) sync_cout << "option name MALUS_BLOCKED_" << i << " type spin default " << MALUS_BLOCKED[i].mgScore << " min 0 max " << std::max(20, static_cast<int>(3 * MALUS_BLOCKED[i].mgScore)) << sync_endl;
		for (int i = 0; i < 6; ++i) sync_cout << "option name PASSED_PAWN_BONUS_" << i << " type spin default " << PASSED_PAWN_BONUS[i].mgScore << " min 0 max " << std::max(20, static_cast<int>(3 * PASSED_PAWN_BONUS[i].mgScore)) << sync_endl;
		for (int i = 0; i < 6; ++i) sync_cout << "option name BONUS_PROTECTED_PASSED_PAWN_" << i << " type spin default " << BONUS_PROTECTED_PASSED_PAWN[i].mgScore << " min 0 max " << std::max(20, static_cast<int>(3 * BONUS_PROTECTED_PASSED_PAWN[i].mgScore)) << sync_endl;
		sync_cout << "option name KING_RING_ATTACK_FACTOR type spin default " << KING_RING_ATTACK_FACTOR << " min 0 max 300" << sync_endl;
		sync_cout << "option name WEAK_SQUARES_FACTOR type spin default " << WEAK_SQUARES_FACTOR << " min 0 max 600" << sync_endl;
		sync_cout << "option name PINNED_FACTOR type spin default " << PINNED_FACTOR << " min 0 max 400" << sync_endl;
		sync_cout << "option name ATTACK_WITH_QUEEN type spin default " << ATTACK_WITH_QUEEN << " min 0 max 2500" << sync_endl;
		for (PieceType pt = PieceType::QUEEN; pt <= PieceType::PAWN; ++pt) {
			sync_cout << "option name PIECEVAL_MG_" << static_cast<int>(pt) << " type spin default " << static_cast<int>(PieceValues[pt].mgScore) << " min 0 max " << 2 * static_cast<int>(PieceValues[pt].mgScore) << sync_endl;
			sync_cout << "option name PIECEVAL_EG_" << static_cast<int>(pt) << " type spin default " << static_cast<int>(PieceValues[pt].egScore) << " min 0 max " << 2 * static_cast<int>(PieceValues[pt].egScore) << sync_endl;
		}
		sync_cout << "option name HANGING_MG type spin default " << static_cast<int>(HANGING.mgScore) << " min 0 max 50" << sync_endl;
		sync_cout << "option name HANGING_EG type spin default " << static_cast<int>(HANGING.egScore) << " min 0 max 50" << sync_endl;
		const int mobilitySize[4] = { 28, 15, 14, 9 };
		for (int mcount = 0; mcount < mobilitySize[static_cast<int>(QUEEN)]; ++mcount) {
			sync_cout << "option name MOBILITY_Q_MG_" << mcount << " type spin default " << static_cast<int>(parameter.MOBILITY_BONUS_QUEEN[mcount].mgScore) << " min -50 max 50" << sync_endl;
			sync_cout << "option name MOBILITY_Q_EG_" << mcount << " type spin default " << static_cast<int>(parameter.MOBILITY_BONUS_QUEEN[mcount].egScore) << " min -50 max 50" << sync_endl;
		}
		for (int mcount = 0; mcount < mobilitySize[static_cast<int>(ROOK)]; ++mcount) {
			sync_cout << "option name MOBILITY_R_MG_" << mcount << " type spin default " << static_cast<int>(parameter.MOBILITY_BONUS_ROOK[mcount].mgScore) << " min -100 max 100" << sync_endl;
			sync_cout << "option name MOBILITY_R_EG_" << mcount << " type spin default " << static_cast<int>(parameter.MOBILITY_BONUS_ROOK[mcount].egScore) << " min -100 max 100" << sync_endl;
		}
		for (int mcount = 0; mcount < mobilitySize[static_cast<int>(BISHOP)]; ++mcount) {
			sync_cout << "option name MOBILITY_B_MG_" << mcount << " type spin default " << static_cast<int>(parameter.MOBILITY_BONUS_BISHOP[mcount].mgScore) << " min -100 max 100" << sync_endl;
			sync_cout << "option name MOBILITY_B_EG_" << mcount << " type spin default " << static_cast<int>(parameter.MOBILITY_BONUS_BISHOP[mcount].egScore) << " min -100 max 100" << sync_endl;
		}
		for (int mcount = 0; mcount < mobilitySize[static_cast<int>(KNIGHT)]; ++mcount) {
			sync_cout << "option name MOBILITY_N_MG_" << mcount << " type spin default " << static_cast<int>(parameter.MOBILITY_BONUS_KNIGHT[mcount].mgScore) << " min -50 max 50" << sync_endl;
			sync_cout << "option name MOBILITY_N_EG_" << mcount << " type spin default " << static_cast<int>(parameter.MOBILITY_BONUS_KNIGHT[mcount].egScore) << " min -50 max 50" << sync_endl;
		}
		sync_cout << "option name LEVER_ON_KING type spin default " << BONUS_LEVER_ON_KINGSIDE << " min 0 max 30" << sync_endl;
		sync_cout << "option name KING_ON_ONE_MG type spin default " << KING_ON_ONE.mgScore << " min 0 max 50" << sync_endl;
		sync_cout << "option name KING_ON_ONE_EG type spin default " << KING_ON_ONE.egScore << " min 0 max 100" << sync_endl;
		sync_cout << "option name KING_ON_MANY_MG type spin default " << KING_ON_MANY.mgScore << " min 0 max 50" << sync_endl;
		sync_cout << "option name KING_ON_MANY_EG type spin default " << KING_ON_MANY.egScore << " min 0 max 150" << sync_endl;
		sync_cout << "option name BONUS_BISHOP_PAIR type spin default " << BONUS_BISHOP_PAIR.egScore << " min 0 max 150" << sync_endl;
		sync_cout << "option name MALUS_BACKWARD_PAWN_MG type spin default " << MALUS_BACKWARD_PAWN.mgScore << " min 0 max 60" << sync_endl;
		sync_cout << "option name MALUS_BACKWARD_PAWN_EG type spin default " << MALUS_BACKWARD_PAWN.egScore << " min 0 max 60" << sync_endl;
		sync_cout << "option name MALUS_WEAK_PAWN_MG type spin default " << MALUS_WEAK_PAWN.mgScore << " min 0 max 60" << sync_endl;
		sync_cout << "option name MALUS_WEAK_PAWN_EG type spin default " << MALUS_WEAK_PAWN.egScore << " min 0 max 60" << sync_endl;
		sync_cout << "option name PAWN_REDUCTION_LIMIT type spin default " << PAWN_REDUCTION_LIMIT << " min 0 max 100" << sync_endl;
		sync_cout << "option name ROOK_ON_OPENFILE_MG type spin default " << ROOK_ON_OPENFILE.mgScore << " min 0 max 100" << sync_endl;
		sync_cout << "option name ROOK_ON_OPENFILE_EG type spin default " << ROOK_ON_OPENFILE.egScore << " min 0 max 100" << sync_endl;
		sync_cout << "option name ROOK_ON_SEMIOPENFILE_MG type spin default " << ROOK_ON_SEMIOPENFILE.mgScore << " min 0 max 100" << sync_endl;
		sync_cout << "option name ROOK_ON_SEMIOPENFILE_EG type spin default " << ROOK_ON_SEMIOPENFILE.egScore << " min 0 max 100" << sync_endl;
		sync_cout << "option name ROOK_ON_7TH_MG type spin default " << ROOK_ON_7TH.mgScore << " min 0 max 100" << sync_endl;
		sync_cout << "option name ROOK_ON_7TH_EG type spin default " << ROOK_ON_7TH.egScore << " min 0 max 100" << sync_endl;
		sync_cout << "option name MALUS_KNIGHT_DISLOCATED type spin default " << MALUS_KNIGHT_DISLOCATED << " min 0 max 100" << sync_endl;
		const std::string PIECE_NAMES("QRBNPK");
		for (int p = 0; p < 6; ++p) {
			const int from = p == 4 ? 8 : 0;
			const int to = p == 4 ? 56 : 64;
			for (int i = from;i < to; ++i) {
				sync_cout << "option name PSQT_" << PIECE_NAMES[p] << "_MG_" << i << " type spin default " << static_cast<int>(parameter.PSQT[2*p][i].mgScore) << " min -50 max 50" << sync_endl;
				sync_cout << "option name PSQT_" << PIECE_NAMES[p] << "_EG_" << i << " type spin default " << static_cast<int>(parameter.PSQT[2*p][i].egScore) << " min -50 max 50" << sync_endl;
			}
		}
	}

	void Parameters::SetFromUCI(std::string name, std::string value)
	{
		if (!name.compare("USE_TT_IN_QSEARCH")) USE_TT_IN_QSEARCH = !value.compare("true");
		else if (!name.compare("KING_DANGER_SCALE")) KING_DANGER_SCALE = stoi(value);
		else if (!name.compare("PAWN_SHELTER_2ND_RANK_EG")) PAWN_SHELTER_2ND_RANK.egScore = static_cast<Value>(stoi(value));
		else if (!name.compare("PAWN_SHELTER_2ND_RANK_MG")) PAWN_SHELTER_2ND_RANK.mgScore = static_cast<Value>(stoi(value));
		else if (!name.compare("PAWN_SHELTER_3RD_RANK_EG")) PAWN_SHELTER_3RD_RANK.egScore = static_cast<Value>(stoi(value));
		else if (!name.compare("PAWN_SHELTER_3RD_RANK_MG")) PAWN_SHELTER_3RD_RANK.mgScore = static_cast<Value>(stoi(value));
		else if (!name.compare("PAWN_SHELTER_4TH_RANK_EG")) PAWN_SHELTER_4TH_RANK.egScore = static_cast<Value>(stoi(value));
		else if (!name.compare("PAWN_SHELTER_4TH_RANK_MG")) PAWN_SHELTER_4TH_RANK.mgScore = static_cast<Value>(stoi(value));
		else if (!name.compare("BETA_PRUNING_FACTOR")) BETA_PRUNING_FACTOR = static_cast<Value>(stoi(value));
		else if (!name.compare("KING_RING_ATTACK_FACTOR")) KING_RING_ATTACK_FACTOR = stoi(value);
		else if (!name.compare("WEAK_SQUARES_FACTOR")) WEAK_SQUARES_FACTOR = stoi(value);
		else if (!name.compare("PINNED_FACTOR")) PINNED_FACTOR = stoi(value);
		else if (!name.compare("ATTACK_WITH_QUEEN")) ATTACK_WITH_QUEEN = stoi(value);
		else if (!name.compare("LEVER_ON_KING")) BONUS_LEVER_ON_KINGSIDE = stoi(value);
		else if (!name.compare("MALUS_BACKWARD_PAWN_MG")) MALUS_BACKWARD_PAWN.mgScore = static_cast<Value>(stoi(value));
		else if (!name.compare("MALUS_BACKWARD_PAWN_EG")) MALUS_BACKWARD_PAWN.egScore = static_cast<Value>(stoi(value));
		else if (!name.compare("MALUS_WEAK_PAWN_MG")) MALUS_WEAK_PAWN.mgScore = static_cast<Value>(stoi(value));
		else if (!name.compare("MALUS_WEAK_PAWN_EG")) MALUS_WEAK_PAWN.egScore = static_cast<Value>(stoi(value));
		else if (name.find("SAFE_CHECK_") == 0) {
			int index = stoi(name.substr(11, std::string::npos));
			SAFE_CHECK[index] = stoi(value);
		}
		else if (name.find("ATTACK_WEIGHT_") == 0) {
			int index = stoi(name.substr(14, std::string::npos));
			ATTACK_WEIGHT[index] = stoi(value);
		}
		else if (name.find("MALUS_BLOCKED_") == 0) {
			int index = stoi(name.substr(14, std::string::npos));
			MALUS_BLOCKED[index] = Eval(stoi(value));
		}
		else if (name.find("PASSED_PAWN_BONUS_") == 0) {
			int index = stoi(name.substr(18, std::string::npos));
			PASSED_PAWN_BONUS[index] = Eval(stoi(value));
		}
		else if (name.find("BONUS_PROTECTED_PASSED_PAWN_") == 0) {
			int index = stoi(name.substr(28, std::string::npos));
			BONUS_PROTECTED_PASSED_PAWN[index] = Eval(stoi(value));
		}
		else if (name.find("PIECEVAL_MG_") == 0) {
			int index = stoi(name.substr(12, std::string::npos));
			PieceValues[index].mgScore = static_cast<Value>(stoi(value));
			InitializeMaterialTable();
		}
		else if (name.find("PIECEVAL_EG_") == 0) {
			int index = stoi(name.substr(12, std::string::npos));
			PieceValues[index].egScore = static_cast<Value>(stoi(value));
			InitializeMaterialTable();
		}
		else if (!name.compare("BONUS_BISHOP_PAIR")) {
			BONUS_BISHOP_PAIR = Eval(static_cast<Value>(stoi(value)));
			InitializeMaterialTable();
		}
		else if (!name.compare("HANGING_EG")) HANGING.egScore = static_cast<Value>(stoi(value));
		else if (!name.compare("HANGING_MG")) HANGING.mgScore = static_cast<Value>(stoi(value));
		else if (!name.compare("KING_ON_ONE_EG")) KING_ON_ONE.egScore = static_cast<Value>(stoi(value));
		else if (!name.compare("KING_ON_ONE_MG")) KING_ON_ONE.mgScore = static_cast<Value>(stoi(value));
		else if (!name.compare("KING_ON_MANY_EG")) KING_ON_MANY.egScore = static_cast<Value>(stoi(value));
		else if (!name.compare("KING_ON_MANY_MG")) KING_ON_MANY.mgScore = static_cast<Value>(stoi(value));
		else if (name.find("MOBILITY_") == 0) {
			const char pt = name[9];
			int index = stoi(name.substr(14, std::string::npos));
			const bool isMg = name[11] == 'M';
			switch (pt)
			{
			case 'Q':
				if (isMg) parameter.MOBILITY_BONUS_QUEEN[index].mgScore = static_cast<Value>(stoi(value)); else parameter.MOBILITY_BONUS_QUEEN[index].egScore = static_cast<Value>(stoi(value));
				break;
			case 'R':
				if (isMg) parameter.MOBILITY_BONUS_ROOK[index].mgScore = static_cast<Value>(stoi(value)); else parameter.MOBILITY_BONUS_ROOK[index].egScore = static_cast<Value>(stoi(value));
				break;
			case 'B':
				if (isMg) parameter.MOBILITY_BONUS_BISHOP[index].mgScore = static_cast<Value>(stoi(value)); else parameter.MOBILITY_BONUS_BISHOP[index].egScore = static_cast<Value>(stoi(value));
				break;
			case 'N':
				if (isMg) parameter.MOBILITY_BONUS_KNIGHT[index].mgScore = static_cast<Value>(stoi(value)); else parameter.MOBILITY_BONUS_KNIGHT[index].egScore = static_cast<Value>(stoi(value));
				break;
			default:
				break;
			}
		}
		else if (!name.compare("PAWN_REDUCTION_LIMIT")) PAWN_REDUCTION_LIMIT = static_cast<Value>(stoi(value));
		else if (!name.compare("ROOK_ON_OPENFILE_MG")) ROOK_ON_OPENFILE.mgScore = static_cast<Value>(stoi(value));
		else if (!name.compare("ROOK_ON_OPENFILE_EG")) ROOK_ON_OPENFILE.egScore = static_cast<Value>(stoi(value));
		else if (!name.compare("ROOK_ON_SEMIOPENFILE_MG")) ROOK_ON_SEMIOPENFILE.mgScore = static_cast<Value>(stoi(value));
		else if (!name.compare("ROOK_ON_SEMIOPENFILE_EG")) ROOK_ON_SEMIOPENFILE.egScore = static_cast<Value>(stoi(value));
		else if (!name.compare("ROOK_ON_7TH_MG")) ROOK_ON_7TH.mgScore = static_cast<Value>(stoi(value));
		else if (!name.compare("ROOK_ON_7TH_EG")) ROOK_ON_7TH.egScore = static_cast<Value>(stoi(value));
		else if (!name.compare("MALUS_KNIGHT_DISLOCATED")) MALUS_KNIGHT_DISLOCATED = static_cast<Value>(stoi(value));
		else if (name.find("PSQT_") == 0) {
			const char pt = name[5];
			const bool isMg = name[7] == 'M';
			int index = stoi(name.substr(10, std::string::npos));
			const Value val = static_cast<Value>(stoi(value));
			const std::string PIECE_CHARS = "QRBNPK";
			size_t pieceIndex = PIECE_CHARS.find(pt);
				if (isMg) {
					parameter.PSQT[2 *pieceIndex][index].mgScore = val;
					parameter.PSQT[2 * pieceIndex + 1][index ^ 56].mgScore = -val;
				}
				else {
					parameter.PSQT[2 * pieceIndex][index].egScore = val;
					parameter.PSQT[2 * pieceIndex + 1][index ^ 56].egScore = -val;
				}
		}
	}

	Option::Option(std::string Name, OptionType Type, std::string DefaultValue, std::string MinValue, std::string MaxValue, bool Technical)
	{
		name = Name;
		otype = Type;
		defaultValue = DefaultValue;
		maxValue = MaxValue;
		minValue = MinValue;
		technical = Technical;
	}

	std::string Option::printUCI()
	{
		std::stringstream ss;
		ss << "option name " << name << " type ";
		switch (otype)
		{
		case OptionType::BUTTON:
			ss << "button";
			return ss.str();
		case OptionType::CHECK:
			ss << "check";
			if (defaultValue.size() > 0) ss << " default " << defaultValue;
			return ss.str();
		case OptionType::STRING:
			ss << "string";
			if (defaultValue.size() > 0) ss << " default " << defaultValue;
			return ss.str();
		case OptionType::SPIN:
			ss << "spin";
			if (defaultValue.size() > 0) ss << " default " << defaultValue;
			if (minValue.size() > 0) ss << " min " << minValue;
			if (maxValue.size() > 0) ss << " max " << maxValue;
			return ss.str();
		default:
			break;
		}
		return ss.str();
	}

	std::string Option::printInfo() const
	{
		std::stringstream ss;
		ss << "info string " << name << " ";
		switch (otype)
		{
		case OptionType::CHECK:
			ss << ((OptionCheck *)this)->getValue();
			return ss.str();
		case OptionType::STRING:
			ss << ((OptionString *)this)->getValue();
			return ss.str();
		case OptionType::SPIN:
			ss << ((OptionSpin *)this)->getValue();
			return ss.str();
		default:
			break;
		}
		return ss.str();
	}

	void OptionThread::set(std::string value)
	{
		_value = stoi(value);
		parameter.HelperThreads = _value - 1;
	}

	void Option960::set(std::string value) noexcept
	{
		Chess960 = !value.compare("true");
	}

	Options::Options() noexcept
	{
		initialize();
	}

	Options::~Options()
	{
		for (std::map<std::string, Option *>::iterator itr = this->begin(); itr != this->end(); itr++)
		{
			delete itr->second;
		}
	}




	void Options::printUCI()
	{
		for (auto it = begin(); it != end(); ++it) {
			if (!it->second->isTechnical())
				sync_cout << it->second->printUCI() << sync_endl;
		}
	}

	void Options::printInfo()
	{
		for (auto it = begin(); it != end(); ++it) {
			sync_cout << it->second->printInfo() << sync_endl;
		}
	}

	void Options::read(std::vector<std::string> &tokens)
	{
		if (find(tokens[2]) == end()) return;
		at(tokens[2])->read(tokens);
	}

	int Options::getInt(std::string key)
	{
		return ((OptionSpin *)at(key))->getValue();
	}

	bool Options::getBool(std::string key)
	{
		return ((OptionCheck *)at(key))->getValue();
	}

	std::string Options::getString(std::string key)
	{
		return find(key) == end() ? "" : ((OptionString *)at(key))->getValue();
	}

#pragma warning(push)
#pragma warning(disable: 26409)

	void Options::set(std::string key, std::string value)
	{
		if (find(key) == end()) {
			(*this)[key] = (Option *)(new OptionString(key));
		}
		((OptionString *)at(key))->set(value);
	}

	void Options::set(std::string key, int value)
	{
		((OptionSpin *)at(key))->set(value);
	}

	void Options::set(std::string key, bool value)
	{
		((OptionCheck *)at(key))->set(value);
	}

	void Options::initialize()
	{
		(*this)[OPTION_CHESS960] = (Option *)(new Option960());
		(*this)[OPTION_HASH] = (Option *)(new OptionHash());
		(*this)[OPTION_CLEAR_HASH] = (Option *)(new OptionButton(OPTION_CLEAR_HASH));
		(*this)[OPTION_PRINT_OPTIONS] = (Option *)(new OptionButton(OPTION_PRINT_OPTIONS));
		(*this)[OPTION_MULTIPV] = (Option *)(new OptionSpin(OPTION_MULTIPV, 1, 1, 216));
		(*this)[OPTION_THREADS] = (Option *)(new OptionThread());
		(*this)[OPTION_PONDER] = (Option *)(new OptionCheck(OPTION_PONDER, false));
		(*this)[OPTION_CONTEMPT] = (Option *)(new OptionContempt());
		(*this)[OPTION_BOOK_FILE] = (Option *)(new OptionString(OPTION_BOOK_FILE, "book.bin"));
		(*this)[OPTION_OWN_BOOK] = (Option *)(new OptionCheck(OPTION_OWN_BOOK, false));
		(*this)[OPTION_OPPONENT] = (Option *)(new OptionString(OPTION_OPPONENT));
		(*this)[OPTION_EMERGENCY_TIME] = (Option *)(new OptionSpin(OPTION_EMERGENCY_TIME, 0, 0, 60000));
		(*this)[OPTION_NODES_TIME] = (Option *)(new OptionSpin(OPTION_NODES_TIME, 0, 0, INT_MAX, true));
		(*this)[OPTION_SYZYGY_PATH] = (Option *)(new OptionString(OPTION_SYZYGY_PATH, DEFAULT_SYZYGY_PATH));
		(*this)[OPTION_SYZYGY_PROBE_DEPTH] = (Option *)(new OptionSpin(OPTION_SYZYGY_PROBE_DEPTH, 1, 0, MAX_DEPTH + 1));
		(*this)[OPTION_SYZYGY_PROBE_LIMIT] = (Option*)(new OptionSpin(OPTION_SYZYGY_PROBE_LIMIT, 7, 0, 7));
		(*this)[OPTION_VERBOSITY] = (Option*)(new OptionSpin(OPTION_VERBOSITY, int{ settings::VerbosityLevel::DEFAULT }, int{ settings::VerbosityLevel::MINIMAL }, int{ settings::VerbosityLevel::MAXIMUM }, true));
		(*this)[OPTION_EVAL_FILE] = (Option*)(new OptionString(OPTION_EVAL_FILE, DEFAULT_NET));
		(*this)[OPTION_USE_NNUE] = (Option*)(new OptionCheck(OPTION_USE_NNUE, settings::parameter.UseNNUE));
		(*this)[OPTION_NNEVAL_SCALE_FACTOR] = (Option*)(new OptionSpin(OPTION_NNEVAL_SCALE_FACTOR, settings::parameter.NNScaleFactor, 1, 256, true));
		(*this)[OPTION_NNEVAL_LIMIT] = (Option*)(new OptionSpin(OPTION_NNEVAL_LIMIT, settings::parameter.NNEvalLimit, 0, 30000, true));
	}

	OptionCheck::OptionCheck(std::string Name, bool value, bool Technical)
	{
		name = Name;
		otype = OptionType::CHECK;
		defaultValue = utils::bool2String(value);
		_value = value;
		maxValue = "";
		minValue = "";
		technical = Technical;
	}

	void OptionCheck::set(std::string value) noexcept
	{
		_value = !value.compare("true");
	}

	void OptionContempt::set(std::string value) 
	{
		_value = stoi(value);
		parameter.Contempt = Value(_value);
	}

	void OptionContempt::set(int value) noexcept
	{
		_value = value;
		parameter.Contempt = Value(_value);
	}

	void OptionString::read(std::vector<std::string>& tokens)
	{
		if ((int)tokens.size() > 5) {
			std::stringstream ss;
			ss << tokens[4];
			for (int i = 5; i < (int)tokens.size(); ++i) {
				ss << " " << tokens[i];
			}
			_value = ss.str();
		}
		else if ((int)tokens.size() == 5) _value = tokens[4];
		else _value = "";
	}

	void OptionHash::set(std::string value)
	{
		_value = stoi(value);
		tt::InitializeTranspositionTable();
	}

	void OptionHash::set(int value) noexcept
	{
		_value = value;
		tt::InitializeTranspositionTable();
	}
}
#pragma warning(pop)
