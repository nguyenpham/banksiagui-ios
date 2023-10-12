/*
  This file is part of Leela Chess Zero.
  Copyright (C) 2018-2021 The LCZero Authors

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

//#include "benchmark/lc0_backendbench.h"
#include "benchmark/lc0_benchmark.h"
//#include "chess/lc0_board.h"
#include "lc0_engine.h"

//#include "lc0ctl/describenet.h"
//#include "lc0ctl/leela2onnx.h"
//#include "lc0ctl/onnx2leela.h"
//#include "selfplay/loop.h"

#include "utils/lc0_commandline.h"
//#include "utils/lc0_esc_codes.h"
#include "utils/lc0_logging.h"
//#include "lc0_version.h"

using namespace lczero;

extern std::string lc0netpath;

class Lc0 {
private:
    EngineLoop engineLoop;
    Benchmark* benchmark = nullptr;

public:
    Lc0() {
        try {
            InitializeMagicBitboards();

            CommandLine::RegisterMode("uci", "(default) Act as UCI engine");
            CommandLine::RegisterMode("benchmark", "Quick benchmark");

            // Consuming optional "uci" mode.
            CommandLine::ConsumeCommand("uci");

            // Ordinary UCI engine.
            engineLoop.RunLoop();

            doCmd("setoption name MinibatchSize value 8");
            doCmd("setoption name MaxPrefetch value 0");
            doCmd("uci");
        } catch (std::exception& e) {
            std::cerr << "Unhandled exception: " << e.what() << std::endl;
            abort();
        }
    }
    
    ~Lc0() {
        if (benchmark) {
            delete benchmark;
            benchmark = nullptr;
        }
    }
    
    void doCmd(const char *cmd)
    {
      if (memcmp(cmd, "bench", strlen("bench")) == 0) {
        if (benchmark) delete benchmark;
        benchmark = new lczero::Benchmark();
        benchmark->Run(lc0netpath);
      } else {
        engineLoop.RunCmd(cmd);
      }
    }

};

static Lc0* lc0 = nullptr;

void lc0_cmd(const char *cmd)
{
    if (lc0) {
        lc0->doCmd(cmd);
    }
}

void lc0_cleanup() {
    if (lc0) {
        delete lc0;
        lc0 = nullptr;
    }
}

void lc0_initialize() {
    lc0_cleanup();
    lc0 = new Lc0();
}


//int main(int argc, const char** argv) {
//  EscCodes::Init();
//  LOGFILE << "Lc0 started.";
//  CERR << EscCodes::Bold() << EscCodes::Red() << "       _";
//  CERR << "|   _ | |";
//  CERR << "|_ |_ |_|" << EscCodes::Reset() << " v" << GetVersionStr()
//       << " built " << __DATE__;
//
//  try {
//    InitializeMagicBitboards();
//
//    CommandLine::Init(argc, argv);
//    CommandLine::RegisterMode("uci", "(default) Act as UCI engine");
////    CommandLine::RegisterMode("selfplay", "Play games with itself");
//    CommandLine::RegisterMode("benchmark", "Quick benchmark");
//    CommandLine::RegisterMode("backendbench",
//                              "Quick benchmark of backend only");
////    CommandLine::RegisterMode("leela2onnx", "Convert Leela network to ONNX.");
////    CommandLine::RegisterMode("onnx2leela",
////                              "Convert ONNX network to Leela net.");
////    CommandLine::RegisterMode("describenet",
////                              "Shows details about the Leela network.");
////
////    if (CommandLine::ConsumeCommand("selfplay")) {
////      // Selfplay mode.
////      SelfPlayLoop loop;
////      loop.RunLoop();
////    } else 
//    if (CommandLine::ConsumeCommand("benchmark")) {
//      // Benchmark mode.
//      Benchmark benchmark;
//      benchmark.Run();
////    } else if (CommandLine::ConsumeCommand("backendbench")) {
////      // Backend Benchmark mode.
////      BackendBenchmark benchmark;
////      benchmark.Run();
////    } else if (CommandLine::ConsumeCommand("leela2onnx")) {
////      lczero::ConvertLeelaToOnnx();
////    } else if (CommandLine::ConsumeCommand("onnx2leela")) {
////      lczero::ConvertOnnxToLeela();
////    } else if (CommandLine::ConsumeCommand("describenet")) {
////      lczero::DescribeNetworkCmd();
//    } else {
//      // Consuming optional "uci" mode.
//      CommandLine::ConsumeCommand("uci");
//      // Ordinary UCI engine.
//      EngineLoop loop;
//      loop.RunLoop();
//    }
//  } catch (std::exception& e) {
//    std::cerr << "Unhandled exception: " << e.what() << std::endl;
//    abort();
//  }
//}
