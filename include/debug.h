#ifndef DEBUG_H
#define DEBUG_H

#ifdef ESPDEBUG
#define DEBUG(...) do {ets_uart_printf("DEBUG: " __VA_ARGS__); ets_uart_printf("\n");} while(0)
#else
#define DEBUG(...)
#endif

#endif
