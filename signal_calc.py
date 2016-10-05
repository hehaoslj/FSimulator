#!/usr/bin/env python
# -*- coding: utf-8 -*-

#author: hehao
#date: 09-15-2016

import os
import sys
import cffi
import json
import subprocess

clib_hpp="""
void signal_calc(const char* const conf);
"""

clib_cxx="""

#include "{hppname}"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern "C" __attribute__ ((visibility("default"))) void
signal_calc(const char* const conf) {{
//Forecaster fc;
//printf("trading instrument:%s\\n", fc.get_trading_instrument().c_str());
printf("the sin(1)=%lf, conf file is %s\\n",sin(1.0), conf);

}}

"""
clib_sh =['bash']

clib_cc = ['gcc']

clib_cflags=[ '-fPIC', '-x', 'c++', '-I{hpppath}', '-shared']

clib_lflags=['-o', 'libsignal_calc.so', '-L{libpath}', '-l{libname}']

clib_flags=['-', '<<EOS']

class signal_conf(object):
    def __init__(self):
        self.name ="signal_calc"
        self.libpath="/Users/hehao/work/fmtrader/lib/libforecaster"
        self.libname="forecaster"
        self.hpppath=self.libpath
        self.hppname="forecaster.h"
if __name__ == "__main__":
    conf = signal_conf()
    #print json.dumps(conf.__dict__)
    args = ' '.join( clib_cc + clib_cflags + clib_lflags + clib_flags)
    pcs = subprocess.Popen(clib_sh, shell=True,
        stdin=subprocess.PIPE, close_fds=True)
    #print args
    pcs.stdin.write(args.format(**conf.__dict__))
    code = clib_cxx.format(**conf.__dict__)
    pcs.stdin.write( code )
    pcs.stdin.write( "\nEOS\n")
    pcs.stdin.flush()
    pcs.stdin.close()
    #
    pcs.wait()

    #sys.stdout.write(code)

    ffi = cffi.FFI()
    ffi.cdef(clib_hpp)
    siglib = ffi.dlopen('libsignal_calc.so')
    arg = ffi.new('char[]', json.dumps(conf.__dict__))
    siglib.signal_calc(arg)

