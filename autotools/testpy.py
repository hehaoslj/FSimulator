#!/usr/bin/env python
# -*- coding: utf-8 -*-

from cffi import FFI
import os
import sys
def test():
    ffi = FFI()
    ffi.cdef("""
    void guavatask(const char* fn);


    int fc_calc(const char* cfg);
    int forecastertask(const char*);
    void optimizetask(const char*);
    """)
    if len(sys.argv) < 1:
        raise ValueError("no configuration file,please run testpy.py <conf.json>")
    curpath = os.path.dirname(os.path.abspath(__file__) )

    fn = ffi.new("char[]", sys.argv[1])
    ft = ffi.dlopen( os.path.join(curpath, "libforecastertask-1.0.so") )
    ot = ffi.dlopen( os.path.join(curpath, "liboptimizetask-1.0.so") )
    ft.forecastertask(fn)
    ot.optimizetask(fn)

if __name__ == "__main__":
    test()

