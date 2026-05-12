import argparse

import ast
from ast import NodeTransformer, NodeVisitor
from ast import Name, Constant, Subscript, Attribute, AST

import __main__
from pathlib import Path

MY_NAME = Path(__main__.__file__).stem


def term_type(typename):
    global __term_typename
    __term_typename = typename


class term:
    terms = {}

    def __init__(self, name):
        if name in term.terms:
            raise Exception(f"Terminal '{name}' already exists")

        self.name = name

        term.terms[name] = self

    def __repr__(self):
        return f"token('{self.name}')"


class nterm:
    nterms = {}

    def __init__(self, name, typename):
        if name in nterm.nterms:
            raise Exception(f"Nonterminal '{name}' already exists")

        self.name = name
        self.typename = typename

        self.nterms[name] = self

    def __repr__(self):
        return f"nterm(name={self.name},ctype={self.typename})"


class nterm_list:
    def __init__(self, type: nterm):
        self.type = type

    def __repr__(self):
        return f"list[{self.type}]"


class NamesetCollector(NodeVisitor):
    def __init__(self):
        self.names = set()

    @classmethod
    def collect(cls, node: AST) -> set[str]:
        vst = cls()
        vst.visit(node)
        return vst.names

    def visit_Name(self, node):
        self.names.add(node.id)


class TransformTokens(NodeVisitor):
    settings = {"term": {"typename": "tok"}}

    def __init__(self, specfile):
        self.specfile = specfile
        self.found_import = False
        self.nameset = set()

    def visit_Import(self, node):
        if any(name == MY_NAME for name in node.names):
            self.found_import = True

    def visit_ImportFrom(self, node):
        if node.module == MY_NAME:
            self.found_import = True

    def visit_Assign(self, node):
        if len(node.targets) != 1:
            raise Exception(
                "Multiple assign targets forbidden in config statement"
            )

        if isinstance(node.targets[0], Attribute):
            setting_target = node.targets[0].value.id
            if setting_target not in self.settings:
                raise Exception(f"Cannot set sttings for '{setting_target}'")
            setting_name = node.targets[0].attr
            if setting_name not in self.settings[setting_target]:
                raise Exception(
                    f"Setting '{setting_target}.{setting_name}' not allowed"
                )
            if not isinstance(node.value, Constant):
                raise Exception(f"Only constants allowed as settign values")

            self.settings[setting_target][setting_name] = node.value

            return

        self.nameset |= NamesetCollector.collect(node)

        # For collecting do here

    def visit_AnnAssign(self, node):
        if isinstance(node.annotation, Name) and node.annotation.id == "term":
            term(node.target.id)
            return

        if isinstance(node.annotation, Name) and node.annotation.id == "list":
            nterm(node.target.id, f"{node.target.id}List")
            return

        self.nameset |= NamesetCollector.collect(node)
        return

        raise Exception(
            f"{self.specfile}:{node.lineno} Unrecognized declaration"
        )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(MY_NAME)
    parser.add_argument("specfile", action="store")
    args = parser.parse_args()

    with open(args.specfile, "r") as f:
        tree = ast.parse(f.read())

        # print(ast.dump(tree, indent=2))
        # exit(0)

        vis = TransformTokens(args.specfile)
        vis.visit(tree)

        if not vis.found_import:
            raise Exception(f"Import of '{MY_NAME}' not found")

        print(vis.nameset - set(term.terms))

        # print(term.terms)
        # print(nterm.nterms)
