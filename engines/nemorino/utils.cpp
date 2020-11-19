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

#include <fstream>
#include <vector>
#include <exception>
#include <mutex>
#include <map>
#include <algorithm>
#include <thread>
#include <future>

#include "types.h"
#include "uci.h"
#include "position.h"
#include "search.h"
#include "utils.h"

#ifdef _WIN32
#if _WIN32_WINNT < 0x0601
#undef  _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Force to include needed API prototypes
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
// The needed Windows API for processor groups could be missed from old Windows
// versions, so instead of calling them directly (forcing the linker to resolve
// the calls at compile time), try to load them at runtime. To do this we need
// first to define the corresponding function pointers.
extern "C" {
	typedef bool(*fun1_t)(LOGICAL_PROCESSOR_RELATIONSHIP,
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, PDWORD);
	typedef bool(*fun2_t)(USHORT, PGROUP_AFFINITY);
	typedef bool(*fun3_t)(HANDLE, CONST GROUP_AFFINITY*, PGROUP_AFFINITY);
}
#endif



//copied from SF
std::ostream& operator<<(std::ostream& os, NemorionoSyncCout sc) {

	static std::mutex m;

	if (sc == IO_LOCK) {
		m.lock();
	} else
		m.unlock();

	return os;
}

namespace NemorinoWinProcGroup {

#ifndef _WIN32

	void bindThisThread(size_t) {}

#else

	std::mutex mtxBindThread;

	/// best_group() retrieves logical processor information using Windows specific
	/// API and returns the best group id for the thread with index idx. Original
	/// code from Texel by Peter Österlund.

	int best_group(size_t idx) {

		int threads = 0;
		int nodes = 0;
		int cores = 0;
		DWORD returnLength = 0;
		DWORD byteOffset = 0;

		// Early exit if the needed API is not available at runtime
		HMODULE k32 = GetModuleHandle("Kernel32.dll");
		auto fun1 = (fun1_t)(void(*)())GetProcAddress(k32, "GetLogicalProcessorInformationEx");
		if (!fun1)
			return -1;

		// First call to get returnLength. We expect it to fail due to null buffer
		if (fun1(RelationAll, nullptr, &returnLength))
			return -1;

		// Once we know returnLength, allocate the buffer
		SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* buffer, * ptr;
		ptr = buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)malloc(returnLength);

		// Second call, now we expect to succeed
		if (!fun1(RelationAll, buffer, &returnLength))
		{
			free(buffer);
			return -1;
		}

		while (ptr->Size > 0 && byteOffset + ptr->Size <= returnLength)
		{
			if (ptr->Relationship == RelationNumaNode)
				nodes++;

			else if (ptr->Relationship == RelationProcessorCore)
			{
				cores++;
				threads += (ptr->Processor.Flags == LTP_PC_SMT) ? 2 : 1;
			}

			byteOffset += ptr->Size;
			ptr = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)(((char*)ptr) + ptr->Size);
		}

		free(buffer);

		std::vector<int> groups;

		// Run as many threads as possible on the same node until core limit is
		// reached, then move on filling the next node.
		for (int n = 0; n < nodes; n++)
			for (int i = 0; i < cores / nodes; i++)
				groups.push_back(n);

		// In case a core has more than one logical processor (we assume 2) and we
		// have still threads to allocate, then spread them evenly across available
		// nodes.
		for (int t = 0; t < threads - cores; t++)
			groups.push_back(t % nodes);

		// If we still have more threads than the total number of logical processors
		// then return -1 and let the OS to decide what to do.
		return idx < groups.size() ? groups[idx] : -1;
	}


	/// bindThisThread() set the group affinity of the current thread

	void bindThisThread(size_t idx) {

		std::lock_guard<std::mutex> lockBindThread(mtxBindThread);

		// Use only local variables to be thread-safe
		const int group = best_group(idx);

		if (group == -1)
			return;

		// Early exit if the needed API are not available at runtime
		HMODULE k32 = GetModuleHandle("Kernel32.dll");
		auto fun2 = (fun2_t)(void(*)())GetProcAddress(k32, "GetNumaNodeProcessorMaskEx");
		auto fun3 = (fun3_t)(void(*)())GetProcAddress(k32, "SetThreadGroupAffinity");

		if (!fun2 || !fun3)
			return;

		GROUP_AFFINITY affinity;
		if (fun2(group, &affinity))
			fun3(GetCurrentThread(), &affinity, nullptr);
	}

#endif

} // namespace WinProcGroup


namespace utils {

	const std::string WHITESPACE = " \n\r\t";

	double K = -1.26 / 400;

	std::string TrimLeft(const std::string& s)
	{
		const size_t startpos = s.find_first_not_of(WHITESPACE);
		return (startpos == std::string::npos) ? "" : s.substr(startpos);
	}

	std::string TrimRight(const std::string& s)
	{
		const size_t endpos = s.find_last_not_of(WHITESPACE);
		return (endpos == std::string::npos) ? "" : s.substr(0, endpos + 1);
	}

	std::string Trim(const std::string& s)
	{
		return TrimRight(TrimLeft(s));
	}

	uint64_t MurmurHash2A(uint64_t input, uint64_t seed) noexcept
	{
		constexpr uint64_t m = 0xc6a4a7935bd1e995;
		constexpr int r = 47;
#pragma warning( push )
#pragma warning( disable: 4307 )
		uint64_t h = seed ^ (8 * m);
#pragma warning( pop )
		uint64_t k = input;

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;

		h ^= h >> r;
		h *= m;
		h ^= h >> r;

		return h;
	}

	std::string mirrorFenVertical(std::string fen)
	{
		std::vector<std::string> token = split(fen);
		if (token.size() < 4) return std::string();
		std::vector<std::string> rows = split(token[0], '/');
		if (rows.size() != 8) return std::string();
		std::stringstream ss;
		bool first = true;
		for (auto row : rows) {
			if (!first) ss << '/';
			first = false;
			std::reverse(row.begin(), row.end());
			ss << row;
		}
		ss << ' ' << token[1] << " - ";
		if (token[3].compare("-") == 0) ss << '-';
		else {
			const char file = (char)((int)'h' - ((int)token[3][0] - (int)'a'));
			ss << file << token[3][1];
		}
		if (token.size() > 4) ss << ' ' << token[4];
		if (token.size() > 5) ss << ' ' << token[5];
		return ss.str();
	}

	double CalcErrorForPackage(const std::vector<std::string>& lines) {
		double error = 0.0;
#pragma warning(suppress: 26409)
		NemorinoSearch* Engine = new NemorinoSearch();
		Engine->Stop.store(false);
		for (auto line : lines) {
			const std::size_t found = line.find(" c9 ");
			Position pos(line.substr(0, found));
			double result = stod(line.substr(found + 4));
			Value score = Engine->qscore(&pos);
			if (pos.GetSideToMove() == BLACK) score = -score;
			const double lerror = result - utils::sigmoid(score);
			error += lerror * lerror;
		}
		delete Engine;
		return error;
	}

	double TexelTuneError(const char* argv[], int argc)
	{
		assert(argc > 3);
		settings::parameter.USE_TT_IN_QSEARCH = false;
		Initialize();
		std::string filename(argv[2]);
		const int packageCount = (int)std::thread::hardware_concurrency() - 1;
		std::vector<std::vector<std::string>> packages(packageCount);
		std::ifstream infile(filename);
		double error = 0.0;
		int n = 0;
		for (std::string line; getline(infile, line); )
		{
			packages[n % packageCount].push_back(line);
			++n;
		}
		std::vector<std::future<double>> futures;
		for (int i = 0; i < packageCount; ++i) {
			futures.push_back(std::async(std::launch::async, CalcErrorForPackage, std::ref(packages[i])));
		}
		for (int i = 0; i < packageCount; ++i) {
			error += futures[i].get();
		}
		//for (int i = 0; i < packageCount; ++i) error += CalcErrorForPackage(packages[i]);
		return error / n;
	}

	double TexelTuneError(std::string data, std::string parameter)
	{
		settings::parameter.USE_TT_IN_QSEARCH = false;
		std::ifstream paramfile(parameter);
		std::vector<std::string> vparam;
		for (std::string line; getline(paramfile, line); )
		{
			const std::size_t found = line.find("=");
			if (found != std::string::npos) {
				if (found == 1 && !line.substr(0, found).compare("K")) K = std::stod(line.substr(found + 1));
				else
					settings::parameter.SetFromUCI(line.substr(0, found), line.substr(found + 1));
			}
		}
		InitializeMaterialTable();
		settings::parameter.HelperThreads = 0;
		const int packageCount = (int)std::thread::hardware_concurrency() - 1;
		//int packageCount = 1;
		std::vector<std::vector<std::string>> packages(packageCount);
		std::ifstream infile(data);
		double error = 0.0;
		int n = 0;
		for (std::string line; getline(infile, line); )
		{
			packages[n % packageCount].push_back(line);
			++n;
		}
		std::vector<std::future<double>> futures;
		for (int i = 0; i < packageCount; ++i) {
			futures.push_back(std::async(std::launch::async, CalcErrorForPackage, std::ref(packages[i])));
		}
		std::vector<double> results;
		for (int i = 0; i < packageCount; ++i) {
			results.push_back(futures[i].get());
		}
		for (auto d : results) {
			error += d;
		}
		//for (int i = 0; i < packageCount; ++i) error += CalcErrorForPackage(packages[i]);
		return error / n;
	}

	double calculateScaleFactor(std::string epd)
	{
		std::ifstream infile(epd);
		std::vector<Value> psqEval;
		std::vector<Value> staticEval;
		std::vector<Value> nnEval;
		for (std::string line; getline(infile, line); ) {
			size_t index = 0;
			for (int i = 0; i < 4; ++i) {
				index = line.find(' ', index + 1);
			}
			if (index == std::string::npos) continue;
			std::string record = line.substr(0, index);
			Position pos(record);
			if (pos.Checked()) 
				continue;
			Value psqVal = (pos.GetPsqEval() + pos.GetMaterialTableEntry()->Evaluation).egScore;
			int absVal = std::abs(static_cast<int>(psqVal));
			if (absVal > 300) continue;
			if (pos.GetSideToMove() == Color::BLACK) psqVal = -psqVal;
			settings::parameter.UseNNUE = false;
			Value staticVal = pos.GetMaterialTableEntry()->EvaluationFunction(pos);
			settings::parameter.UseNNUE = true;
			Value nnVal = static_cast<Value>(pos.NNScore());
			psqEval.push_back(psqVal);
			staticEval.push_back(staticVal);
			nnEval.push_back(nnVal);
		}
		//calculate averages
		double avgPsq = 0;
		for (Value v : psqEval) {
			avgPsq += std::abs(static_cast<int>(v));
		}
		avgPsq /= psqEval.size();
		double avgStatic = 0;
		for (Value v : staticEval) {
			avgStatic += std::abs(static_cast<int>(v));
		}
		avgStatic /= staticEval.size();
		double avgNN = 0;
		for (Value v : nnEval) {
			avgNN += std::abs(static_cast<int>(v));
		}
		avgNN /= nnEval.size();
		return avgStatic / avgNN;
	}

	void replaceExt(std::string& s, const std::string& newExt) {
		const std::string::size_type i = s.rfind('.', s.length());
		if (i != std::string::npos) {
			s.replace(i + 1, newExt.length(), newExt);
		}
	}

	void debugInfo(std::string info)
	{
		sync_cout << "info string " << info << sync_endl;
	}

	void debugInfo(std::string info1, std::string info2)
	{
		sync_cout << "info string " << info1 << " " << info2 << sync_endl;
	}


	std::vector<std::string> split(const std::string str, char sep) {
		std::vector<std::string> tokens;
		std::stringstream ss(str); // Turn the string into a stream.
		std::string tok;
		while (std::getline(ss, tok, sep)) {
			tokens.push_back(tok);
		}
		return tokens;
	}

}
