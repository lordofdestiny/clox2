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

    @classmethod
    def get(cls, name: str):
        return cls.terms.get(name, None)


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

    @classmethod
    def get(cls, name: str):
        return cls.nterms.get(name, None)


class nterm_list(nterm):
    def __init__(self, name: str, type: str):
        super().__init__(name, type)

    def __repr__(self):
        return f"nterm_list(name={self.name!r}, type={self.type!r})"


class rule:
    def __init__(self, sequence: list[term | nterm], non_terminal: nterm):
        if any(not isinstance(elt, (term, nterm)) for elt in sequence):
            raise Exception(f"Invalid rule sequence {sequence}")
        self.sequence = sequence
        self.nterm = non_terminal

    def __repr__(self):
        return f"rule(name={self.nterm!r}, sequence={self.sequence!r})"


class sub_rule:
    def __init__(self, sequence: list[term | nterm], non_terminal: nterm):
        self.sequence = sequence
        self.nterm = non_terminal

    def __repr__(self):
        return f"sub_rule(name={self.nterm!r}, sequence={self.sequence!r})"
    

class rule_set:
    def __init__(self, rules: list[rule], non_terminal: nterm):
        self.rules = rules
        self.nterm = non_terminal

    def __repr__(self):
        return f"rule_set(name={self.nterm!r}, rules={self.rules!r})"
