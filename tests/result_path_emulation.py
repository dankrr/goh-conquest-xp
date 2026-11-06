#!/usr/bin/env python3
"""Execute two eligibility-dependent branches from the real 1.067 image."""
import hashlib,sys,struct,pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_64,UC_HOOK_CODE
from unicorn.x86_const import *
raw=open(sys.argv[1],'rb').read()
assert hashlib.sha256(raw).hexdigest()=='4b6a807ad867c4f6eb1019376224d74fa3a6ac048355739ab0b76d73c7c0a267'
p=pefile.PE(data=raw);base=0x140000000

def run(begin,stop,registers,writes,targets):
 u=Uc(UC_ARCH_X86,UC_MODE_64);u.mem_map(base+0xd00000,0x10000);u.mem_map(0x200000,0x10000)
 u.mem_write(base+begin,p.get_data(begin,stop-begin));u.mem_write(0x201140,struct.pack('<Q',0x202000))
 for addr,data in writes:u.mem_write(addr,data)
 for reg,val in registers:u.reg_write(reg,val)
 seen=[]
 def hook(u,addr,size,_):
  if addr-base in targets:seen.append(targets[addr-base]);u.emu_stop()
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(base+begin,base+stop,count=20)
 assert len(seen)==1,seen
 return seen[0]
for sil in (0,1):
 for ranked in (0,1):
  for xp in (0,1):
   outcome=run(0xd0858d,0xd085ca,[(UC_X86_REG_RSI,sil),(UC_X86_REG_RDI,0x201000),(UC_X86_REG_R15,0)],[(0x20212c,bytes([ranked])),(0x20212e,bytes([xp]))],{0xd085ab:'allocate',0xd085c9:'skip'})
   assert outcome==('allocate' if not sil and (ranked or xp) else 'skip'),(sil,ranked,xp,outcome)
for obj in (0,0x203000):
 outcome=run(0xd0881c,0xd0883a,[(UC_X86_REG_RSI,0x201000)],[(0x2011c0,struct.pack('<Q',obj))],{0xd08828:'call_builder',0xd08839:'skip'})
 assert outcome==('call_builder' if obj else 'skip'),(obj,outcome)
print('PASS: actual instructions skip optional result object when both card flags are zero and skip builder when object is absent')
print('This confirms a possible path, not the path taken by a specific battle.')
