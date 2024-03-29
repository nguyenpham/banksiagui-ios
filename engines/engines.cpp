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
#include <set>
#include <map>
#include <vector>
#include <assert.h>

#include "engines-bridging-header.h"

#define HashSize  "128"

static std::map<int, std::vector<std::string>> searchMsgMap;
static std::map<int, int> coreMap;

static std::map<int, std::string> networkMap;

std::string lc0netpath;
static std::set<int> initSet;

static std::mutex searchMsgVecMutex;

void engine_message(int eid, const std::string& str) {
    std::lock_guard<std::mutex> lock(searchMsgVecMutex);
    if (searchMsgMap.find(eid) == searchMsgMap.end()) {
        std::vector<std::string> vec;
        vec.push_back(str);
        searchMsgMap[eid] = vec;
    } else {
        searchMsgMap[eid].push_back(str);
    }
    std::cout << str << std::endl;
}

extern "C" void engine_message_c(int eid, const char* s) {
    std::string str = std::string(s);
    engine_message(eid, str);
}

extern "C" const char* engine_getSearchMessage(int eid) {
    if (searchMsgMap.find(eid) != searchMsgMap.end()) {
        std::lock_guard<std::mutex> lock(searchMsgVecMutex);
        if (searchMsgMap.find(eid) != searchMsgMap.end() && !searchMsgMap[eid].empty()) {
            static std::string tmpString;
            tmpString = searchMsgMap[eid].front();
            searchMsgMap[eid].erase(searchMsgMap[eid].begin());
            return tmpString.c_str();
        }
    }
    return nullptr;
}

extern "C" void engine_clearAllMessages(int eid)
{
    if (searchMsgMap.find(eid) != searchMsgMap.end()) {
        std::lock_guard<std::mutex> lock(searchMsgVecMutex);
        if (searchMsgMap.find(eid) != searchMsgMap.end()) {
            searchMsgMap[eid].clear();
        }
    }
}

void stockfish_initialize();
void stockfish_cmd(const char *cmd);
void stockfish_cleanup();

void lc0_initialize();
void lc0_cmd(const char *cmd);
void lc0_cleanup();


void rubichess_initialize();
void rubichess_uci_cmd(const char* str);
void rubichess_cleanup();


extern "C" void setNetworkPath(int eid, const char *path)
{
    networkMap[eid] = path;
    if (eid == lc0) {
        lc0netpath = path;
    }
}

void sendOptionLc0Network(int eid)
{
    auto lc0netpath = networkMap[lc0];
    assert(!lc0netpath.empty());
    auto cmd = "setoption name WeightsFile value " + lc0netpath;
    engine_cmd(eid, cmd.c_str());
}


void sendOptionNNUE_SF()
{
    auto path = networkMap[stockfish];
    assert(!path.empty());
    auto cmd = "setoption name EvalFile value " + path;
    engine_cmd(stockfish, cmd.c_str());
}

void sendOptionNNUE_Rubi()
{
    auto path = networkMap[rubi];
    assert(!path.empty());
    auto cmd = "setoption name NNUENetpath value " + path;
    engine_cmd(rubi, cmd.c_str());
}

void engine_initialize(int eid, int coreNumber)
{
    
#ifndef USE_NEON
    //        #pragma message ("WARNING: NEON is NOT defined")
    std::cout<< "WARNING: NEON is NOT defined" << std::endl;
#endif
    
    if (initSet.find(eid) == initSet.end()) {
        initSet.insert(eid);
        
        switch (eid) {
            case stockfish:
                stockfish_initialize();  /// Threads, Hash, Ponder, EvalFile, Skill level
                sendOptionNNUE_SF();
                break;
                
            case lc0:
                lc0_initialize();  /// Threads, Hash, Ponder, EvalFile, Skill level
                sendOptionLc0Network(eid);
                break;
                
            case rubi:
                rubichess_initialize();  /// Threads, Hash, Ponder
                sendOptionNNUE_Rubi();
                break;
                
            default:
                break;
        }
        
        if (eid != lc0) { // lc0 has not option hash
            auto cmd = std::string("setoption name hash value ") + HashSize;
            engine_cmd(eid, cmd.c_str());
        }
    }
    
    if (coreMap.find(eid) == coreMap.end()
        || coreMap[eid] != coreNumber) {
        coreMap[eid] = coreNumber;
        std::string cmd = "setoption name threads value " + std::to_string(coreNumber);
        engine_cmd(eid, cmd.c_str());
    }
}
                 
void engine_cmd(int eid, const char *cmd) {
    switch (eid) {
        case stockfish:
            stockfish_cmd(cmd);
            break;
            
        case lc0:
            lc0_cmd(cmd);
            break;
            
        case rubi:
            rubichess_uci_cmd(cmd);
            break;
            
        default:
            break;
    }
}




