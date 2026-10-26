#!/usr/bin/env python3
"""A small FastTracker XM reader for BORE 2's band tools: instruments (samples, loops), and every channel's notes."""
import struct
class Inst:
    def __init__(s): s.name=''; s.samples=[]; s.map=[0]*96
class Samp:
    pass
def read(fn):
    d = open(fn,'rb').read()
    hl, = struct.unpack('<I', d[60:64])
    songlen, restart, nch, npat, ninst, flags, tempo, bpm = struct.unpack('<HHHHHHHH', d[64:80])
    order = list(d[80:80+256])[:songlen]
    p = 60 + hl
    pats = []
    for _ in range(npat):
        pl, = struct.unpack('<I', d[p:p+4]); rows, ps = struct.unpack('<HH', d[p+5:p+9]); body = d[p+pl:p+pl+ps]; p += pl + ps
        grid = [[None]*nch for _ in range(rows)]; q = 0
        for r in range(rows):
            for c in range(nch):
                if ps == 0: break
                b = body[q]; q += 1; note = ins = vol = eff = par = 0
                if b & 0x80:
                    if b & 1: note = body[q]; q += 1
                    if b & 2: ins = body[q]; q += 1
                    if b & 4: vol = body[q]; q += 1
                    if b & 8: eff = body[q]; q += 1
                    if b & 16: par = body[q]; q += 1
                else:
                    note = b; ins = body[q]; vol = body[q+1]; eff = body[q+2]; par = body[q+3]; q += 4
                grid[r][c] = (note, ins, vol, eff, par)
        pats.append(grid)
    insts = []
    for _ in range(ninst):
        il, = struct.unpack('<I', d[p:p+4]); I = Inst(); I.name = d[p+4:p+26].split(b'\0')[0].decode('latin1')
        ns, = struct.unpack('<H', d[p+27:p+29])
        if ns:
            shs, = struct.unpack('<I', d[p+29:p+33]); I.map = list(d[p+33:p+129])
        p += il; hdrs = []
        for _ in range(ns):
            S = Samp(); S.len, S.ls, S.ll = struct.unpack('<III', d[p:p+12]); S.vol = d[p+12]; S.fine = struct.unpack('b', d[p+13:p+14])[0]
            S.type = d[p+14]; S.pan = d[p+15]; S.rel = struct.unpack('b', d[p+16:p+17])[0]; S.name = d[p+18:p+40].split(b'\0')[0].decode('latin1')
            hdrs.append(S); p += shs
        for S in hdrs:
            S.data = d[p:p+S.len]; p += S.len; S.bits16 = bool(S.type & 16); S.loop = S.type & 3
            I.samples.append(S)
        insts.append(I)
    return dict(nch=nch, order=order, pats=pats, insts=insts, tempo=tempo, bpm=bpm)
