#!/usr/bin/env python
# -*- coding: utf-8 -*-

""" Generate signals

Calculate signal
"""
import autotools
import cffi
import os

def signal_calc(cfg):
    ffi = cffi.FFI()
    with open(autotools.ffidef, 'r') as f:
        for ln in f.readlines():
            if ln[0] == "#":
                continue
            ffi.cdef(ln)
    siglib = ffi.dlopen(os.path.join(os.path.abspath('.'), cfg.signal_app.appname) )
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
    return siglib
