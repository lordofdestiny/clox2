from ast import NodeTransformer, NodeVisitor, Name, BinOp, AST, Attribute
from ..types import term


class OnlyNameNtermExpression(NodeTransformer):
    def __init__(self, termset: set[str] = None, reverse=False):
        self.termset = termset
        self.reverse = reverse

    def visit(self, node):
        if isinstance(node, Name):
            return self.visit_Name(node)

        if isinstance(node, BinOp):
            return self.visit_BinOp(node)

        return False

    def visit_Name(self, node):
        if self.termset is None:
            return True
        if self.reverse:
            return node.id not in self.termset
        return node.id in self.termset

    def visit_BinOp(self, node):
        if not isinstance(node, BinOp):
            return isinstance(node, Name)

        left = self.visit(node.left)
        right = self.visit(node.right)

        return left and right


def is_nterm_variant_nterm(node: AST, reverse=False):
    return OnlyNameNtermExpression(term.terms, reverse).visit(node)


class NamesetCollector(NodeVisitor):
    def __init__(self):
        self.names = set()

    @classmethod
    def collect(cls, node: AST) -> set[str]:
        vst = cls()
        vst.visit(node)
        return vst.names

    def visit_Assign(self, node):
        if not isinstance(node.targets[0], Attribute):
            self.generic_visit(node)

    def visit_AnnAssign(self, node):
        self.visit(node.target)
        if node.value is not None:
            self.visit(node.value)

    def visit_Name(self, node):
        self.names.add(node.id)


def collect_nterm_names(node: AST) -> set[str]:
    return NamesetCollector.collect(node)
