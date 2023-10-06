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

//extern bool benchworking;
//bool benchworking = false;

void stockfish_cmd(const char *cmd)
{
    Stockfish::UCI::cmd(cmd);
}

//extern const char* Stockfish::StartFEN;
void _stockfish_initialize();


void stockfish_initialize() {
  // this function is located in main.cpp
  _stockfish_initialize();
}

//int Stockfish::UCI::cmd(const std::string& cmd) {
//  static Stockfish::Position pos;
//  static Stockfish::StateListPtr states(new std::deque<StateInfo>(1));
//  static bool first = true;
//  
//  if (first) {
//    first = false;
//    pos.set(StartFEN, false, &states->back(), Threads.main());
//  }
//  
//    std::istringstream is(cmd);
//    
//    std::string token; // Avoid a stale if getline() returns empty or blank line
//    is >> std::skipws >> token;
//    
//    
//    if (    token == "quit"
//        ||  token == "stop") {
//      Threads.stop = true;
//      benchworking = false;
//    }
//    
//    // The GUI sends 'ponderhit' to tell us the user has played the expected move.
//    // So 'ponderhit' will be sent if we were told to ponder on the same move the
//    // user has played. We should continue searching but switch from pondering to
//    // normal search.
//    else if (token == "ponderhit")
//      Threads.main()->ponder = false; // Switch to normal search
//    
//    else if (token == "uci")
//      sync_cout << "id name " << engine_info(true)
//      << "\n"       << Options
//      << "\nuciok"  << sync_endl;
//    
//    else if (token == "setoption")  setoption(is);
//    else if (token == "go")         go(pos, is, states);
//    else if (token == "position")   position(pos, is, states);
//    else if (token == "ucinewgame") Search::clear();
//    else if (token == "isready")    sync_cout << "readyok" << sync_endl;
//    
//    // Additional custom non-UCI commands, mainly for debugging.
//    // Do not use these commands during a search!
//    else if (token == "flip")     pos.flip();
//    else if (token == "bench")    bench(pos, is, states);
//    else if (token == "d")        sync_cout << pos << sync_endl;
//    else if (token == "eval")     Stockfish::UCI::trace_eval(pos);
//    else if (token == "compiler") sync_cout << compiler_info() << sync_endl;
//    else
//      sync_cout << "Unknown command: " << cmd << sync_endl;
////  }
//  
//  return 1;
//}
