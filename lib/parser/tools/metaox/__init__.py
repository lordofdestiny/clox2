__version__ = "0.1.0"
__author__ = "lordofdestiny"


from .types import term, nterm
from .visitor.term_nterm_creator import TermNtermCollector
from .visitor.names import collect_nterm_names

METAOX_MODULE_NAME = __name__
