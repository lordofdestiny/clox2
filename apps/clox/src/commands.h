#ifndef __CLOX2_COMMANDS_H__
#define __CLOX2_COMMANDS_H__

#include <args.h>

int repl();

int runFile(const Command* cmd);

int compileFile(const Command* cmd);

#endif // __CLOX2_COMMANDS_H__
