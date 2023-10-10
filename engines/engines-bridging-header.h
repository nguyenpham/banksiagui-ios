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

#ifndef SFWapper_hpp
#define SFWapper_hpp

#include <stdio.h>
#include "engineids.h"

#ifdef __cplusplus
extern "C" {
#endif


void engine_initialize(int eid, int coreNumber, int skillLevel, int nnueMode);
void engine_cmd(int eid, const char *cmd);
const char *engine_getSearchMessage(int eid);
void engine_clearAllMessages(int eid);

void setNetworkPath(int eid, const char *path);

#ifdef __cplusplus
}
#endif

#endif /* SFWapper_hpp */
