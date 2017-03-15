#!/usr/bin/env python
# -*- coding: utf-8 -*-

import cffi
import os
curpath = os.path.abspath('.')

ffibuilder = cffi.FFI()

ffibuilder.set_source("forecastertask",
"""
extern int fc_calc(const char*);
int forecastertask(const char* cfg)
{
    return fc_calc(cfg);
}

"""
,libraries=['forecaster']
,library_dirs=[curpath]
,extra_link_args=["-Wl,-rpath,%s" % curpath])

init_code = ""
init_api=""

ffibuilder.embedding_init_code(init_code)
ffibuilder.embedding_api(init_api)

ffibuilder.compile(target="libforecastertask-1.0.*", verbose=True)
