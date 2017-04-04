#!/usr/bin/env python
# -*- coding: utf-8 -*-



import json
import inspect

class ConfigObjectEncoder(json.JSONEncoder):
    def default(self, obj):
        if hasattr(obj, "to_json"):
            return self.default(obj.to_json())
        elif hasattr(obj, "__dict__"):
            d = dict(
                (key, value)
                for key, value in inspect.getmembers(obj)
                if not key.startswith("__")
                and not inspect.isabstract(value)
                and not inspect.isbuiltin(value)
                and not inspect.isfunction(value)
                and not inspect.isgenerator(value)
                and not inspect.isgeneratorfunction(value)
                and not inspect.ismethod(value)
                and not inspect.ismethoddescriptor(value)
                and not inspect.isroutine(value)
            )
            return self.default(d)
        return obj

def config_object_hook(obj):
    c = Config();
    if type(obj) == dict:
        c.__dict__ = obj
        c.__list__ = c.__dict__.keys()
    return c

def config_dumps(obj):
    return json.dumps(obj, cls = ConfigObjectEncoder, indent = 4, sort_keys = True)

def config_loads(s):
    return json.loads(s, object_hook=config_object_hook)

class Config(object):
    def __init__(self, *args):
        if len(args) == 2 and type(args[0]) in (list,tuple):
            self.__dict__ = dict( zip( list(args[0]), list(args[1]) ) )
            self.__list__ = list(args[0])
        else:
            self.__dict__ = dict()
            self.__list__ = list()
    def setv(self, values):
        idx=0
        max_idx = len(values)
        for key in self.__list__:
            if max_idx <= idx:
                break
            self.__dict__[key] = values[idx]
            idx+=1

    def setn(self, names):
        self.__list__ = list(names)
        #print self.__list__

    #list method
    def __getitem__(self, idx):
        key = self.__list__[idx]
        return self.__dict__[key]
    def __len__(self):
        return len(self.__dict__)
    #dict method
    def __getattr__(self, name):
        if name in self.__dict__:
            return self.__dict__[name]
        else:
            # Default behaviour
            return object.__getattribute__(self, name)
    def __enter__(self):
        #print "__enter__ method"
        return self  # 返回对象给as后的变量
    def __exit__(self, exc_type, exc_value, exc_traceback):
        #print "__exit__ method"
        if exc_traceback is None:
            #print "Exited without Exception"
            return True
        else:
            #print "Exited with Exception"
            return False

    def dumpf(self, s):
        f=open(s, 'w')
        f.write(self.dumps())
        f.close()
    def dumps(self):
        return config_dumps(self)
    def loads(self, s):
        self.__dict__ = json.loads(s)
        self.__list__ = self.__dict__.keys()
    def dict(self):
        return json.loads( self.dumps() )
    def loadf(self, s):
        f=open(s, 'r')
        ss = f.read()
        f.close()
        self.loads(ss)
    def safe_str(self, v):
        if type(v) == list:
            return  ' '.join(self.safe_str(vv) for vv in v)
        else:
            return v.safe_dict() if type(v) == Config else v.encode('utf-8') if type(v) == unicode else str(v).lower() if type(v) == bool else 'nil' if type(v) == type(None) else str(v)
    def safe_dict(self):
        o=dict()
        for k,v in self.dict().items():
                o[k] = self.safe_str(v)
        return o
