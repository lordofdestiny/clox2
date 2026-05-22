from typing import Optional


class term:
    terms = {}

    def __init__(self, name: str):
        if name in term.terms:
            raise Exception(f"Terminal '{name}' already exists")

        self.name = name

        term.terms[name] = self

    def __repr__(self):
        return f"token('{self.name!r}')"


class nterm:
    nterms = {}

    def __init__(self, name: str, type: str):
        if name in nterm.nterms:
            raise Exception(f"Nonterminal '{name}' already exists")

        self.name = name
        self.type = type

        self.nterms[name] = self

    def __repr__(self):
        return f"nterm(name={self.name!r})"


class nterm_list(nterm):
    def __init__(self, name: str, type: str):
        super().__init__(name, type)

    def __repr__(self):
        return f"nterm_list(name={self.name!r}, type={self.type!r})"


class rule:
    def __init__(
        self, sequence: list[term | nterm], name: Optional[str] = None
    ):
        self.sequence = sequence
        self.name = name

    def __repr__(self):
        return f"rule(name={self.name!r}, sequence={self.sequence!r})"


class rule_set:
    def __init__(self, rules: list[rule]):
        self.rules = rules

    def __repr__(self):
        return f"rule_set(rules={self.rules!r})"
