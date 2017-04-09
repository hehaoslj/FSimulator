#!/usr/bin/env python
# -*- coding: utf-8 -*-

""" Calculate sub signals
    Generate data set
"""

import os
import sys
sys.path.append( os.path.abspath(".") )


import cffi
import os
import autotools
from autotools import base_config

import web

#void forecastertask(const char*);
def forecastertask(fn):
    ffi = cffi.FFI()

    f = open(ffi.string(fn), 'rb')
    code = f.read()
    cfg = base_config.config_loads(code)
    f.close()

    print fn

    f=open("./autotools/clibdef.h", "r")
    cdef = f.read()
    ffi.cdef(cdef)
    siglib = ffi.dlopen("libforecaster.so")

    print dir(siglib)

    fc = siglib.fc_create('rb_0')
    inst = ffi.new("const char**")
    siglib.fc_get_trading_instrument(fc, inst )

    print 'instrument:', ffi.string(inst[0])
    size = siglib.fc_get_instruments_size(fc)
    print 'sub size:', size
    for k in range(size):
        siglib.fc_get_instruments(fc, k, inst)
        print 'sub[',k,']:',ffi.string(inst[0])

    size = siglib.fc_get_subsignals_size(fc);
    print 'signal size:', size
    for k in range(size):
        siglib.fc_get_subsignals(fc, k, inst)
        print 'signal[', k,']:', ffi.string(inst[0])

    for fname in cfg.signal_calc.files:
        datfile = ffi.new("char []", os.path.join(cfg.signal_calc.datpath, fname).encode('utf-8') )
        outpath = ffi.new("char []", cfg.signal_calc.outpath.encode('utf-8') )
        namepattern = ffi.new("char []", cfg.signal_calc.pattern.encode('utf-8') )
        siglib.fc_calc_subsignals(fc, datfile, outpath, namepattern)
    siglib.fc_delete( fc )

    fc = None
    inst = None

if __name__ == "__main__":
    forecastertask("../conf_linux.json")
