/*
  Banksia GUI, a chess GUI for iOS
  Copyright (C) 2020 Nguyen Hong Pham

  Banksia GUI is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Banksia GUI is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <iostream>
#include <sstream>

#include "stockfish/misc.h"
#include "stockfish/uci.h"
#include "stockfish/position.h"
#include "stockfish/thread.h"

void stockfish_cmd(const char *cmd)
{
    Stockfish::UCI::cmd(cmd);
}

//void _stockfish_initialize();
//void stockfish_initialize() {
//  // this function is located in stocfish/main.cpp
//  _stockfish_initialize();
//}

void stockfish_cleanup() {
}

