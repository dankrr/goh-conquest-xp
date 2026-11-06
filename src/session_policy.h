#ifndef GOH_SESSION_POLICY_H
#define GOH_SESSION_POLICY_H
#include <stdint.h>
/* Pure model. A production verifier for these evidence fields does not yet exist. */
typedef struct { uintptr_t identity; uint64_t generation; int active, origin_solo_conquest, peer_count_known, remote_peers; } SessionEvidence;
typedef struct { SessionEvidence evidence; int raw_type; } Lifecycle;
/* Sampling is observational. An absent/unreadable state and peer data never confer trust. */
static inline void lifecycle_observe(Lifecycle *l, uintptr_t id, int type) {
 if(l->evidence.identity!=id || l->raw_type!=type){l->evidence.identity=id;l->evidence.generation++;l->raw_type=type;}
 l->evidence.active=-1;l->evidence.origin_solo_conquest=-1;l->evidence.peer_count_known=0;l->evidence.remote_peers=-1;
}
/* Model only: when a trusted membership event exists, revoke previous proof. */
static inline void lifecycle_membership_change(Lifecycle *l,int known,int remote){
 if(l->evidence.peer_count_known!=known || l->evidence.remote_peers!=remote){l->evidence.generation++;l->evidence.origin_solo_conquest=-1;}
 l->evidence.peer_count_known=known;l->evidence.remote_peers=remote;
}
static inline int policy_model(const SessionEvidence *s, uintptr_t event_session, uint64_t event_generation) {
 return s && s->identity && event_session && s->identity==event_session && s->generation==event_generation && s->active==1 && s->origin_solo_conquest==1 && s->peer_count_known==1 && s->remote_peers==0;
}
/* No runtime evidence provider is validated. This cannot be turned on via config. */
static inline int policy_runtime(const SessionEvidence *s, uintptr_t event_session, uint64_t event_generation) {
 (void)s; (void)event_session; (void)event_generation; return 0;
}
#endif
