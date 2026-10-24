import argparse
from ast import parse as ast_parse, dump as ast_dump
from pprint import pprint

from metaox import term, nterm, TermNtermCollector, collect_names

from metaox.visitor.rules import CreateRules

if __name__ == "__main__":
    parser = argparse.ArgumentParser("metaox")
    parser.add_argument("specfile", action="store")
    parser.add_argument("--tree", "-t", action="store_true")
    args = parser.parse_args()

    with open(args.specfile, "r") as f:
        tree = ast_parse(f.read())

    if args.tree:
        print(ast_dump(tree, indent=2))
        exit(0)

    nameset = collect_names(tree)

    TermNtermCollector(args.specfile, nameset).traverse(tree)

    CreateRules().traverse(tree)
