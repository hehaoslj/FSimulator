#!/usr/bin/env python
# -*- coding: utf-8 -*-

import cffi
import os

ffibuilder = cffi.FFI()


ffibuilder.set_source("signalcalctask", """
""")

init_code = """#!/usr/bin/env python
# -*- coding: utf-8 -*-
from signalcalctask import ffi

"""
init_api=""

replace_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "signalcalc_tmpl.cl")
with open(replace_file, 'r') as f:
    init_code += "cl_tmpl = \"\"\""
    for ln in f.readlines():
        init_code += ln
    init_code += "\"\"\""

code_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "signalcalchandle.py")

with open(code_file, "r") as f:
    last_ln=""
    for ln in f.readlines():
        if ln[:4] =="def " and last_ln[:1] == "#":
            init_code += "@ffi.def_extern()\n"
            init_api += last_ln.replace("#", "")
        init_code += ln
        last_ln = ln

print init_api
ffibuilder.embedding_init_code(init_code)
ffibuilder.embedding_api(init_api)

ffibuilder.compile(target="libsignalcalctask-1.0.*", verbose=True)
