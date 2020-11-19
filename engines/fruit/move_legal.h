
// move_legal.h

#ifndef FRUIT_MOVE_LEGAL_H
#define FRUIT_MOVE_LEGAL_H

// includes

#include "board.h"
#include "list.h"
#include "util.h"

// functions

extern bool move_is_pseudo  (int move, board_t * board);
extern bool quiet_is_pseudo (int move, board_t * board);

extern bool pseudo_is_legal (int move, board_t * board);

#endif // !defined MOVE_LEGAL_H

// end of move_legal.h

