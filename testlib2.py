#!/usr/bin/env python
# -*- coding: utf-8 -*-
import math

#double do_stuff(int x,int y);
def do_stuff(x, y):
    #print "adding %d and %d" % (x, y)
    rt = 0
    for i in range(x*y):
        rt +=math.sqrt( math.fabs(math.sin(x) + math.cos(y) ) * math.pi )

    return rt

#double do_stuff2(int x, int y);
def do_stuff2(x,y):
    return math.sqrt( math.fabs(math.sin(x) + math.cos(y) ) * math.pi )

#double do_stuff3(int x, int y);
def do_stuff3(x,y):
    rt = 0
    for i in range(x*y*100000):
        rt +=math.sqrt( math.fabs(math.sin(x) + math.cos(y) ) * math.pi )
    return rt
if __name__ == "__main__":
    rt = 0
    #for i in range(99999+1):
    rt += do_stuff3(3, 4)
    print "result is ", rt
