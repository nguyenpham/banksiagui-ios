/*
  Xiphos, a UCI chess engine
  Copyright (C) 2018, 2019 Milos Tatarevic

  Xiphos is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Xiphos is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>

#include "eval.h"
#include "game.h"
#include "hash.h"
#include "make.h"
#include "pawn_eval.h"
#include "perft.h"
#include "phash.h"
#include "position.h"
#include "search.h"
#include "tables.h"
#include "uci.h"

int xiphos_initialize()
{
  init_rook_c_flag_mask();
  init_bitboards();
  init_distance();
  init_phash();
  init_pst();
  init_lmr();

  uci();

  return 0;
}
