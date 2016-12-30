#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""Make signal calculator

Using cffi module and native C/C++ compiler
"""

import autotools
import subprocess

def cmd_gen(cfg):

    cfg.signal_app.ffisrc = autotools.ffisrc
    cfg.signal_app.ffidef = autotools.ffidef
    sa_param = cfg.signal_app.safe_dict()
    #print sa_param
    cflag = "-I{hpppath}".format(**sa_param)
    lflag = "{ffisrc} {libpath}/lib{libname}.a -o {appname}".format(**sa_param)

    if cfg.builder.cflags.count(cflag) == 0:
        cfg.builder.cflags.append(cflag)
    if cfg.builder.lflags.count(lflag) == 0:
        cfg.builder.lflags.append(lflag)

    bd_param = cfg.builder.safe_dict()
    #print bd_param
    cmd = '{cc} {cflags} {aflags} {lflags}'.format(**bd_param)
    #print cmd
    return cmd

def compile(cfg):
    cmd = cmd_gen(cfg)
    pcs = subprocess.Popen(cmd, shell=True,
        stdout = subprocess.PIPE, close_fds=True)
    print pcs.stdout.read()
