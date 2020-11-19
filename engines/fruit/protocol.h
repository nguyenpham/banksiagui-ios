
// protocol.h

#ifndef FRUIT_PROTOCOL_H
#define FRUIT_PROTOCOL_H

// includes

#include "util.h"

// functions

extern void loop  ();
//extern void event ();

extern void get   (char string[], int size);
extern void send  (const char format[], ...);

#endif // !defined PROTOCOL_H

// end of protocol.h

