import graphlib
import itertools
import collections

from ast import BinOp, Name, NodeVisitor, Attribute, Tuple
from ..types import rule, rule_set, sub_rule, term, nterm


def inline_lists(subrules):
    for rule in subrules:
        if isinstance(rule, list):
            yield from rule
        else:
            yield rule


def tarjan(graph):
    index = 0
    stack = []
    indices = {}
    lowlink = {}
    components = collections.defaultdict(set)

    def strong_connect(v):
        nonlocal index
        indices[v] = index
        lowlink[v] = index
        index += 1
        stack.append(v)

        for w in graph[v]:
            if w not in nterm.nterms:
                continue

            if not w in indices:
                strong_connect(w)
                lowlink[v] = min(lowlink[v], lowlink[w])
            elif w in stack:
                lowlink[v] = min(lowlink[v], indices[w])

        if lowlink[v] == indices[v]:
            while stack:
                w = stack.pop()
                components[indices[v]].add(w)
                if w == v:
                    break

    for v in set(graph.keys()) - set(term.terms.keys()):
        if v not in indices:
            strong_connect(v)

    return components


import collections


def construct_metagraph(graph, components):
    # Map each individual node to its corresponding component ID
    node_to_comp = {}
    for comp_id, nodes in components.items():
        for node in nodes:
            node_to_comp[node] = comp_id

    metagraph = collections.defaultdict(set)

    for u, children in graph.items():
        comp_u = node_to_comp.get(u)
        if comp_u is None:
            continue

        for v in children:
            comp_v = node_to_comp.get(v)
            # Add an edge between different components, ignoring internal cycles
            if comp_v is not None and comp_u != comp_v:
                metagraph[comp_u].add(comp_v)

    # Convert sets back to lists for a clean adjacency list representation
    return {
        comp_id: list(neighbors) for comp_id, neighbors in metagraph.items()
    }


class CreateRules(NodeVisitor):
    def __init__(self):
        super().__init__()
        self.ruleset = {}
        self.ruleset_names = {}
        self.ruleset_graph = {}

    def traverse(self, node):
        self.visit(node)

        self._create_rest()
        print("Ruleset size: ", len(self.ruleset))

    def _resolve_term_nterm(self, name: str) -> bool:
        obj = term.get(name)
        if obj:
            return obj

        obj = nterm.get(name)
        if obj:
            return obj

        raise Exception(f"Undefined terminal or non-terminal '{name}'")

    def _resolve_all_names(
        self, names: list[list[str]]
    ) -> list[list[term | nterm]]:
        return [
            [self._resolve_term_nterm(name) for name in sublist]
            for sublist in names
        ]

    def _create_rest(self):
        result = tarjan(self.ruleset_graph)
        metagraph = construct_metagraph(self.ruleset_graph, result)

        graph = graphlib.TopologicalSorter(metagraph)
        for nterms in map(lambda x: result[x], graph.static_order()):
            if len(nterms) > 1:
                for cnterm in nterms:
                    todo = (
                        rule
                        for rule in self.ruleset_graph[cnterm]
                        if rule not in self.ruleset and rule not in term.terms
                    )
                    print(cnterm, list(todo))
                continue

            cnterm = nterms.pop()
            rules = self._resolve_all_names(self.ruleset_names[cnterm])
            if len(rules) < 2:
                self.ruleset[cnterm] = rule(rules[0], cnterm)
            else:
                rules = [sub_rule(rrule, cnterm) for rrule in rules]
                self.ruleset[cnterm] = rule_set(rules, cnterm)

    def _is_term(self, name: str) -> bool:
        return name in term.terms

    def _is_nterm(self, name: str) -> bool:
        return name in nterm.nterms

    @classmethod
    def _flatten_binop(cls, node) -> list[list[str] | str]:
        if isinstance(node, BinOp):
            return [
                *cls._flatten_binop(node.left),
                *cls._flatten_binop(node.right),
            ]
        elif isinstance(node, Name):
            return [[node.id]]
        elif isinstance(node, Tuple):
            return [
                list(
                    inline_lists(
                        [cls._flatten_binop(elt)[0] for elt in node.elts]
                    )
                )
            ]
        else:
            raise Exception(f"Unhandled node type {node}")

    def _create_rule_set(self, node):
        rule_name = node.targets[0].id
        subrules = self._flatten_binop(node.value)
        self.ruleset_names[rule_name] = subrules
        self.ruleset_graph[rule_name] = set(
            filter(lambda x: x != rule_name, inline_lists(subrules))
        )

    def _create_rule(self, node):
        rule_name = node.targets[0].id
        rule_deps = None
        if isinstance(node.value, Name):
            rule_deps = [node.value.id]
        elif isinstance(node.value, Tuple):
            rule_deps = [elt.id for elt in node.value.elts]
        else:
            raise Exception(f"Unhandled rule type {node.value}")

        if rule_name in self.ruleset_names:
            raise Exception(f"Rule '{rule_name}' already exists")

        if rule_name not in nterm.nterms and rule_name not in term.terms:
            raise Exception(
                f"Rule '{rule_name}' references undefined terminal or non-terminal"
            )

        self.ruleset_names[rule_name] = [rule_deps]
        self.ruleset_graph[rule_name] = set(rule_deps)

    def visit_Assign(self, node):
        if node.value is None:
            return

        if isinstance(node.targets[0], Attribute):
            return

        if isinstance(node.value, BinOp):
            self._create_rule_set(node)
            return

        if isinstance(node.value, Tuple):
            self._create_rule(node)
            return

        if isinstance(node.value, Name):
            self._create_rule(node)
            return

        raise Exception(f"Unhandled assignment type {node}")

    def visit_AnnAssign(self, node):
        if node.value is None:
            return

        rule_name = node.target.id
        rule_value_type = node.annotation.slice.id
        subrules = self._flatten_binop(node.value)

        if len(subrules) < 2:
            raise Exception(
                f"Rule '{rule_name}' must have at least two subrules"
            )

        for rule in subrules:
            if rule_value_type in rule:
                break
        else:
            raise Exception(f"Rule '{rule_name}' is missing a base case")

        recursive_rules = []
        for rule in subrules:
            if isinstance(rule, list) and rule_value_type in rule:
                recursive_rules.append(rule)

        if not recursive_rules:
            raise Exception(f"Rule '{rule_name}' is missing a recursive case")

        if not any(rule_value_type in rule for rule in recursive_rules):
            raise Exception(
                f"Rule '{rule_name}' is missing a terminating recursive case"
            )

        self.ruleset_names[rule_name] = subrules
        self.ruleset_graph[rule_name] = set(
            filter(lambda x: x != rule_name, inline_lists(subrules))
        )
