import markdown, markdown.extensions.fenced_code
import os
import re
import urllib.parse as parse, urllib.request as req
import textwrap

from argparse import ArgumentParser, Namespace, Action
from dataclasses import dataclass
from pathlib import Path
from string import Template
from typing import Type, Optional, overload


class ValidateFile(Action):
    def __call__(self, parser, namespace, values, option_string=None):
        file = Path(values)
        if not file.is_file():
            parser.error(
                f"Enter a valid file. Got: {option_string} {file.resolve()}"
            )
        setattr(namespace, self.dest, file)


def make_parser():
    parser = ArgumentParser("./generate")
    parser.add_argument(
        "-o",
        "--out-dir",
        dest="outdir",
        action="store",
        default="docs",
    )
    parser.add_argument(
        "-g",
        "--gramar",
        dest="grammar_file",
        action=ValidateFile,
        required=True,
    )
    return parser


@overload
def read_file(filename: Path, binary=True) -> bytes: ...
@overload
def read_file(filename: Path, binary=False) -> str: ...
def read_file(filename: Path, binary: True | False) -> bytes | str:
    mode = "rb" if binary else "r"
    with open(filename, mode) as f:
        return f.read()


@dataclass
class Args(Namespace, Type):
    outdir: Path = None
    grammar_file: Optional[Path] = None


class Generator:
    args: Args
    markdown_cache: str

    def __init__(self, args: Args):
        self.args = args
        self.markdown_cache = None

    def save_file(self, data: bytes | str, filename: Path, mode="wb"):
        mode = None
        if type(data) == str:
            mode = "w"
        if type(data) == bytes:
            mode = "wb"
        if mode is None:
            raise ValueError("Only 'bytes' and 'str' can be saved")

        path = self.resolve_outdir(filename)
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, mode) as f:
            f.write(data)

    @property
    def grammar_path(self):
        return self.args.grammar_file

    def resolve_outdir(self, filename: Path):
        return self.args.outdir / filename

    def read_grammar(self, title: str):
        file = read_file(self.grammar_path, True)
        semicolon_regex = rb"(?<!');(?!')"
        escaped_file = re.sub(semicolon_regex, b".", file)
        title_bytes = bytes(title, "ascii")
        return b'"' + title_bytes + b'" {\r\n' + escaped_file + b"\r\n}"

    def generate_diagram(self, image: Path, desc: str) -> None:
        grammar = self.read_grammar(desc)

        encoded = parse.urlencode({"syntax": grammar})
        base_url = "https://ebnf.wiki-mathe-info.de"

        url = f"{base_url}/ebnf.php?{encoded}"
        with req.urlopen(url) as f:
            data = f.read()
            self.save_file(data, image)

    @property
    @staticmethod
    def template_str(cls):
        template_str = f"""\
        # VXen grammar

        This file documents VXen grammar. Parser and lexer follow these rules when
        parsing the source code.
        ## Grammar in EBNF

        ```ebnf
        $code
        ```

        ## Syntax diagram

        ![Syntax diagram should be visible here]($image)
        """
        return template_str

    def generate_markdowm(self, image: str) -> str | None:
        if self.markdown_cache is not None:
            return self.markdown_cache

        code = read_file(self.grammar_path, False)
        template = Template(textwrap.dedent(self.template_str))
        code = template.substitute({"image": image, "code": code})

        self.markdown_cache = code

        return self.markdown_cache

    def generate_html(self, image: str) -> str | None:
        markdown_text = self.generate_markdowm(image)
        code = markdown.markdown(
            markdown_text,
            output_format="xhtml",
            extensions=[markdown.extensions.fenced_code.FencedCodeExtension()],
        )

        return code

    def generate(self, image: Path):
        self.generate_diagram(image, title)
        mdfile = self.generate_markdowm(image)
        htmlfile = self.generate_html(image)

        self.save_file(mdfile, Path("Grammar.md"))
        self.save_file(htmlfile, Path("Grammar.html"))


if __name__ == "__main__":
    args: Args = make_parser().parse_args()

    title = "VXen programming language grammar in EBNF notation"
    grammar, image = "grammar.ebnf", "diagram.png"

    generator = Generator(args)
    generator.generate(Path(image))
