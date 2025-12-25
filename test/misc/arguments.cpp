#include <gtest/gtest.h>

#include <ostream>
#include <vector>
#include <tuple>
#include <string>
#include <cstring>

#include "clox/args.h"

struct MainArgs{
    int argc;
    char** argv;

    MainArgs(std::vector<std::string> const& args) {
        argc = static_cast<int>(args.size());
        argv = new char*[argc + 1];

        for (int i = 0; i < argc; ++i) {
            argv[i] = new char[args[i].size() + 1];
            std::strcpy(argv[i], args[i].c_str());
            argv[i][args[i].size()] = '\0';
        }

        // Null-terminate the argv array
        argv[argc] = nullptr;
    }

    ~MainArgs() {
        for (int i = 0; i < argc; ++i) {
            delete[] argv[i];
        }
        delete[] argv;
    }
};

class ArgumentParsingMultipleParametersTest 
    : public ::testing::TestWithParam<std::tuple<
        std::string,                     // Test name
        std::vector<std::string>,        // Input arguments
        Command                         // Expected Command struct
    >> {
};

TEST_P(ArgumentParsingMultipleParametersTest, ParseArguments) {
    auto& [test_name, args, expected] = GetParam();
    MainArgs mainArgs(args);

    Command result = parseArgs(mainArgs.argc, mainArgs.argv);
    EXPECT_STREQ(result.input_file, expected.input_file);
    EXPECT_STREQ(result.output_file, expected.output_file);
    EXPECT_EQ(result.input_type, expected.input_type);
    EXPECT_EQ(result.output_type, expected.output_type);
    EXPECT_EQ(result.inline_code, expected.inline_code);
    EXPECT_EQ(result.type, expected.type);
}

std::ostream& operator<<(std::ostream& os, const Command& cmd) {
    os << "Command(input_file=" << (cmd.input_file ? cmd.input_file : "nullptr")
       << ", output_file=" << (cmd.output_file ? cmd.output_file : "nullptr")
       << ", input_type=" << cmd.input_type
       << ", output_type=" << cmd.output_type
       << ", inline_code=" << cmd.inline_code
       << ", type=" << cmd.type
       << ")";
    return os;
}

INSTANTIATE_TEST_SUITE_P(
    ArgumentParsingTests,
    ArgumentParsingMultipleParametersTest,
    testing::Values(
        std::make_tuple(
            "REPLMode",
            std::vector<std::string>{"./clox"},
            Command{
                .input_file = nullptr,
                .output_file = nullptr,
                .input_type = Command::CMD_EXEC_UNSET,
                .output_type = Command::CMD_COMPILE_UNSET,
                .inline_code = false,
                .type = CMD_REPL
            }
        ),
        std::make_tuple(
            "ExecuteSourceFile",
            std::vector<std::string>{"./clox", "input.lox"},
            Command{
                .input_file = "input.lox",
                .output_file = nullptr,
                .input_type = Command::CMD_EXEC_SOURCE,
                .output_type = Command::CMD_COMPILE_UNSET,
                .inline_code = false,
                .type = CMD_EXECUTE
            }
        ),
        std::make_tuple(
            "ExecuteBinaryFileWith_xb",
            std::vector<std::string>{"./clox", "-xb", "input.lox.bin"},
            Command{
                .input_file = "input.lox.bin",
                .output_file = nullptr,
                .input_type = Command::CMD_EXEC_BINARY,
                .output_type = Command::CMD_COMPILE_UNSET,
                .inline_code = false,
                .type = CMD_EXECUTE
            }
        ),
        std::make_tuple(
            "CompileSourcetoBinaryWith_c_o",
            std::vector<std::string>{"./clox", "-c", "-o", "output.lox.bin", "input.lox"},
            Command{
                .input_file = "input.lox",
                .output_file = "output.lox.bin",
                .input_type = Command::CMD_EXEC_SOURCE,
                .output_type = Command::CMD_COMPILE_BINARY,
                .inline_code = false,
                .type = CMD_COMPILE
            }
        ),
        std::make_tuple(
            "CompileInlineSourceWith_is",
            std::vector<std::string>{"./clox", "-is", "input.lox"},
            Command{
                .input_file = "input.lox",
                .output_file = nullptr,
                .input_type = Command::CMD_EXEC_SOURCE,
                .output_type = Command::CMD_COMPILE_BYTECODE,
                .inline_code = true,
                .type = CMD_DISASSEMBLE
            }
        ),
        std::make_tuple(
            "DisassembleBinarytoBytecodeWith_sbi",
            std::vector<std::string>{"./clox", "-sbi", "input.lox.bin", "-o", "input.lox.s"},
            Command{
                .input_file = "input.lox.bin",
                .output_file = "input.lox.s",
                .input_type = Command::CMD_EXEC_BINARY,
                .output_type = Command::CMD_COMPILE_BYTECODE,
                .inline_code = true,
                .type = CMD_DISASSEMBLE
            }
        )
    ),
    [&](const auto& info) {
        return std::get<0>(info.param);
    }
);