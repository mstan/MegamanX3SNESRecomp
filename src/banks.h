#ifndef BANKS_H
#define BANKS_H

#include "types.h"

/* v2 ABI: every recompiled function takes `CpuState *cpu` and mutates
 * register / flag state in place. No aggregate return typedefs, no
 * RECOMP_BANK<BB> macros — v2 always emits all functions in a bank. */

#endif /* BANKS_H */
