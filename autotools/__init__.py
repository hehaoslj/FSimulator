#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Auto tools

automatic simulation
Require paramiko cffi
"""
from autotools import base_config
from autotools.base_config import Config

from autotools import builder
from autotools import signal_calc

import md5
import os

def get_hkey(src):
    return md5.md5(src).hexdigest()

def test_ffi(cfgfile = 'conf_linux.json'):
    cfg=None
    src = ""
    with open(ffisrc, 'rb') as f:
        src = f.read()

    with open(cfgfile, 'rb') as f:
        code = f.read()
        cfg = base_config.config_loads(code)
        hkey = get_hkey(src+code)
        okey = None
        hkfile = os.path.join(os.path.dirname(cfgfile),
            os.path.splitext( os.path.basename(cfgfile) )[0]+ '.hkey')
        try:
            with open(hkfile, 'r') as fk:
                okey = fk.read()
        except:
            pass
        if hkey == okey and os.path.exists(cfg.signal_app.appname):
            pass
        else:
            fk=open(hkfile, 'w')
            fk.write(hkey)
            fk.close()
            builder.compile(cfg)

    return signal_calc.signal_calc(cfg)


ffisrc = os.path.join( os.path.dirname(__file__), "clibapp.cpp")
ffidef = os.path.join( os.path.dirname(__file__), "clibdef.h")

__all__=["test_ffi", "ffisrc", "ffidec"]
