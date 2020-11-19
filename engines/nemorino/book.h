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
#include <fstream>
#include <string>
#include "types.h"
#include "position.h"
#include "utils.h"

namespace polyglot {

	//A Polyglot book is a series of "entries" of 16 bytes
	//All integers are stored highest byte first(regardless of size)
	//The entries are ordered according to key.Lowest key first.
	struct Entry {
		uint64_t key = 0;
		uint16_t move = MOVE_NONE;
		uint16_t weight = 0;
		uint32_t learn = 0;
	};

	class Book : private std::ifstream
	{
	public:
		Book();
		explicit Book(const std::string& filename);
		~Book();
		Book(const Book&) = default;
		Book& operator=(const Book&) = default;
		Book(Book&&) = default;
		Book& operator=(Book&&) = default;



		Move probe(Position& pos, bool pickBest, ValuatedMove * moves, int moveCount);

	private:
		std::string fileName = "book.bin";
		size_t count;
		Entry readEntry();
	};

}
