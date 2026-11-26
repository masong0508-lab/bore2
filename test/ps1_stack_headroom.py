#!/usr/bin/env python3
"""Static PS1 ELF resident-memory guard, not a runtime stack watermark."""
import argparse
from pathlib import Path

def main():
    p=argparse.ArgumentParser()
    p.add_argument("symbols",type=Path)
    p.add_argument("--minimum",type=int,default=8192)
    args=p.parse_args()
    values=[]
    for line in args.symbols.read_text().splitlines():
        fields=line.split()
        if len(fields)>=3 and fields[-1]=="_end":
            values.append(int(fields[0],16)&0x1fffffff)
    if len(values)!=1:
        raise SystemExit("Expected exactly one ELF _end symbol")
    end=values[0]
    free=0x200000-end
    print(f"PS1 resident end: 0x{end:06x}; theoretical stock-RAM stack headroom: {free} bytes")
    if end<0x10000 or free<args.minimum:
        raise SystemExit(f"Insufficient resident stack headroom: {free} bytes < {args.minimum}")

if __name__=="__main__": main()
