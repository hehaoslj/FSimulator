#!/usr/bin/env python
# -*- coding: utf-8 -*-

""" Optimization """
import os
import sys

sys.path.append( os.path.abspath(".") )

import autotools
from autotools.base_config import Config

import numpy as np
import struct

#void optimizetask(const char*);
def optimizetask(fn):
    f=open(ffi.string(fn),'r')
    code=f.read()
    f.close()
    cfg = autotools.base_config.config_loads(code)

    pos = cfg.signal_calc.files[0].find('__')
    if pos < 0:
        raise ValueError("file does not contain delimiter '__'")
    date = cfg.signal_calc.files[0][pos-10 :pos]
    time = cfg.signal_calc.files[0][pos+2    :pos+2+8]
    tp = cfg.insttype
    #print pos,date, time, tp
    fc = os.path.join(cfg.signal_calc.outpath,
    cfg.signal_calc.pattern.replace('%type', tp).replace('%date', date).replace('%time', time) )
    #print fc

    #with open(fc+'.fc', 'rb') as f:

    #y=np.loadtxt(fc+'.fc')
    #x=np.loadtxt(fc+'.csv', delimiter=',', skiprows=1, usecols=(2,3,4,5,6,7,8,9))

    #x2=np.array(x)
    #x2.resize((len(y),8))
    #r=np.linalg.lstsq(x2, y)[0]
    #print list(r)

if __name__ == "__main__":
    optimizetask(sys.argv[1])
