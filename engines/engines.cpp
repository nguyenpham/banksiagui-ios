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

#include "engines-bridging-header.h"


static std::map<int, std::vector<std::string>> searchMsgMap;
static std::map<int, int> coreMap;

static std::string nnuepath;
std::string lc0netpath;
static std::set<int> initSet;
static std::map<int, int> skillLevelMap;

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

#ifndef LC0ONLY
void stockfish_initialize();
void stockfish_cmd(const char *cmd);
#endif

#ifndef STOCKFISHONLY
void lc0_initialize();
void lc0_cmd(const char *cmd);
#endif

#ifndef NNONLY
void stockfish_initialize();
void stockfish_cmd(const char *cmd);

void stockfish_initialize();
void stockfish_cmd(const char *cmd);

#endif


#if !defined STOCKFISHONLY && !defined LC0ONLY

#ifndef NNONLY
extern "C" void ethereal_initialize();
extern "C" void ethereal_uci_cmd(const char* str);
extern "C" void xiphos_initialize();
extern "C" void xiphos_uci_cmd(const char* str);

void depenchess_initialize();
void depenchess_uci_cmd(const char* str);

void rubichess_initialize();
void rubichess_uci_cmd(const char* str);

void laser_initialize();
void laser_uci_cmd(const char* str);

#endif

void igel_initialize();
void igel_uci_cmd(const char* str);

//void nemorino_initialize();
//void nemorino_uci_cmd(const char* str);
//
//void fruit_initialize();
//void fruit_uci_cmd(const char* str);

#endif

extern "C" void setStockfishNNUEPath(const char *path) {
  nnuepath = path;
}

extern "C" void setLc0NetPath(const char *path) {
  lc0netpath = path;
}

void sendOptionLc0Network(int eid)
{
  assert(!lc0netpath.empty());
  auto cmd = "setoption name WeightsFile value " + lc0netpath;
  engine_cmd(eid, cmd.c_str());
}


void sendOptionNNUE(int eid)
{
  assert(!nnuepath.empty());
  auto cmd = "setoption name EvalFile value " + nnuepath;
  engine_cmd(eid, cmd.c_str());
}

void sendSkillLevel(int eid, int skillLevel)
{
  auto cmd = "setoption name Skill Level value " + std::to_string(skillLevel);
  engine_cmd(eid, cmd.c_str());
}

void engine_initialize(int eid, int coreNumber, int skillLevel, int nnueMode)
{
  if (initSet.find(eid) == initSet.end()) {
    initSet.insert(eid);
    
    switch (eid) {
#ifndef LC0ONLY
      case stockfish:
        stockfish_initialize();  /// Threads, Hash, Ponder, EvalFile, Skill level
        sendOptionNNUE(eid);
        break;
#endif
        
#ifndef STOCKFISHONLY
      case lc0:
        lc0_initialize();  /// Threads, Hash, Ponder, EvalFile, Skill level
        sendOptionLc0Network(eid);
        break;
#endif

#if !(defined STOCKFISHONLY || defined LC0ONLY)
        
      case igel:
        igel_initialize();  /// Threads, Hash, Ponder, EvalFile, Skill level
        sendOptionNNUE(eid);
        break;
        
//      case nemorino:
//        nemorino_initialize();  /// Threads, Hash, Ponder, EvalFile
//        sendOptionNNUE(eid);
//        break;

#ifndef NNONLY
      case ethereal:
        ethereal_initialize();  /// Threads, Hash, Ponder
        break;
        
      case xiphos:
        xiphos_initialize();  /// Threads, Hash, Ponder
        break;
        
      case rubichess:
        rubichess_initialize();  /// Threads, Hash, Ponder
        break;
        
      case laser:
        laser_initialize();  /// Threads, Hash, Ponder
        break;
        
      case defenchess:
        depenchess_initialize();  /// Threads, Hash, Ponder
        break;
                
//      case fruit:
//        fruit_initialize();  /// Hash Ponder, no threads
//        break;
#endif /// #ifndef NNONLY
#endif

      default:
        break;
    }
    
#define HashSize  "128"
    auto cmd = std::string("setoption name hash value ") + HashSize;
    engine_cmd(eid, cmd.c_str());
  }
  
  if (coreMap.find(eid) == coreMap.end() || coreMap[eid] != coreNumber) {
    coreMap[eid] = coreNumber;
    std::string cmd = "setoption name threads value " + std::to_string(coreNumber);
    engine_cmd(eid, cmd.c_str());
  }

#ifndef LC0ONLY
  if (eid == stockfish) {
    if (skillLevelMap.find(eid) == skillLevelMap.end() || skillLevelMap[eid] != skillLevel) {
      skillLevelMap[eid] = skillLevel;
      sendSkillLevel(eid, skillLevel);
    }
    
    static int sfnnueMode = -1;
    if (sfnnueMode != nnueMode) {
      sfnnueMode = nnueMode;
      std::string cmd = "setoption name Use NNUE value " + std::to_string(nnueMode);
      engine_cmd(eid, cmd.c_str());
    }
  }
#endif
  
}

void engine_cmd(int eid, const char *cmd)
{
  switch (eid) {
#ifndef LC0ONLY
    case stockfish:
      stockfish_cmd(cmd);
      break;
#endif
      
#ifndef STOCKFISHONLY
      case lc0:
        lc0_cmd(cmd);
        break;
#endif

#if !(defined STOCKFISHONLY || defined LC0ONLY)

#ifndef NNONLY
    case ethereal:
      ethereal_uci_cmd(cmd);
      break;
      
    case xiphos:
      xiphos_uci_cmd(cmd);
      break;
      
    case rubichess:
      rubichess_uci_cmd(cmd);
      break;
      
    case laser:
      laser_uci_cmd(cmd);
      break;
      
    case defenchess:
      depenchess_uci_cmd(cmd);
      break;
      
#endif /// #ifndef NNONLY

    case igel:
      igel_uci_cmd(cmd);
      break;

//    case nemorino:
//      nemorino_uci_cmd(cmd);
//      break;
//
//    case fruit:
//      fruit_uci_cmd(cmd);
//      break;
#endif

    default:
      break;
  }
}




