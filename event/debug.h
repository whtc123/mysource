#ifndef __DEBUG_H_
#define __DEBUG_H_



#define COLOR_NONE         "\033[m" 
#define COLOR_RED          "\033[0;32;31m" 
#define COLOR_LIGHT_RED    "\033[1;31m" 
#define COLOR_GREEN        "\033[0;32;32m" 
#define COLOR_LIGHT_GREEN  "\033[1;32m" 
#define COLOR_BLUE         "\033[0;32;34m" 
#define COLOR_LIGHT_BLUE   "\033[1;34m" 
#define COLOR_DARY_GRAY    "\033[1;30m" 
#define COLOR_CYAN         "\033[0;36m" 
#define COLOR_LIGHT_CYAN   "\033[1;36m" 
#define COLOR_PURPLE       "\033[0;35m" 
#define COLOR_LIGHT_PURPLE "\033[1;35m" 
#define COLOR_BROWN        "\033[0;33m" 
#define COLOR_YELLOW       "\033[1;33m" 
#define COLOR_LIGHT_GRAY   "\033[0;37m" 
#define COLOR_WHITE        "\033[1;37m"

/*#ifdef DEBUG*/
#if 1
#define TraceNorm(str...)   printf(             "[normal]" str)
#define TraceInfo(str...)   printf(COLOR_GREEN  "[info  ]" COLOR_NONE str)
#define TraceImport(str...) printf(COLOR_PURPLE "[import]" COLOR_NONE str)
#define TraceWarn(str...)   printf(COLOR_BLUE   "[warn  ]" COLOR_NONE str)
#define TraceErr(str...)    printf(COLOR_RED    "[error ]" COLOR_NONE str)
#define Cls()               printf("\x1B\x5B\x48\x1B\x5B\x4A");
#else
#define TraceNorm(str...)
#define TraceInfo(str...)
#define TraceImport(str...)
#define TraceWarn(str...)
#define TraceErr(str...)
#define Cls()
#endif


#endif

