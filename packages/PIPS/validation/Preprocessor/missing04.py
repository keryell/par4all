from __future__ import with_statement
from validation import vworkspace
from pyps import module
with vworkspace() as w:
    w.props.PREPROCESSOR_MISSING_FILE_HANDLING="generate"
    w.props.STUB_IO_BARRIER=True
    w.props.STUB_MEMORY_BARRIER=True
    f=w.fun.f
    f.display(module.print_code_cumulated_effects)
    f.display(module.print_code_preconditions)

