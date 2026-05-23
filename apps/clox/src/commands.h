#ifndef __CLOX2_COMMANDS_H__
#define __CLOX2_COMMANDS_H__

#include <args.h>

int repl();

int runFileCommand(const Command* cmd);

int compileFileCommand(const Command* cmd);

#endif // __CLOX2_COMMANDS_H__
