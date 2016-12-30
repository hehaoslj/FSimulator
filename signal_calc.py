#!/usr/bin/env python
# -*- coding: utf-8 -*-

#author: hehao
#date: 09-22-2016

import os
import sys
import cffi
import subprocess
import time
from base_config import Config, config_dumps


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
for(int i=0; i<10; ++i) {{
printf("calc [%d] = %lf\\n", i, fc->get_forecast());
}}

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
	FILE* f = fopen(argv[1], "rb");
        char buf[64];
        fread(buf,64, 1, f);
        printf("conf file is:%s\\n", buf);
        fclose(f);
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

clib_cflags=[ '-g', '-O3','-std=c++11', '-fPIC',  '-shared',
    '-march=native', '-mtune=native', '-malign-double', 
    '-I"{hpppath}"', '-I/usr/include -I/usr/local/include']
clib_dlflags=['-fPIC', '-shared']
clib_dllflags = ['-o', 'signal_calc.o', '-x', 'c++', '-c' ]
clib_appflags = ['-x', 'c++', '-o', 'signal_calc.app.o', '-c']
clib_lflags=['-L{libpath}', '{libpath}/lib{libname}.a', '-L/usr/lib', '-L/usr/local/lib', '-ljansson', '-lstdc++', '-lm', '-lrt', '-lpthread']

clib_flags=['-', '<<EOS']

if os.name == 'posix':
    if os.uname()[0] == 'Darwin': #OSX
        #clang dont support align-double
        clib_cflags.remove('-malign-double')
        #clang dont have rt library
        clib_lflags.remove('-lrt')
class signal_conf(Config):
    def __init__(self):
        self.conftype ="signal_calc"
        self.libpath="/home/hehao/work/fmtrader/lib/libforecaster"
        self.libname="forecaster"
        self.hpppath=self.libpath
        self.hppname="forecaster.h"
        self.insttype = "hc_0"
        self.output = Config()
        self.output.path="/home/hehao/data"

def signal_calc(conf):

    param = conf.dict()
#    args = ' '.join( clib_cc + clib_cflags + clib_dllflags + clib_dlflags + clib_flags).format(**param)
#    pcs = subprocess.Popen(clib_sh, shell=True,
#        stdin=subprocess.PIPE, close_fds=True)
#    print args
#    pcs.stdin.write(args)
#    code = clib_cxx.format(**param)
#    pcs.stdin.write( code )
#    pcs.stdin.write( "\nEOS\n")
#    pcs.stdin.flush()
    #time.sleep(1)
    #pcs.stdin.close()
    #pcs.wait()

    #pcs = subprocess.Popen(clib_sh, shell=True, 
    #    stdin = subprocess.PIPE, close_fds = True)

#    args = ' '.join( clib_cc + clib_cflags + ['signal_calc.o'] +
#        clib_dlflags + clib_lflags + ['-o libsignal_calc.so'] ).format(**param)
#    print args
#    pcs.stdin.write(args)
#    pcs.stdin.flush()
#    pcs.stdin.close()
#    pcs.wait()
    #sys.stdout.write(code)
    
    pcs = subprocess.Popen(clib_sh, shell=True, 
        stdin = subprocess.PIPE, close_fds=True)
    args = ' '.join( clib_cc + clib_cflags +
        clib_appflags + clib_flags).format(**param)
    print args
    pcs.stdin.write(args)
    f=open(curpath+"/clibapp.cpp", 'r')
    code = f.read()
    f.close()
    code ='#include "{hppname}"\n'+ code.replace('{', '{{').replace('}','}}')
    code = code.format(**param)
    #print "code"
    #print code
    pcs.stdin.write('\n'+code)
    pcs.stdin.write("\nEOS\n")
    pcs.stdin.flush()
    #pcs.stdin.close()
    #pcs.wait()

    #time.sleep(1)
    #pcs = subprocess.Popen(clib_sh, shell=True,
    #    stdin = subprocess.PIPE, close_fds=True)
    args = ' '.join(clib_cc + clib_cflags + ['signal_calc.app.o'] +
        clib_lflags + ['-o signal_calc.app']).format(**param)+'\n'
    print args
    pcs.stdin.write(args)
    pcs.stdin.flush()
    #pcs.stdin.close()
    #time.sleep(3)
    f=open("conf.json", "w")
    f.write( conf.dumps() )
    f.close()
    time.sleep(1)
    print "call signal calc"
    pcs.stdin.write('\n' + curpath+ '/signal_calc.app conf.json\n')
    pcs.stdin.flush()
    pcs.stdin.close()
    pcs.wait()
        
    #exit(0)
    return

def signal_ffi(conf):    
    ffi = cffi.FFI()
    ffi.cdef(clib_hpp)
    siglib = ffi.dlopen(curpath+'/libsignal_calc.so')
    arg = ffi.new('char[]', conf.dumps())
    siglib.signal_calc(arg)

#if __name__ == "__main__":
#    conf = signal_conf()
#    signal_calc(conf)

#rb2_0(rb02, rb01) ni2_0 ru2_0 zn2_0 rb_0
def test():
    c=signal_conf()
    c.loadf('conf_linux.json')
    signal_calc(c)

def test_dl():
    param=signal_conf()
    param.loadf('conf_linux.json')
    cmd = ' '.join(clib_cc + clib_cflags +clib_dllflags).format(**param.dict())
    cmd += " clibapp.cpp"
    pcs = subprocess.Popen(cmd, shell=True,
        stdout = subprocess.PIPE, close_fds=True)
    print pcs.stdout.read()

def test_ffi():
    ffi = cffi.FFI()
    with open('clibapp.h', 'r') as f:
        for ln in f.readlines():
            if ln[0] == "#":
                continue
            ffi.cdef(ln)
    #ffi.cdef('#include "clibapp.h"')
    siglib = ffi.dlopen(curpath + "/signal_calc.app")
    print siglib.fc_create
    #arg = ffi.new('char[]', "conf_linux.json")
    #args = ffi.new('char*[2]', (arg,)*2)
    #siglib.main(2, args)
    fc = siglib.fc_create("rb_0")
    siglib.fc_delete(fc)

client=None
import paramiko

def test_ssh():
    global client
    client = paramiko.SSHClient()
    client.load_system_host_keys()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect("192.168.56.101", 22, "hehao", "123")
    #chan = client.invoke_shell()
    #print(repr(client.get_transport()))
    chan = client.invoke_shell('vt102')
    time.sleep(1);
    chan.setblocking(0)
    chan.sendall('ls\n')
    print chan.recv(128)
    return chan

def test():
    import autotools
    autotools.test_ffi('/home/hehao/hwork/fsimulator/conf_linux.json')

if __name__ == "__main__":
    test()
