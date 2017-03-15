#!/usr/bin/env python
# -*- coding: utf-8 -*-

import cffi
import os

ffibuilder = cffi.FFI()


ffibuilder.set_source("guavatask", """
""")

init_code = """#!/usr/bin/env python
# -*- coding: utf-8 -*-
from guavatask import ffi

"""
init_api=""

replace_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "base_config.py")

code_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "guavahandle.py")

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

ffibuilder.compile(target="libguavatask-1.0.*", verbose=True)
