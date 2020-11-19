/*
  This file is part of Leela Chess Zero.
  Copyright (C) 2018-2019 The LCZero Authors

  Leela Chess is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Leela Chess is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Leela Chess.  If not, see <http://www.gnu.org/licenses/>.

  Additional permission under GNU GPL version 3 section 7

  If you modify this Program, or any covered work, by linking or
  combining it with NVIDIA Corporation's libraries from the NVIDIA CUDA
  Toolkit and the NVIDIA CUDA Deep Neural Network library (or a
  modified version of those libraries), containing parts covered by the
  terms of the respective license agreement, the licensors of this
  Program grant you additional permission to convey the resulting work.
*/

#include "benchmark/benchmark.h"
//#include "benchmark/backendbench.h"
#include "chess/board.h"
#include "engine.h"
//#include "selfplay/loop.h"
#include "utils/commandline.h"
#include "utils/esc_codes.h"
#include "utils/logging.h"
#include "utils/numa.h"
#include "lc0version.h"

lczero::EngineLoop *engineLoop = nullptr;
lczero::Benchmark* benchmark = nullptr;

extern std::string lc0netpath;

void lc0_cmd(const char *cmd)
{
  if (memcmp(cmd, "bench", strlen("bench")) == 0) {
    if (benchmark) delete benchmark;
    benchmark = new lczero::Benchmark();
    benchmark->Run(lc0netpath);
  } else {
    engineLoop->RunCmd(cmd);
  }
}

void lc0_initialize() {
  using namespace lczero;
  EscCodes::Init();
  LOGFILE << "Lc0 started.";
  CERR << EscCodes::Bold() << EscCodes::Red() << "       _";
  CERR << "|   _ | |";
  CERR << "|_ |_ |_|" << EscCodes::Reset() << " v" << GetVersionStr()
       << " built " << __DATE__;

  try {
    InitializeMagicBitboards();
    Numa::Init();

//    CommandLine::Init(argc, argv);
    CommandLine::RegisterMode("uci", "(default) Act as UCI engine");
    CommandLine::RegisterMode("selfplay", "Play games with itself");
    CommandLine::RegisterMode("benchmark", "Quick benchmark");
    CommandLine::RegisterMode("backendbench", "Quick benchmark of backend only");

//    if (CommandLine::ConsumeCommand("selfplay")) {
//      // Selfplay mode.
//      SelfPlayLoop loop;
//      loop.RunLoop();
//    } else if (CommandLine::ConsumeCommand("benchmark")) {
//      // Benchmark mode.
//      Benchmark benchmark;
//      benchmark.Run();
//    } else if (CommandLine::ConsumeCommand("backendbench")) {
//      // Backend Benchmark mode.
//      BackendBenchmark benchmark;
//      benchmark.Run();
//    } else
    {
      // Consuming optional "uci" mode.
      CommandLine::ConsumeCommand("uci");
      // Ordinary UCI engine.
//      EngineLoop loop;
//      loop.RunLoop();
////      lc0_cmd("uci");
//      loop.RunCmd("uci");
      
      engineLoop = new lczero::EngineLoop();
      engineLoop->RunLoop();
//      lc0_cmd("uci");
      lc0_cmd("setoption name MinibatchSize value 8");
      lc0_cmd("setoption name MaxPrefetch value 0");
      lc0_cmd("uci");
    }
  } catch (std::exception& e) {
    std::cerr << "Unhandled exception: " << e.what() << std::endl;
    abort();
  }
}

