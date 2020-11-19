
// see.h

#ifndef FRUIT_SEE_H
#define FRUIT_SEE_H

// includes

#include "board.h"
#include "util.h"

// functions

extern int see_move   (int move, const board_t * board);
extern int see_square (const board_t * board, int to, int colour);

#endif // !defined SEE_H

// end of see.h

