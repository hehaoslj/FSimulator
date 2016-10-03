#!/usr/bin/env python
# -*- coding: utf-8 -*-

#author: hehao
#date: 09-22-2016

import os
import sys
import cffi
import json
import subprocess
import time

clib_hpp="""
void signal_calc(const char* const conf);
"""

clib_cxx="""

#include "{hppname}"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <time.h>

extern "C" __attribute__ ((visibility("default"))) void
signal_calc(const char* const conf) {{
std::string type="hc_0";
struct tm date;
time_t t = time(NULL);
localtime_r(&t, &date);

Forecaster *fc  = ForecasterFactory::createForecaster( type, date );
printf("trading instrument:%s\\n", fc->get_trading_instrument().c_str());
printf("the sin(1)=%lf, conf file is %s\\n",sin(1.0), conf);

}}

"""

clib_app = """
#include "{hppname}"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <time.h>

int main(int argc, char* argv[]) {{
std::string type="hc_0";
struct tm date;
time_t t = time(NULL);
localtime_r(&t, &date);

Forecaster *fc  = ForecasterFactory::createForecaster( type, date );
printf("trading instrument:%s\\n", fc->get_trading_instrument().c_str());
if(argc > 1) 
{{
    printf("the sin(1)=%lf\\nthe config file is %s\\n",sin(1.0), argv[1]);
}} 
else 
{{
    printf("please input the config file\\n");
}}

return 0;

}}

"""

curpath = os.path.dirname( os.path.abspath(__file__) )
#print curpath
clib_sh =['bash']

clib_cc = ['gcc']

clib_cflags=[ '-O3','-std=c++11', 
    '-march=native', '-mtune=native', '-malign-double', 
    '-I"{hpppath}"']
clib_dlflags=['-fPIC', '-shared']
clib_dllflags = ['-o', 'signal_calc.o', '-x', 'c++', '-c' ]
clib_appflags = ['-x', 'c++', '-o', 'signal_calc.app.o', '-c']
clib_lflags=['-L{libpath}', '{libpath}/lib{libname}.a', '-lstdc++', '-lm', '-lrt', '-lpthread']

clib_flags=['-', '<<EOS']


class signal_conf(object):
    def __init__(self):
        self.conftype ="signal_calc"
        self.libpath="/home/hehao/work/fmtrader/lib/libforecaster"
        self.libname="forecaster"
        self.hpppath=self.libpath
        self.hppname="forecaster.h"
        self.ittype = "hc_0"
        self.output = {}
        self.output['path']="/home/hehao/data"

if __name__ == "__main__":
    conf = signal_conf()
    #print json.dumps(conf.__dict__)
    args = ' '.join( clib_cc + clib_cflags + clib_dllflags + clib_dlflags + clib_flags).format(**conf.__dict__)
    pcs = subprocess.Popen(clib_sh, shell=True,
        stdin=subprocess.PIPE, close_fds=True)
    print args
    pcs.stdin.write(args)
    code = clib_cxx.format(**conf.__dict__)
    pcs.stdin.write( code )
    pcs.stdin.write( "\nEOS\n")
    pcs.stdin.flush()
    #pcs.stdin.close()
    time.sleep(1)
    #
    #pcs.wait()

    #pcs = subprocess.Popen(clib_sh, shell=True, 
    #    stdin = subprocess.PIPE, close_fds = True)

    args = ' '.join( clib_cc + clib_cflags + ['signal_calc.o'] + clib_dlflags + clib_lflags + ['-o libsignal_calc.so'] ).format(**conf.__dict__)
    print args
    pcs.stdin.write(args)
    pcs.stdin.flush()
    pcs.stdin.close()
    pcs.wait()
    #sys.stdout.write(code)
    
    pcs = subprocess.Popen(clib_sh, shell=True, 
        stdin = subprocess.PIPE, close_fds=True)
    args = ' '.join( clib_cc + clib_cflags + clib_appflags + clib_flags).format(**conf.__dict__)
    print args
    pcs.stdin.write(args)
    code = clib_app.format(**conf.__dict__)
    pcs.stdin.write(code)
    pcs.stdin.write("\nEOS\n")
    pcs.stdin.flush()
    #time.sleep(1)
    args = ' '.join(clib_cc + clib_cflags + ['signal_calc.app.o'] + clib_lflags + ['-o signal_calc.app']).format(**conf.__dict__)+'\n' 
    print args
    pcs.stdin.write(args)
    pcs.stdin.flush()
    #pcs.stdin.close()
    #time.sleep(3)
    f=open("conf.json", "w")
    f.write( json.dumps(conf.__dict__) )
    f.close()
    print "call signal calc"
    pcs.stdin.write(curpath+ '/signal_calc.app conf.json\n')
    pcs.stdin.flush()
    pcs.stdin.close()
    pcs.wait()
        
    exit(0)
    
    ffi = cffi.FFI()
    ffi.cdef(clib_hpp)
    siglib = ffi.dlopen(curpath+'/libsignal_calc.so')
    arg = ffi.new('char[]', json.dumps(conf.__dict__))
    siglib.signal_calc(arg)

