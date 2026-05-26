from ast import NodeVisitor
from ast import Name, Constant, Subscript, Attribute, AST
from platform import node

from ..types import nterm, term, nterm_list


class TermNtermCollector(NodeVisitor):
    settings = {"term": {"typename": "tok"}}

    def __init__(self, specfile, nameset: set[str]):
        self.specfile = specfile
        self.nameset = nameset
        self.nterm_nameset = set()

    def traverse(self, node):
        self.visit(node)
        diff = self.nameset - self.nterm_nameset
        if len(diff) > 0:
            s = "\n".join(f"- {s}" for s in diff)
            raise Exception(f"Undefined names in {self.specfile}:\n{s}")

    def _try_modify_setting(self, node):
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

    def visit_Assign(self, node):
        if len(node.targets) != 1:
            raise Exception(
                "Multiple assign targets forbidden in config statement"
            )

        if isinstance(node.targets[0], Attribute):
            self._try_modify_setting(node)
            return

        nt = nterm(node.targets[0].id, node.targets[0].id)
        self.nterm_nameset.add(nt.name)

    def visit_AnnAssign(self, node):
        if isinstance(node.annotation, Name) and node.annotation.id != "term":
            raise Exception(f"Invalid annotation type for '{node.target.id}'")

        if (
            isinstance(node.annotation, Subscript)
            and node.annotation.value.id != "list"
        ):
            raise Exception("Only 'list' type annotations allowed")

        if isinstance(node.annotation, Name) and node.annotation.id == "term":
            tr = term(node.target.id)
            self.nameset.remove(tr.name)
            return

        if (
            isinstance(node.annotation, Subscript)
            and node.annotation.value.id == "list"
        ):
            nt = nterm_list(node.target.id, node.annotation.slice.id)
            self.nterm_nameset.add(nt.name)
            return

        nt = nterm(node.target.id)
        self.nterm_nameset.add(nt.name)
        return

        raise Exception(
            f"{self.specfile}:{node.lineno} Unrecognized declaration"
        )
