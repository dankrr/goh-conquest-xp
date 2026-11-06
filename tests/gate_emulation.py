#!/usr/bin/env python3
"""Execute the original gate and the proposed branch-only copy on 1.067 bytes."""
import hashlib,struct,sys,pefile,subprocess,tempfile,pathlib
from unicorn import Uc,UC_ARCH_X86,UC_MODE_64,UC_HOOK_CODE
from unicorn.x86_const import *
p=pefile.PE(sys.argv[1]);raw=open(sys.argv[1],'rb').read()
assert hashlib.sha256(raw).hexdigest()=='4b6a807ad867c4f6eb1019376224d74fa3a6ac048355739ab0b76d73c7c0a267'
B=0x140000000;orig=B+0xc40b80;clone=B+0x1800000
code=p.get_data(0xc40b80,0x94)
assert code[0x10:0x12]==b'\x75\x75'
# displacement locations and next instruction offsets for every relative instruction
relocs=[(0xa,0xe),(0x15,0x19),(0x1b,0x1f),(0x28,0x2c),(0x2e,0x32),(0x43,0x47),(0x59,0x5d),(0x62,0x66),(0x6a,0x6e)]
def relocated():
 c=bytearray(code)
 for pos,end in relocs:
  disp=struct.unpack_from('<i',code,pos)[0];target=orig+end+disp
  struct.pack_into('<i',c,pos,target-(clone+end))
 c[0x10:0x12]=b'\x90\x90'
 return bytes(c)

with tempfile.TemporaryDirectory() as tmp:
 helper=pathlib.Path(tmp)/'clone_test'
 subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I',str(pathlib.Path(__file__).resolve().parents[1]/'src'),str(pathlib.Path(__file__).with_name('clone_test.c')),'-o',str(helper)],check=True)
 actual_clone=subprocess.check_output([str(helper)])
 assert actual_clone==relocated(), 'compiled C clone differs from independently relocated gate'
print('PASS: C clone matches independent relocations; corrupted signature and unreachable clone rejected')

def run(patched,mod,steam_present,steam_ready,card_present,card_ready,xpcheck,initial=0x7f):
 u=Uc(UC_ARCH_X86,UC_MODE_64);u.mem_map(B,0x1900000);u.mem_map(0x200000,0x200000)
 u.mem_write(orig,code);u.mem_write(clone,actual_clone);u.mem_write(B+0x1003f60,p.get_data(0x1003f60,4))
 def q(a,v):u.mem_write(a,struct.pack('<Q',v))
 # IAT getter -> pointer to the Steam wrapper. Its first virtual method reports state.
 q(B+0xfee1a0,0x230000);q(0x210000,0x220000 if steam_present else 0);q(0x220000,0x221000);q(0x221008,0x230100)
 u.mem_write(0x240cca,bytes([card_ready]));u.mem_write(0x25012e,bytes([initial]))
 q(0x2ff000,0x230200);u.reg_write(UC_X86_REG_RSP,0x2ff000);u.reg_write(UC_X86_REG_RCX,0x250000)
 seen=[]
 def hook(uc,addr,size,_):
  if addr==0x230200:uc.emu_stop();return
  vals={B+0x69d4d0:mod,0x230000:0x210000,0x230100:steam_ready,B+0xc3d840:0x240000 if card_present else 0}
  if addr in vals:
   seen.append(addr);uc.reg_write(UC_X86_REG_RAX,vals[addr])
  elif addr==B+0xcf8770:
   seen.append(addr);uc.reg_write(UC_X86_REG_XMM0,struct.unpack('<I',struct.pack('<f',xpcheck))[0])
  elif addr==B+0xf50260:seen.append(addr)
  else:return
  rsp=uc.reg_read(UC_X86_REG_RSP);ret=struct.unpack('<Q',uc.mem_read(rsp,8))[0];uc.reg_write(UC_X86_REG_RSP,rsp+8);uc.reg_write(UC_X86_REG_RIP,ret)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(clone if patched else orig,0x230201,count=500)
 return bytes(u.mem_read(0x25012e,1))[0],seen
cases=0
for steam_present in (0,1):
 for steam_ready in (0,1):
  for card_present in (0,1):
   for card_ready in (0,1):
    for value in (0.0,1.0):
     args=(steam_present,steam_ready,card_present,card_ready,value)
     clean=run(False,0,*args);patched=run(True,1,*args);blocked=run(False,1,*args)
     assert clean==patched,(args,clean,patched)
     assert blocked[0]==0,(args,blocked)
     cases+=1
for initial in (0,1):
 for mod in (0,1):
  for ready in (0,1):
   assert run(False,mod,1,1,1,ready,1.0,initial)[0]==run(False,mod,1,1,1,ready,1.0,0x7f)[0]
   assert run(True,mod,1,1,1,ready,1.0,initial)[0]==run(True,mod,1,1,1,ready,1.0,0x7f)[0]
ctor=p.get_data(0xc3975b,7)
assert ctor==bytes.fromhex('48 89 bb 28 01 00 00')
u=Uc(UC_ARCH_X86,UC_MODE_64);u.mem_map(0x200000,0x1000);u.mem_map(0x300000,0x1000)
u.mem_write(0x200000,ctor);u.mem_write(0x300128,b'\xff'*8);u.reg_write(UC_X86_REG_RBX,0x300000);u.reg_write(UC_X86_REG_RDI,0);u.emu_start(0x200000,0x200007)
assert u.mem_read(0x300128,8)==b'\0'*8
print('PASS: exact constructor instruction clears eight bytes including xpgain when RDI=0')
print('PASS: gate output does not depend on starting xpgain in tested branches')
print(f'PASS: {cases} full original-vs-branch-only eligibility cases; Steam/card/threshold checks preserved')
print('The proposed copy changes only the scene-mod branch. This emulation alone does not verify a live session or XP award.')
