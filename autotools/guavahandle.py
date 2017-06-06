#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
sys.path.append( os.path.abspath(".") )
#print sys.path
import struct


from autotools.base_config import Config



class pcap_guava2(object):
    guava2_udp_data_size = 104
    def __init__(self, fn = None, ids=[], stacksize=1):
        self.fn = fn
        self.ids = ids
        self.stacksize=stacksize
    def TIME(self):
        self.pkg_data.data.UpdateTime + '-' + str(self.pkg_data.data.UpdateMillisec).zfill(3)
    def reset(self):
        self.fp = None
        self.guava={}
        if self.stacksize <0:
            self.stacksize = 1

        if type(self.ids) in (str, unicode):
            ln= list()
            ln.append(self.ids)
            self.ids = ln
        elif type(self.ids) is tuple:
            self.ids=list(self.ids)
        elif type(self.ids) is list:
            pass
        else:
            self.ids=list()

        self.file_hdr = b'\0'*24

        self.pcap_hdr = Config()
        self.pcap_hdr.setn(['tv_sec', 'tv_usec', 'size','length'])
        self.pcap_hdr.setv([0,0,0,0])

        self.pkg_data = self.new_pkg()

        self.codes=list()

    def new_pkg(self):
        pkg_data = Config()
        pkg_data.udp_head = b'\0'*42
        with Config() as h:
            pkg_data.data = h
            h.setn(['InstrumentID',
            'fill1',
            'UpdateTime',
            'ActionDay',
            'UpdateMillisec',
            'Volume',
            'LastPrice',
            'Turnover',
            'OpenInterest',
            'BidPrice1',
            'AskPrice1',
            'BidVolume1',
            'AskVolume1'
            ])
            h.setv(['\0'*31, '\0', '\0'*8, '\0'*8, 0, 0,
            0.0, 0.0, 0.0, 0.0, 0.0,
            0, 0])
        return pkg_data

    def proc(self):

        self.reset()
        cnt = 0

        if type(self.fn) in (str, unicode):
            self.fp = open(self.fn, 'rb')
            self.file_hdr = self.fp.read(24)
            while True:
                #Read pcap header
                hdr = self.fp.read(16)
                if len(hdr) != 16:
                    break #EOF Occured!
                with self.pcap_hdr as h:
                    h.tv_sec, h.tv_usec, h.size, h.length = struct.unpack('4i', hdr)

                #Read data
                dat = self.fp.read(self.pcap_hdr.length)
                if len(dat) != self.pcap_hdr.length:
                    break #EOF Occured!

                #only proc normal data
                if self.pcap_hdr.length != 42+ self.guava2_udp_data_size:
                    continue


                self.pkg_data.udp_head = dat[:42]

                self.pkg_data.data.setv(struct.unpack('=31sc8s8s2i5d2i', dat[42:]))



                #Check ids
                cnt = cnt+1
                pkg_code = self.pkg_data.data.InstrumentID.replace('\0', '')
                if len(self.ids) == 0 or pkg_code in self.ids:
                    if not self.guava.has_key(pkg_code):
                        self.guava[pkg_code]=list()
                    pkg_stack = self.guava[pkg_code]
                    while len(pkg_stack) >= self.stacksize:
                        pkg_stack.pop(0)
                    #pkg_data = self.new_pkg()
                    #pkg_data.udp_head = self.pkg_data.udp_head
                    #pkg_data.data.setv(list(self.pkg_data.data._itemValues) )
                    #pkg_stack.append(pkg_data)

                #append new code in list
                #if self.codes.count(pkg_code) == 0:
                #    self.codes.append(pkg_code)
        print self.fn, "total pkg:", cnt
        keys = ''
        keyn = 0

        print len(self.codes), self.codes
    def file_hdr(self):
        return self.file_hdr

class pcap_guava1(object):
    guava_udp_head_size = 56
    guava_udp_normal_size = 52
    b"""ids empty: all symbols"""
    def __init__(self, fn=None, ids=[], stacksize=1):
        self.fn = fn
        self.ids=ids
        self.stacksize = stacksize

    def reset(self):
        self.fp = None
        self.guava={}
        if self.stacksize <0:
            self.stacksize = 1

        if type(self.ids) in (str, unicode):
            ln= list()
            ln.append(self.ids)
            self.ids = ln
        elif type(self.ids) is tuple:
            self.ids=list(self.ids)
        elif type(self.ids) is list:
            pass
        else:
            self.ids=list()

        self.file_hdr = b'\0'*24

        self.pcap_hdr = Config()
        self.pcap_hdr.setn(['tv_sec', 'tv_usec', 'size','length'])
        self.pcap_hdr.setv([0,0,0,0])

        self.pkg_data = self.new_pkg()

        self.codes=[]
    def new_pkg(self):
        pkg_data = Config()
        pkg_data.udp_head = b'\0'*42
        with Config() as h:
            pkg_data.head = h
            h.setn(['m_sequence',#/* 会话编号 */
            'm_exchange_id',#/* 市场  0 表示中金  1表示上期 */
            'm_channel_id',#/* 通道编号 */
            'm_symbol_type_flag',#/* 合约标志 */
            'm_symbol_code',#/* 合约编号 */
            'm_symbol', #/* 合约 */
            'm_update_time', #/* 最后更新时间(秒) */
            'm_millisecond', #/* 最后更新时间(毫秒) */
            'm_quote_flag'#/* 行情标志0 无time sale, 无lev1,1 有time sale 无lev1,2 无time sale, 有lev1,3 有time sale, 有lev1,4 summary信息    */
            ])
            h.setv([0,1,0,0,0,b'\0'*31,b'\0'*9,0,0])

        with Config() as h:
            pkg_data.normal=h
            h.setn(['m_last_px',#/* 最新价 */
            'm_last_share',     #/* 最新成交量 */
            'm_total_value',	#/* 成交金额 */
            'm_total_pos',    #/* 持仓量 */
            'm_bid_px',       #/* 最新买价 */
            'm_bid_share',      #/* 最新买量 */
            'm_ask_px',       #/* 最新卖价 */
            'm_ask_share'      #/* 最新卖量 */
            ])
            h.setv([0.0, 0, 0.0, 0.0, 0.0, 0, 0.0, 0])
        return pkg_data

    def proc(self):

        self.reset()
        cnt = 0

        if type(self.fn) in (str, unicode):
            self.fp = open(self.fn, 'rb')
            self.file_hdr = self.fp.read(24)
            while True:
                #Read pcap header
                hdr = self.fp.read(16)
                if len(hdr) != 16:
                    break #EOF Occured!
                with self.pcap_hdr as h:
                    h.tv_sec, h.tv_usec, h.size, h.length = struct.unpack('4i', hdr)

                #Read data
                dat = self.fp.read(self.pcap_hdr.length)
                if len(dat) != self.pcap_hdr.length:
                    break #EOF Occured!

                #only proc normal data
                if self.pcap_hdr.length != 42+ self.guava_udp_head_size + self.guava_udp_normal_size:
                    continue

                self.pkg_data.udp_head = dat[:42]

                self.pkg_data.head.setv(struct.unpack('=i3ci31s9sic', dat[42:42+self.guava_udp_head_size]))

                self.pkg_data.normal.setv(struct.unpack('=di3didi', dat[42+self.guava_udp_head_size:]))



                #Check ids
                cnt = cnt+1
                pkg_code = self.pkg_data.head.m_symbol.replace('\0', '')
                if len(self.ids) == 0 or pkg_code in self.ids:
                    if not self.guava.has_key(pkg_code):
                        self.guava[pkg_code]=list()
                    pkg_stack = self.guava[pkg_code]
                    while len(pkg_stack) >= self.stacksize:
                        pkg_stack.pop(0)
                    #pkg_data = self.new_pkg()
                    #pkg_data.udp_head = dat[:42]
                    #pkg_data.head.setv(list(self.pkg_data.head._itemValues))
                    #pkg_data.normal.setv(list(self.pkg_data.normal._itemValues))
                    #pkg_stack.append(pkg_data)

                #append new code in list
                #if self.codes.count(pkg_code) == 0:
                #    self.codes.append(pkg_code)
        print self.fn, "total pkg:", cnt
        keys = ''
        keyn = 0

        print len(self.codes), self.codes
    def file_hdr(self):
        return self.file_hdr

#void guavatask(const char* fn);
def guavatask(fn):
    import os
    from cffi import FFI
    cfg = Config()
    ffi = FFI()
    fn2 = ffi.string(fn)
    #print fn, fn2
    cfg.loadf(fn2)
    g1 = pcap_guava1(stacksize=cfg.stacksize, ids=cfg.idset)
    g2 = pcap_guava2(stacksize=cfg.stacksize, ids=cfg.idset)
    for fn in cfg.fset:
        if os.path.isfile(fn):
            if fn.find('guava2') <0:
                g1.fn = fn
                g1.proc()
            else:
                g2.fn = fn
                g2.proc()

if __name__ == "__main__":
    fset = ["/home/hehao/hwork/shif/package_2016-07-07__20-07-01.cap", "/Users/hehao/work/shif/package_2016-07-07__20-07-01.cap"]
    guavatask(fset, 5)


