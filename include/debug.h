#ifndef DEBUG_H
#define DEBUG_H

#ifdef ESPDEBUG
#define DEBUG(x) ets_uart_printf("DEBUG: %s\n", (x))
#else
#define DEBUG(X)
#endif

#endif
