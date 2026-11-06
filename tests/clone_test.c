#include "gate_clone.h"
#include <stdio.h>
#include <string.h>
int main(void){
 unsigned char out[0x94],bad[0x94];
 memcpy(bad,xp_gate_bytes,sizeof bad);bad[0x30]^=1;
 if(xp_gate_copy(out,0x141800000ULL,0x140c40b80ULL,bad))return 1;
 if(xp_gate_copy(out,0x7fff00000000ULL,0x140c40b80ULL,xp_gate_bytes))return 2;
 if(!xp_gate_copy(out,0x141800000ULL,0x140c40b80ULL,xp_gate_bytes))return 3;
 return fwrite(out,1,sizeof out,stdout)==sizeof out?0:4;
}
