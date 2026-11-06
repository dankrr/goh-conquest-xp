#ifndef GOH_SOLO_POLICY_H
#define GOH_SOLO_POLICY_H
#include <stdint.h>
typedef struct {uint64_t generation; uintptr_t active,authorized; unsigned init_thread; int init_mode;} SoloState;
static void solo_invalidate(SoloState *s){s->generation++;s->active=0;s->authorized=0;s->init_thread=0;s->init_mode=-1;}
static void solo_created(SoloState *s,uintptr_t p,int mode,unsigned tid){solo_invalidate(s);s->active=p;s->init_mode=mode;s->init_thread=tid;}
static void solo_page_commit(SoloState *s,uint64_t start,unsigned tid,int requested,uintptr_t p,int type){
 if(requested==1 && s->generation==start+1 && p && p==s->active && s->init_thread==tid && s->init_mode==1 && type==1)s->authorized=p;
 else s->authorized=0;
}
static int solo_eligible(const SoloState *s,uintptr_t p,int type){return s->authorized&&s->authorized==p&&s->active==p&&type==1;}
#endif
