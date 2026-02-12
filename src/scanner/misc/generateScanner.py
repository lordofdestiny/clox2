from abc import ABC, abstractmethod
from collections import ChainMap, defaultdict
from contextlib import contextmanager, AbstractContextManager, redirect_stdout
from dataclasses import dataclass, field
from enum import auto, Enum
from pathlib import Path
from typing import TextIO, Optional, TypedDict, NotRequired, Protocol
import argparse
import os
import sys
import tomllib


@dataclass
class ProgramArgs:
    config_file: str
    output_header: str
    output_header_decls: str
    output_source: str
    workdir: str


def parseArgs() -> ProgramArgs:
    parser = argparse.ArgumentParser(
        prog="generateScanner",
        description="Generates the boilerplate functions for the clox scanner",
    )
    parser.add_argument("config_file", help="Scanner configuration file")
    parser.add_argument(
        "-D",
        "--output-header-decls",
        action="store",
        help="Output file to write the type declarations to",
        metavar="decl_header_file",
        required=True,
    )
    parser.add_argument(
        "-H",
        "--output-header",
        action="store",
        help="Output file to write the function declarations to",
        metavar="header_file",
        required=True,
    )
    parser.add_argument(
        "-S",
        "--output-source",
        action="store",
        help="Output file to write the source code to",
        metavar="source_file",
        required=True,
    )
    parser.add_argument(
        "-C", "--workdir", action="store", metavar=dir, required=False
    )
    return parser.parse_args()


class GeneratorConfig(TypedDict):
    tokenPrefix: NotRequired[str]
    identTokenName: str
    tokenType: NotRequired[str]
    tokenEnumType: NotRequired[str]
    makeTokenFunctionName: str
    errorTokenFunctionName: str
    checkKeywordFunctionName: str
    charTokenFunctionName: str
    identFunctionName: str
    symbolFunctionName: str


class IncludeLists(TypedDict):
    header_file: NotRequired[list[str]]
    header_decls_file: NotRequired[list[str]]
    source_file: NotRequired[list[str]]


class TokensConfig(TypedDict):
    keywords: list[str]
    specials: list[str]
    literals: list[str]
    symbols: dict[str, str]


class GeneratorSpec(TypedDict):
    settings: GeneratorConfig
    includes: IncludeLists
    tokens: TokensConfig


class TokenStringCallback(Protocol):
    def __call__(self, word: str, token: Optional[str]) -> str: ...


def read_config(filename: str) -> GeneratorSpec:
    with open(filename, "rb") as f:
        return GeneratorSpec(tomllib.load(f))


def load_config(filename: str) -> GeneratorSpec:
    config = read_config(filename)

    for keyword in config["tokens"]["keywords"]:
        if len(keyword) <= 0:
            raise ConfigError(f"Keyword can't be empty")

    for name, token in config["tokens"]["symbols"].items():
        if len(token) <= 0:
            raise ConfigError(f"Symbol token '{name}' can't be empty")
        if len(token) > 2:
            raise ConfigError(
                f"Symbol token be at most two characters long '{token}' => '{name}')"
            )

    ident_token = config["settings"]["identTokenName"]

    if ident_token not in config["tokens"]["literals"]:
        raise ConfigError(
            f"settings.identTokenName ({ident_token}) must be listed in tokens.literals"
        )

    return config


class TabIndentation(AbstractContextManager):
    _index: dict[str, "TabIndentation"] = {}

    def __init__(self, tabs=0):
        self.fn_write = None
        self.newline = True
        self.tabs = tabs

    def __enter__(self):
        self.fn_write = sys.stdout.write
        sys.stdout.write = self.write
        return self

    def write(self, *args, **kwargs):
        if args[0] == "\n":
            self.newline = True
        elif self.newline:
            self.fn_write("\t" * self.tabs)
            self.newline = False
        self.fn_write(*args, *kwargs)

    @contextmanager
    def indent(self, by=1):
        self.tabs += by
        yield
        self.tabs -= by

    def __exit__(self, *exc):
        sys.stdout.write = self.fn_write


@dataclass
class Index:
    elems: list["IndexNode"] = field(default_factory=list)
    value: Optional[str] = None
    token: Optional[str] = None

    def insert(self, value: str, token: Optional[str] = None, start=0):
        if start >= len(value):
            return

        first = value[start]
        if not self.contains(first, 0):
            ie = IndexNode(first, Index())
            self.elems.append(ie)

        for elem in self.elems:
            if elem.char == first:
                elem.insert(value, token, start + 1)
                break

    def contains(self, value: str, start=0):
        if start >= len(value):
            return True

        return any(elem.contains(value, start) for elem in self.elems)

    def extract(self) -> Optional[tuple[str, str]]:
        if len(self.elems) == 0:
            return self.value, self.token

        if len(self.elems) == 1 and self.value is None:
            return self.elems[0].index.extract()

        return None


@dataclass
class IndexNode:
    char: str
    index: "Index" = field(default_factory=Index)

    def insert(self, value: str, token: Optional[str] = None, start=0):
        self.index.insert(value, token, start)
        if start == len(value):
            self.index.value = value
            self.index.token = token

    def contains(self, value: str, start=0):
        if start < len(value) and value[start] == self.char:
            return self.index.contains(value, start + 1)

        return False


@dataclass
class IndexCodeGenerator(ABC):
    gen: "CodeGenerator"
    index: Index
    tabs: TabIndentation

    @abstractmethod
    def generate(self):
        pass

    @abstractmethod
    def handle_index(self, index: Index, depth: int):
        pass

    @abstractmethod
    def handle_node(self, node: IndexNode, depth: int):
        pass


@dataclass
class GenerateIdentifierCode(IndexCodeGenerator):
    def generate(self, function_name: str):
        settings = self.gen.spec["settings"]
        returnType = settings["tokenEnumType"]
        indentTokenName = settings["identTokenName"]

        ident_token = self.gen.make_token(indentTokenName, TokenClass.SKIP)

        print(f"{returnType} {function_name}(Scanner* scanner) {{")
        with self.tabs.indent():
            self.handle_index(self.index, 0)
            print(f"return {ident_token};")
        print("}")

    def handle_index(self, index: Index, depth: int):
        print(f"switch (scanner->start[{depth}]) {{")
        for elem in index.elems:
            print(f"case '{elem.char}':", end="")
            with self.tabs.indent():
                self.handle_node(elem, depth)
        print("default: ;")
        print("}")

    def handle_node(self, node: IndexNode, depth: int):
        if len(node.index.elems) >= 1 and not node.index.extract():
            print()
            print(f"if (scanner->current - scanner->start > {depth + 1}) {{")
            with self.tabs.indent():
                self.handle_index(node.index, depth + 1)
            print("}")
            print("break;")
        else:
            word, *_ = node.index.extract()
            curr = depth + 1
            rem = len(word) - curr
            suffix = word[curr:]
            full_token = self.gen.make_token(word, TokenClass.KEYWORD)
            args = f'scanner, {curr}, {rem}, "{suffix}", {full_token}'
            checkKeywordFun = self.gen.spec["settings"][
                "checkKeywordFunctionName"
            ]
            print(f" return {checkKeywordFun}({args});")


@dataclass
class GenerateSymbolCode(IndexCodeGenerator):
    def generate(self, function_name: str):
        settings = self.gen.spec["settings"]
        errorTokenFun = settings["errorTokenFunctionName"]
        returnType = settings["tokenType"]

        print(f"{returnType} {function_name}(Scanner* scanner, char c) {{")
        with self.tabs.indent():
            self.handle_index(self.index, 0)
            print()
            print(f'return {errorTokenFun}(scanner, "Unexpected character.");')
        print("}")

    def handle_index(self, index: Index, depth: int):
        print("switch (c) {")
        for elem in index.elems:
            print(f"case '{elem.char}':", end="")
            self.handle_node(elem, depth + 1)
        print("default: break;")
        print("}")

    def make_token(self, index: Index):
        if len(index.value) == 1:
            return self.gen.make_token(index.token, TokenClass.ONE_CHAR)
        else:
            return self.gen.make_token(index.token, TokenClass.TWO_CHAR)

    def handle_node(self, node: IndexNode, *_):
        if len(node.index.elems) != 0:
            print(" {")

        makeTokenFun = self.gen.spec["settings"]["makeTokenFunctionName"]

        with self.tabs.indent():
            for elem in node.index.elems:
                token = self.make_token(elem.index)
                print(
                    f"if (match(scanner, '{elem.char}')) return {makeTokenFun}(scanner, {token});"
                )

            token = self.make_token(node.index)
            if len(node.index.elems) == 0:
                print(" ", end="")

            print(f"return {makeTokenFun}(scanner, {token});")

        if len(node.index.elems) != 0:
            print("}")


class ConfigError(Exception):
    """Exception raised for invalid scanner TOML configuration."""


@dataclass(frozen=True)
class TokenClassData:
    id: int
    comment: str


class TokenClass(TokenClassData, Enum):
    SKIP = auto(), None
    ONE_CHAR = auto(), "Single character tokens"
    TWO_CHAR = auto(), "Two character tokens"
    LITERAL = auto(), "Value literal tokens"
    KEYWORD = auto(), "Keyword tokens"
    SPECIAL = auto(), "Special tokens"


class CodeGenerator:
    spec: GeneratorSpec
    keywords: Index
    symbols: Index
    tokens: defaultdict[TokenClass, list[str]]

    def __init__(self, spec: GeneratorSpec):
        self.spec = ChainMap(
            spec,
            {
                "settings": {
                    "tokenPrefix": "TOKEN",
                    "tokenType": "Token",
                    "tokenEnumType": "TokenType",
                },
                "includes": {
                    "header_file": [],
                    "header_decls_file": [],
                    "source_file": [],
                },
            },
        )

        tokens = self.spec["tokens"]
        self.keywords = self.make_keyword_index(tokens["keywords"])
        self.symbols = self.make_symbols_index(tokens["symbols"])
        self.tokens = defaultdict(list)

    def make_token(self, name: str, token_class: TokenClass) -> str:
        token = f"{self.spec['settings']['tokenPrefix']}_{name.upper()}"
        self.tokens[token_class].append(token)
        return token

    @staticmethod
    def make_keyword_index(keywords: list[str]):
        index = Index()
        for word in keywords:
            index.insert(word, word)
        return index

    @staticmethod
    def make_symbols_index(symbols: dict[str, str]):
        index = Index()
        for token, value in symbols.items():
            index.insert(value, token)
        return index

    def generate_enum_group(self, token_class: TokenClass):
        tokens = self.tokens[token_class]

        count = len(tokens)
        for i, tok in enumerate(tokens):
            tok = tokens[i]
            print(tok, end="")
            if i != count - 1:
                print(", ", end="")
            if i % 4 == 3:
                print()

    def generate_enum(self, ti: TabIndentation):
        print("typedef enum {")
        with ti.indent():
            count = len(TokenClass)
            for i, tk in enumerate(TokenClass):
                if tk.comment is None:
                    continue

                print(f"// {tk.comment}")
                self.generate_enum_group(tk)
                if i != count - 1:
                    print(",\n")
        print()
        print(f"}} {self.spec['settings']['tokenEnumType']};")

    def generate_includes(self, headers: list[str]):
        for header in headers:
            if len(header) > 0:
                print(f"#include {header}")

    def generate_source(self, file=TextIO):
        settings = self.spec["settings"]
        with redirect_stdout(file):
            with TabIndentation() as ti:
                self.generate_includes(self.spec["includes"]["source_file"])
                print()
                GenerateIdentifierCode(
                    self,
                    self.keywords,
                    ti,
                ).generate(settings["identFunctionName"])
                print()
                GenerateSymbolCode(self, self.symbols, ti).generate(
                    settings["symbolFunctionName"]
                )

    def generate_header(self, file=TextIO):
        with redirect_stdout(file):
            with TabIndentation() as ti:
                settings = self.spec["settings"]
                print("#ifndef __CLOX2_SCANNER_GENERATED_H__")
                print("#define __CLOX2_SCANNER_GENERATED_H__")
                print()
                self.generate_includes(self.spec["includes"]["header_file"])
                print()
                print(
                    f"{settings['tokenEnumType']} {settings["identFunctionName"]}(Scanner* scanner);"
                )
                print(
                    f"{settings['tokenType']} {settings['charTokenFunctionName']}(Scanner* scanner, char c);"
                )
                print()
                print("#endif // __CLOX2_SCANNER_GENERATED_H__")

    def generate_header_decls(self, file=TextIO):
        tokens = self.spec["tokens"]
        with redirect_stdout(file):
            with TabIndentation() as ti:
                for tok in tokens["literals"]:
                    self.make_token(tok, TokenClass.LITERAL)
                for tok in tokens["specials"]:
                    self.make_token(tok, TokenClass.SPECIAL)
                print("#ifndef __CLOX2_SCANNER_GENERATED_DECLS_H__")
                print("#define __CLOX2_SCANNER_GENERATED_DECLS_H__")
                print()
                self.generate_enum(ti)
                print()
                print("#endif // __CLOX2_SCANNER_GENERATED_DECLS_H__")


if __name__ == "__main__":
    args = parseArgs()

    # Change working directory
    target_dir = None
    if args.workdir is None:
        target_dir = Path.cwd()
    else:
        target_dir = Path(args.workdir).resolve()

    if not target_dir.is_dir():
        print(f"Error: Directory not found at '{target_dir}'")
        exit(1)

    os.chdir(target_dir)

    try:
        cfg = load_config(args.config_file)
        gen = CodeGenerator(cfg)

        Path(args.output_source).parent.mkdir(parents=True, exist_ok=True)
        with open(args.output_source, "w") as f:
            gen.generate_source(f)

        Path(args.output_header_decls).parent.mkdir(parents=True, exist_ok=True)
        with open(args.output_header_decls, "w") as f:
            gen.generate_header_decls(f)

        Path(args.output_header).parent.mkdir(parents=True, exist_ok=True)
        with open(args.output_header, "w") as f:
            gen.generate_header(f)

    except FileNotFoundError as fnfe:
        print(f"Could not open '{fnfe.filename}'")
