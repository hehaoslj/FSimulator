#!/usr/bin/env python
# -*- coding: utf-8 -*-

""" Optimizer signals multiple & parameter
    Generate opencl script
"""

import os
import sys
sys.path.append( os.path.abspath(".") )

import time

import cffi

import autotools
from autotools import base_config

import web.template

def render(code, **keywords):
    tmpl = web.template.Template(code, **keywords)
    return lambda *a, **kw: tmpl(*a, **kw)

def get_signalcalc_pattern_name(cfg, idx = 0):
    nm = cfg.signal_calc.files[idx]
    pos = nm.find('__')
    if pos < 0:
        raise ValueError("file does not contain delimiter '__'")
    date = nm[pos-10 :pos]
    time = nm[pos+2    :pos+2+8]
    tp = cfg.insttype
    name = cfg.signal_calc.pattern.replace('%type', tp).replace('%date', date).replace('%time', time)
    return name

def get_idx_file(cfg):
    nm = get_signalcalc_pattern_name(cfg, 0)  +".idx"
    fname = os.path.join(cfg.signal_calc.outpath, nm)

    #Do replace
    reps = cfg.optimizer.clreps
    for i in range(len(reps)/2):
        fname = fname.replace(reps[2*i], reps[2*i+1])
    return fname

#const char* genopencltask(const char*);
def genopencltask(fn):
    global cl_tmpl
    #print cl_tmpl
    gen = render(cl_tmpl)

    #open config file
    ffi = cffi.FFI()
    f = open(ffi.string(fn), 'rb')
    code = f.read()
    cfg = base_config.config_loads(code)
    f.close()

    ##generate ctx
    #basic info
    ctx = base_config.Config()
    ctx.user_dept = "Department of IF"
    ctx.user_name = "He Hao"
    ctx.tm_now = time.ctime()
    ctx.version ="V1.01"
    #ctx.summary=""

    #cl-specified info
    ctx.clvec = cfg.optimizer.clvec
    ctx.aligned =cfg.optimizer.aligned

    # open index file
    idx_file = get_idx_file(cfg)
    f = open(idx_file, 'rb')
    code = f.read()
    idx = base_config.config_loads(code)
    f.close()

    #index-specified info
    ctx.param_count = idx.size
    ctx.param_names = idx.names

    result = str(gen(ctx))
    rt = ffi.new("char[]", result)

    return rt

