/*
Nemorino is a UCI chess engine authored by Christian Günther
<https://bitbucket.org/christian_g_nther/nemorino/src/master/>

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

#include <stdio.h>
#include <iostream>
#include <fstream>
#include "stdlib.h"
#include "types.h"
#include "board.h"
#include "position.h"
//#include "test.h"
#include "uci.h"
#include "utils.h"
//#include "test.h"

//static bool popcountSupport() noexcept;

void nemorino_initialize()
{

//#ifndef NO_POPCOUNT
//	if (!popcountSupport()) {
//		std::cout << "No Popcount support - Engine does't work on this hardware!" << std::endl;
//		return;
//	}
//#endif // !1
//#ifdef EXTENDED
//	settings::parameter.extendedOptions = true;
//#endif
//	if (argc > 1 && argv[1]) {
//		std::string arg1(argv[1]);
//		if (!arg1.compare("bench")) {
////			int depth = 12;
////			int threads = 1;
////			if (argc > 2) depth = std::atoi(argv[2]);
////			if (argc > 3) threads = std::min(std::atoi(argv[3]), static_cast<int>(std::thread::hardware_concurrency() - 1));
////			int hash = 32 * threads;
////			if (argc > 4) hash = std::atoi(argv[4]);
////			((settings::OptionSpin*)settings::options[settings::OPTION_HASH])->set(hash);
////			Initialize();
////			settings::parameter.UseNNUE = network.Load(settings::parameter.NNUEFile);
////			((settings::OptionSpin*)settings::options[settings::OPTION_THREADS])->set(threads);
////			settings::parameter.HelperThreads = threads - 1;
////			tablebases::init(settings::DEFAULT_SYZYGY_PATH);
////			InitializeMaterialTable();
////			test::benchmark(depth);
//			return 0;
//		}
//		else if (!arg1.rfind("bench")) {
//			Initialize();
//			settings::parameter.UseNNUE = network.Load(settings::parameter.NNUEFile);
//			if (settings::parameter.NNUEFile.length() > 0) {
//				if (settings::parameter.UseNNUE) std::cout << "NN " << settings::parameter.NNUEFile << " loaded!" << std::endl;
//				else std::cout << "Failed to load " << settings::parameter.NNUEFile << "!" << std::endl;
//			}
//			int val = std::stoi(arg1.substr(5));
//			((settings::OptionSpin*)settings::options[settings::OPTION_THREADS])->set(val);
//			settings::parameter.HelperThreads = val;
//			tablebases::init(settings::DEFAULT_SYZYGY_PATH);
//			InitializeMaterialTable();
//			int depth = 13;
//			if (argc > 2) depth = std::atoi(argv[2]);
////			if (argc > 3) test::benchmark(argv[3], depth); else test::benchmark(depth);
//			return 0;
//		}
//		else if (!arg1.compare("-q")) {
//			std::cout << utils::TexelTuneError(argv, argc) << std::endl;
//			return 0;
//		}
//		else if (!arg1.compare("-e")) {
//			settings::parameter.extendedOptions = true;
//		}
////		else if (!arg1.compare("perft")) {
////			Initialize();
////			test::testPerft(test::PerftType::P3);
////		}
//		else if (!arg1.compare("tt") && argc > 3) {
//			Initialize(true);
//			std::cout << utils::TexelTuneError(std::string(argv[2]), std::string(argv[3]));
//			return 0;
//		}
//		else if (!arg1.compare("csf") && argc > 2) {
//			Initialize();
//			settings::parameter.UseNNUE = network.Load(settings::parameter.NNUEFile);
//			if (settings::parameter.UseNNUE) {
//				std::cout << utils::calculateScaleFactor(std::string(argv[2]));
//				return 0;
//			}
//			else {
//				std::cout << "Couldn't load NN EvalFile";
//			}
//		}
////		else if (!arg1.compare("nnue_eval") && argc > 2) {
////			std::cout << test::nnue_evaluate(argv[2], argc > 3 ? argv[3] : "nnue_scored.epd", argc > 4 ? argv[4] : DEFAULT_NET)
////			<< " positions analyzed!" << std::endl;
////			exit(0);
////		}
//	}
//  Initialize();
//  //test::testPack();
//  UCIInterface uciInterface;

}

static Position pos;
static UCIInterface uciInterface;

void nemorino_uci_cmd(const char* str)
{
  std::string input = str;
  
  uciInterface.dispatch(input);
  
  
//		if (!input.compare(0, 3, "uci")) {
//			Initialize();
			//test::testPack();
//			UCIInterface uciInterface;
//			uciInterface.loop();
//			return;
//		}
//		else if (!input.compare(0, 7, "version")) {
//#ifdef NO_POPCOUNT
//			std::cout << VERSION << "." << BUILD_NUMBER << " (No Popcount)" << std::endl;
//#else
//#ifdef PEXT
//			std::cout << VERSION << "." << BUILD_NUMBER << " (BMI2)" << std::endl;
//#else
//			std::cout << "Nemorino " << VERSION << "." << BUILD_NUMBER << std::endl;
//#endif
//#endif
//		}
//		else if (!input.compare(0, 8, "position")) {
//			Initialize();
//			std::string fen;
//			if (input.length() < 10) fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
//			else fen = input.substr(9);
//			pos.setFromFEN(fen);
//		}
//		else if (!input.compare(0, 6, "perft ")) {
//			Initialize();
//			int depth = atoi(input.substr(6).c_str());
//			std::cout << "Perft:\t" << test::perft(pos, depth) << "\t" << pos.fen() << std::endl;
//		}
//		else if (!input.compare(0, 7, "divide ")) {
//			Initialize();
//			int depth = atoi(input.substr(7).c_str());
//			test::divide(pos, depth);
//		}
//		else if (!input.compare(0, 5, "bench")) {
//			Initialize();
//			int depth = 11;
//			if (input.length() > 6) depth = atoi(input.substr(6).c_str());
//			test::benchmark(depth);
//		}
//		else if (!input.compare(0, 5, "print")) {
//			Initialize();
//			std::cout << pos.print() << std::endl;
//		}
//		else if (!input.compare(0, 4, "help")) {
//			constexpr int width = 20;
//			std::cout << "Nemorino is a chess engine supporting UCI protocol. To play" << std::endl;
//			std::cout << "chess with it you need a GUI (like arena or winboard)" << std::endl;
//			std::cout << "License: GNU GENERAL PUBLIC LICENSE v3 (https://www.gnu.org/licenses/gpl.html)" << std::endl << std::endl;
//			std::cout << std::left << std::setw(width) << "uci" << "Starts engine in UCI mode" << std::endl;
//			std::cout << std::left << std::setw(width) << "version" << "Outputs the current version of the engine" << std::endl;
//			std::cout << std::left << std::setw(width) << "position <fen>" << "Sets the engine's position using the FEN-String <fen>" << std::endl;
//			std::cout << std::left << std::setw(width) << "perft <depth>" << "Calculates the Perft count of depth <depth> for the current position" << std::endl;
//			std::cout << std::left << std::setw(width) << "divide <depth>" << "Calculates the Perft count at depth <depth> - 1 for all legal moves" << std::endl;
//			std::cout << std::left << std::setw(width) << "bench [<depth>]" << "Searches a fixed set of position at fixed depth <depth> (default depth is 11)" << std::endl;
//			std::cout << std::left << std::setw(width) << "print" << "Outputs the internal board with some information like evaluation and hash keys" << std::endl;
//			std::cout << std::left << std::setw(width) << "help" << "Prints out this help lines" << std::endl;
//			std::cout << std::left << std::setw(width) << "quit" << "Exits the program" << std::endl;
//		}
//		else if (!input.compare(0, 4, "quit")) break;
//		else if (!input.compare(0, 8, "setvalue")) {
//			std::vector<std::string> token = utils::split(input);
//			int indx = 1;
//			while (indx < (int)token.size() - 1) {
//				settings::options.set(token[indx], token[indx + 1]);
//				indx += 2;
//			}
//		}
}

//#ifdef _MSC_VER
//static bool popcountSupport() noexcept {
//#ifdef _M_ARM
//	return false;
//#else
//	int cpuInfo[4];
//	constexpr int functionId = 0x00000001;
//	__cpuid(cpuInfo, functionId);
//	return (cpuInfo[2] & (1 << 23)) != 0;
//#endif
//}
//#endif // _MSC_VER
//
//#ifdef __GNUC__
//#ifndef __arm__
//#define cpuid(func,ax,bx,cx,dx)\
//	__asm__ __volatile__ ("cpuid":\
//	"=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));
//#endif
//static bool popcountSupport() noexcept {
//#ifdef __arm__
//	return false;
//#else
//	int cpuInfo[4];
//	cpuid(0x00000001, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
//	return (cpuInfo[2] & (1 << 23)) != 0;
//#endif
//}
//#endif // __GNUC__

