#!/usr/bin/env python
# -*- coding: utf-8 -*-

import cffi
import os

ffibuilder = cffi.FFI()

#C++ code
ffibuilder.set_source("my_plugin", """
#include <math.h>
double do_stuff4(int x, int y)
{
    int i;
    double rt = 0;
    for(i=0;i<100000*x*y;++i)
    {
        rt += sqrt(fabs(sin(x)+cos(y))*M_PI);
    }
    return rt;
}
""")

init_code = """#!/usr/bin/env python
# -*- coding: utf-8 -*-
from my_plugin import ffi

"""
init_api=""

code_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "testlib2.py")

with open(code_file, "r") as f:
    last_ln=""
    for ln in f.readlines():
        if ln[:4] =="def ":
            init_code += "@ffi.def_extern()\n"
            init_api += last_ln.replace("#", "")
        init_code += ln
        last_ln = ln

#print init_code
ffibuilder.embedding_init_code(init_code)
ffibuilder.embedding_api(init_api)

ffibuilder.compile(target="libplugin-1.5.*", verbose=True)
