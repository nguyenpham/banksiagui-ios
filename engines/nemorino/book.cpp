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

#include "book.h"
#include <vector>
#include <cstdlib>
#include <time.h>
#include <string>
#include <algorithm>
#include <iostream>
#ifdef _MSC_VER // Windows
#include <process.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif
#include "utils.h"


namespace polyglot {

	Book::Book() {
		this->open(fileName, std::ifstream::in | std::ifstream::binary);
		this->seekg(0, std::ios::end);
		count = size_t(this->tellg() / sizeof(Entry));
#ifdef _MSC_VER // Windows
		srand(uint32_t(time(NULL)*_getpid()));
#else
        srand(uint32_t(time(NULL)*getpid()));
#endif
	}

	Book::Book(const std::string& filename)
	{
		fileName = filename;
		this->open(fileName, std::ifstream::in | std::ifstream::binary);
		this->seekg(0, std::ios::end);
		count = size_t(this->tellg() / sizeof(Entry));
#ifdef _MSC_VER // Windows
		srand(uint32_t(time(NULL)*_getpid()));
#else
        srand(uint32_t(time(NULL)*getpid()));
#endif
	}


	Book::~Book()
	{
		if (is_open()) close();
	}

	Entry Book::readEntry() {
		Entry result;
		result.key = 0;
		result.move = 0;
		result.weight = 0;
		for (int i = 0; i < 8; ++i) result.key = (result.key << 8) + std::ifstream::get();
		for (int i = 0; i < 2; ++i) result.move = (uint16_t)((result.move << 8) + std::ifstream::get());
		for (int i = 0; i < 2; ++i) result.weight = (uint16_t)((result.weight << 8) + std::ifstream::get());
		return result;
	}

	Move Book::probe(Position& pos, bool pickBest, ValuatedMove * moves, int moveCount) {
		if (!is_open()) return MOVE_NONE;
		//Make a binary search to find the right Entry
		size_t low = 0;
		size_t high = count - 1;
		Entry entry;
		size_t searchPoint = 0;
		while (low < high && good()) {
			searchPoint = low + (high - low) / 2;
			seekg(searchPoint * sizeof(Entry), ios_base::beg);
			entry = readEntry();
			if (pos.GetHash() == entry.key) break;
			else if (pos.GetHash() < entry.key) high = searchPoint; else low = searchPoint + 1;
		}
#pragma warning(suppress: 6001)
		if (entry.key != pos.GetHash()) return MOVE_NONE;
		std::vector<Entry> entries;
		entries.push_back(entry);
		const size_t searchPointStore = searchPoint;
		while (searchPoint > 0 && good()) {
			searchPoint--;
			seekg(searchPoint * sizeof(Entry), ios_base::beg);
			entry = readEntry();
			if (pos.GetHash() != entry.key) break; else entries.push_back(entry);
		}
		searchPoint = searchPointStore;
		while (searchPoint < (count - 1) && good()) {
			searchPoint++;
			seekg(searchPoint * sizeof(Entry), ios_base::beg);
			entry = readEntry();
			if (pos.GetHash() != entry.key) break; else entries.push_back(entry);
		}
		Entry best = entries[0];
		uint32_t sum = best.weight;
		for (size_t i = 1; i < entries.size(); ++i) {
			const Entry e = entries[i];
			if (e.weight > best.weight) best = e;
			sum += e.weight;
		}
		Move move = best.move;
		if (!pickBest && entries.size() > 0) {
			const uint32_t indx = rand() % sum;
			sum = 0;
			for (size_t i = 0; i < entries.size(); ++i) {
				const Entry e = entries[i];
				sum += e.weight;
				if (sum > indx) {
					move = e.move;
					break;
				}
			}
		}
		//Now move contains move in polyglot representation
		const int pt = (move >> 12) & 7;
		if (pt)
			move = createMove<PROMOTION>(from(move), to(move), PieceType(4 - pt));
		else {
			//Castling is stored as king captures rook
			if (pos.GetSideToMove() == WHITE && from(move) == InitialKingSquare[WHITE]) {
				if (to(move) == InitialRookSquare[0] && pos.CastlingAllowed(W0_0)) {
					move = createMove<CASTLING>(InitialKingSquare[WHITE], G1);
				}
				else if (to(move) == InitialRookSquare[1] && pos.CastlingAllowed(W0_0_0)) {
					move = createMove<CASTLING>(InitialKingSquare[WHITE], C1);
				}
			}
			else if (pos.GetSideToMove() == BLACK && from(move) == InitialKingSquare[BLACK]) {
				if (to(move) == InitialRookSquare[2] && pos.CastlingAllowed(B0_0)) {
					move = createMove<CASTLING>(InitialKingSquare[BLACK], G8);
				}
				else if (to(move) == InitialRookSquare[3] && pos.CastlingAllowed(B0_0_0)) {
					move = createMove<CASTLING>(InitialKingSquare[BLACK], C8);
				}
			}
		}
		//Now check if polyglot move is part of generated move list
		move &= 0x3FFF;
		for (int i = 0; i < moveCount; i++) {
			if ((moves[i].move & 0x3FFF) == move) return moves[i].move;
		}
		return MOVE_NONE;
	}

}
