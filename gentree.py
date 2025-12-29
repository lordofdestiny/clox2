from dataclasses import dataclass, field
from pprint import pprint

keywords = ["and",
"as",
"break",
"case",
"catch",
"class",
"continue",
"default",
"else",
"false",
"for",
"func",
"finally",
"if",
"nil",
"or",
"print",
"return",
"static",
"super",
"switch",
"this",
"throw",
"true",
"try",
"var",
"while"
]

@dataclass
class Index:
    elements: list['IndexElement'] = field(default_factory=list)        
    value: str = None

    def insert(self, value: str, start = 0):
        if start >= len(value):
            self.value = value
            return
        
        first = value[start]
        if not self.contains(first, 0):
            ie = IndexElement(first, Index())
            self.elements.append(ie)
        
        for elem in self.elements:
            if elem.char == first:
                elem.insert(value, start+1)
                break
    
    def contains(self, value: str, start = 0):
        if start >= len(value):
            return True
        
        return any(elem.contains(value, start) for elem in self.elements)

    def __contains__(self, value: str, start = 0):
        return self.contains(value, start)
    
    def __len__(self):
        return sum(len(elem) for elem in self.elements)
    
    def __iter__(self):
        if self.value is not None:
            yield self.value
        else:
            yield from (ix for elem in self.elements for ix in elem)

    def render(self, depth=0):
        if len(self.elements) > 1:
            print(f'switch (scanner.start[{depth}]) {{')
            for elem in self.elements:
                elem.render(depth + 1)
            print("\tdefault: ;")
            print("}")
        elif len(self.elements) == 1:
            pass
        else:
            raise Exception("Unexpected state")


@dataclass
class IndexElement:
    char: str
    index: 'Index' = field(default_factory=Index)

    def insert(self, value: str, start = 0):
        self.index.insert(value, start)

    def contains(self, value: str, start = 0):
        if start < len(value) and value[start] == self.char:
            return self.index.contains(value, start + 1)

        return False

    def __contains__(self, value: str, start = 0):
        return self.contains(value, start)
    
    def __len__(self):
        if len(self.index) == 0:
            return 1

        else:
            return len(self.index)
        
    def __iter__(self):
        yield from self.index

    def render(self, depth=0):
        print(f'{'\t'*depth}case \'{self.char}\':')
        if len(index.elements) > 1:
            print(f'{'\t'*(depth+1)}if (scanner.current - scanner.start > {depth}) {{')
            index.render(depth + 1)
            print(f'{'\t'*(depth+1)}}}')
        else:
            word = "??????"
            print(f'return checkKeyword({depth + 1}, {-1}, {word[depth+1]}, TOKEN_??????)')
        
def build(words):
    index = Index()
    for kw in sorted(words):
        index.insert(kw)

    return index

def write_code(index: Index):
    print("static TokenType indentifierType() {")
    index.render()
    print('\treturn TOKEN_IDENTIFIER;')
    print("}")


index = build(keywords)

write_code(index)