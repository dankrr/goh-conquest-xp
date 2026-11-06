#include "solo_policy.h"
#include <assert.h>
#include <stdio.h>
int main(void){SoloState s={0};uint64_t g=s.generation;solo_created(&s,0x1000,1,42);solo_page_commit(&s,g,42,1,0x1000,1);assert(solo_eligible(&s,0x1000,1));assert(!solo_eligible(&s,0,1));assert(!solo_eligible(&s,0x1001,1));assert(!solo_eligible(&s,0x1000,4));solo_invalidate(&s);assert(!solo_eligible(&s,0x1000,1));solo_created(&s,0x1000,1,42);assert(!solo_eligible(&s,0x1000,1));g=s.generation;solo_created(&s,0x1234,4,42);solo_page_commit(&s,g,42,4,0x1234,3);assert(!solo_eligible(&s,0x1234,3));g=s.generation;solo_created(&s,0x1234,1,43);solo_page_commit(&s,g,42,1,0x1234,1);assert(!solo_eligible(&s,0x1234,1));puts("PASS: solo origin, mismatch, teardown, same-address recreation, and mode guards");}
