#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <tlhelp32.h>
#include <stdint.h>
#include "session_policy.h"
#include "solo_policy.h"
#include "gate_clone.h"



void *memset(void *p,int c,size_t n){unsigned char *s=p;while(n--)*s++=(unsigned char)c;return p;}
void *memcpy(void *d,const void *s,size_t n){unsigned char *x=d;const unsigned char *y=s;while(n--)*x++=*y++;return d;}
static size_t len(const char *s){size_t n=0;while(s[n])n++;return n;}
static char *lastslash(char *s){char *p=0;while(*s){if(*s=='\\')p=s;s++;}return p;}
static int compare(const void *a,const void *b,size_t n){const unsigned char *x=a,*y=b;while(n--){if(*x++!=*y++)return 1;}return 0;}
static void copy(void *a,const void *b,size_t n){unsigned char *x=a;const unsigned char *y=b;while(n--)*x++=*y++;}
static void widecat(wchar_t *a,const wchar_t *b){while(*a)a++;while((*a++=*b++));}
/* A deliberately narrow adaptation of nature2468/gohmodloader's achievement example.
   This replaces only the collector call for the zero-initialized local vector. */
static const unsigned char digest[32]={0x4b,0x6a,0x80,0x7a,0xd8,0x67,0xc4,0xf6,0xeb,0x10,0x19,0x37,0x62,0x24,0xd7,0x4f,0xa3,0xa6,0xac,0x04,0x83,0x55,0x73,0x9a,0xb0,0xb7,0x6d,0x73,0xc7,0xc0,0xa2,0x67};
static const unsigned char block[]={0x48,0x89,0x78,0x10,0x0f,0x57,0xc0,0x4c,0x89,0x70,0x18,0x33,0xd2,0x45,0x33,0xf6,0x4c,0x89,0x74,0x24,0x30,0xf3,0x0f,0x7f,0x44,0x24,0x20,0xe8,0x3c,0x66,0xcd,0xff,0x48,0x8b,0x5c,0x24,0x20,0x48,0x8b,0x7c,0x24,0x28,0x48,0x3b,0xdf,0x0f,0x85,0xdf,0x00,0x00,0x00};
static const unsigned char collector[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x20,0x56,0x41,0x54,0x41,0x56,0x48,0x83,0xec,0x60};
static HMODULE self,system_version;
static INIT_ONCE once=INIT_ONCE_STATIC_INIT;
static void logmsg(const char *s){ char path[MAX_PATH]; DWORD n=GetModuleFileNameA(self,path,MAX_PATH); if(!n||n>=MAX_PATH)return; char *p=lastslash(path); if(!p)return; copy(p+1,"goh_scoped.log",15); HANDLE h=CreateFileA(path,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0); if(h!=INVALID_HANDLE_VALUE){DWORD w; WriteFile(h,s,(DWORD)len(s),&w,0); WriteFile(h,"\r\n",2,&w,0); CloseHandle(h);} }
static BOOL CALLBACK load_real(PINIT_ONCE x,PVOID y,PVOID *z){ (void)x;(void)y;(void)z; wchar_t p[MAX_PATH]; UINT n=GetSystemDirectoryW(p,MAX_PATH); if(!n||n+13>=MAX_PATH)return FALSE; widecat(p,L"\\version.dll"); system_version=LoadLibraryW(p); return system_version!=NULL; }
static FARPROC real(const char *name){if(!InitOnceExecuteOnce(&once,load_real,0,0))return NULL;return GetProcAddress(system_version,name);}
DWORD WINAPI GetFileVersionInfoSizeW(LPCWSTR p,LPDWORD h){typedef DWORD (WINAPI *F)(LPCWSTR,LPDWORD); F f=(F)real("GetFileVersionInfoSizeW");return f?f(p,h):0;}
BOOL WINAPI GetFileVersionInfoW(LPCWSTR p,DWORD h,DWORD n,LPVOID d){typedef BOOL (WINAPI *F)(LPCWSTR,DWORD,DWORD,LPVOID);F f=(F)real("GetFileVersionInfoW");return f?f(p,h,n,d):FALSE;}
BOOL WINAPI VerQueryValueW(LPCVOID p,LPCWSTR s,LPVOID *v,PUINT n){typedef BOOL (WINAPI *F)(LPCVOID,LPCWSTR,LPVOID*,PUINT);F f=(F)real("VerQueryValueW");return f?f(p,s,v,n):FALSE;}
static int hash_ok(void){ wchar_t path[MAX_PATH]; unsigned char buf[1024],out[32]; DWORD n=0; BCRYPT_ALG_HANDLE a=NULL; BCRYPT_HASH_HANDLE h=NULL; HANDLE f=INVALID_HANDLE_VALUE; int ok=0; if(!GetModuleFileNameW(NULL,path,MAX_PATH))goto done; f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL); if(f==INVALID_HANDLE_VALUE)goto done; if(BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,NULL,0)<0 || BCryptCreateHash(a,&h,NULL,0,NULL,0,0)<0)goto done; for(;;){ if(!ReadFile(f,buf,sizeof(buf),&n,NULL))goto done; if(!n)break; if(BCryptHashData(h,buf,n,0)<0)goto done; } if(BCryptFinishHash(h,out,32,0)<0)goto done; ok=!compare(out,digest,32); done: if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);if(f!=INVALID_HANDLE_VALUE)CloseHandle(f);return ok; }

/* All lifecycle data is observational: no verified origin or peer provider exists. */
static SRWLOCK track_lock=SRWLOCK_INIT;
static Lifecycle lifecycle={.raw_type=-2};
static uintptr_t last_gate_session; static uint64_t last_gate_epoch;
static void track(uintptr_t id,int type){
 AcquireSRWLockExclusive(&track_lock);
 uint64_t prior=lifecycle.evidence.generation;lifecycle_observe(&lifecycle,id,type);if(prior!=lifecycle.evidence.generation){last_gate_session=0;last_gate_epoch=0;}
 ReleaseSRWLockExclusive(&track_lock);
}
static int event_binding(const char *tag,uintptr_t id){int matched=-1;AcquireSRWLockExclusive(&track_lock);
 if(!compare(tag,"xp_gate",8)){last_gate_session=id;last_gate_epoch=lifecycle.evidence.generation;}
 if(!compare(tag,"xp_result",10))matched=id && id==last_gate_session && lifecycle.evidence.generation==last_gate_epoch;
 ReleaseSRWLockExclusive(&track_lock);return matched;}
static unsigned long long epoch(void){unsigned long long n;AcquireSRWLockShared(&track_lock);n=lifecycle.evidence.generation;ReleaseSRWLockShared(&track_lock);return n;}
static int setting(const wchar_t *key,int fallback){wchar_t path[MAX_PATH];DWORD n=GetModuleFileNameW(self,path,MAX_PATH);if(!n||n>=MAX_PATH)return fallback;while(n&&path[n-1]!=L'\\'&&path[n-1]!=L'/')--n;if(!n||n+18>=MAX_PATH)return fallback;const wchar_t suffix[]=L"goh_scoped.ini";for(unsigned i=0;i<sizeof(suffix)/sizeof(suffix[0]);i++)path[n+i]=suffix[i];return GetPrivateProfileIntW(L"features",key,fallback,path)==1;}
/* Observation only: calls the original functions exactly once, preserving results. */
static unsigned char *game;
static SRWLOCK solo_lock=SRWLOCK_INIT;static SoloState solo={0};static int xp_enabled;
static uintptr_t (WINAPI *original_init)(int);static uintptr_t (WINAPI *original_end)(void);
static void (WINAPI *xp_copy)(void*);
static unsigned long long rdptr(const void *p);
static uintptr_t WINAPI scoped_init(int mode){
 AcquireSRWLockExclusive(&solo_lock);solo_invalidate(&solo);uint64_t gen=solo.generation;ReleaseSRWLockExclusive(&solo_lock);
 uintptr_t result=original_init(mode);uintptr_t id=(uintptr_t)rdptr(game+0x15a4a80);
 AcquireSRWLockExclusive(&solo_lock);if(solo.generation==gen){solo.active=id;solo.init_mode=mode;solo.init_thread=GetCurrentThreadId();}ReleaseSRWLockExclusive(&solo_lock);return result;
}
static uintptr_t WINAPI scoped_end(void){AcquireSRWLockExclusive(&solo_lock);solo_invalidate(&solo);ReleaseSRWLockExclusive(&solo_lock);return original_end();}

static int readmem(const void *p,void *out,SIZE_T n){SIZE_T got=0;return p && ReadProcessMemory(GetCurrentProcess(),p,out,n,&got) && got==n;}
static unsigned long long rdptr(const void *p){unsigned long long v=0;readmem(p,&v,8);return v;}
static int byteval(const void *p){unsigned char v=0;return readmem(p,&v,1)?v:-1;}
static int int32val(const void *p){int v=0;return readmem(p,&v,4)?v:-1;}
static void snapshot(const char *tag,void *obj,int before,int after,int xp,int result){
 char s[768]; unsigned long long session=rdptr(game+0x15a4a80), begin=rdptr(game+0x15586b0), end=rdptr(game+0x15586b8), current=rdptr(game+0x15a83d0);
 int session_type=session?int32val((void*)(uintptr_t)(session+16)):-1, active=0, scene=0, entries=-1; track((uintptr_t)session,session_type); int same=event_binding(tag,(uintptr_t)session); unsigned long long n,member_begin=0,member_end=0; int member_entries=-1;
 if(obj && readmem((char*)obj+0x730,&member_begin,8) && readmem((char*)obj+0x738,&member_end,8) && member_end>=member_begin && member_end-member_begin<=8192 && ((member_end-member_begin)%8)==0)member_entries=(int)((member_end-member_begin)/8);
 if(begin && end>=begin && end-begin<=8192 && ((end-begin)%8)==0){entries=(int)((end-begin)/8); for(n=begin;n<end;n+=8){unsigned long long mod=rdptr((void*)(uintptr_t)n);int a=mod?byteval((void*)(uintptr_t)(mod+0x118)):-1, b=mod?byteval((void*)(uintptr_t)(mod+0x119)):-1;if(a<0||b<0){active=-1;scene=-1;break;}if(a>0)active++;if(a>0&&b>0)scene++;}}
 wsprintfA(s,"t=%lu event=%s global_session=%p type=%d epoch=%I64u observed_gate_match=%d origin=unknown peers=unknown member_entries=%d manager_first=%p gate_object=%p mods_entries=%d active=%d active_scene=%d xpgain_before=%d xpgain_after=%d xp_value=%d original_return=%d",(unsigned long)GetTickCount(),tag,(void*)(uintptr_t)session,session_type,epoch(),same,member_entries,(void*)(uintptr_t)current,obj,entries,active,scene,before,after,xp,result);logmsg(s);
}
static void WINAPI observe_gate(void *obj){int before=byteval((char*)obj+0x12e);typedef void (WINAPI *F)(void*);
 uintptr_t id=(uintptr_t)rdptr(game+0x15a4a80);int type=id?int32val((void*)(id+16)):-1;
 AcquireSRWLockExclusive(&solo_lock);int applied=xp_enabled&&xp_copy&&solo_eligible(&solo,id,type);
 if(applied)xp_copy(obj);else ((F)(game+0xc40b80))(obj);ReleaseSRWLockExclusive(&solo_lock);
 int after=byteval((char*)obj+0x12e);char line[120];wsprintfA(line,"event=xp_decision applied=%d mode=%d",applied,type);logmsg(line);snapshot("xp_gate",obj,before,after,-1,applied);}
static unsigned char WINAPI observe_result(void *selfp,void *resultp){typedef unsigned char (WINAPI *F)(void*,void*);int xp=int32val((char*)resultp+0x68);int a=byteval((char*)resultp+0x6c),b=byteval((char*)resultp+0x6d),c=byteval((char*)resultp+0x6e),d=byteval((char*)resultp+0x6f);unsigned char r=((F)(game+0x4b9940))(selfp,resultp);snapshot("xp_result",0,a,b,xp,r);char line[160];wsprintfA(line,"t=%lu event=result_flags flag_6c=%d flag_6d=%d flag_6e=%d flag_6f=%d",(unsigned long)GetTickCount(),a,b,c,d);logmsg(line);return r;}

static void WINAPI observe_page(void *page,int mode){
 typedef void (WINAPI *F)(void*,int);uintptr_t vt=*(uintptr_t*)page;F original=*(F*)(vt+0xb8);
 AcquireSRWLockExclusive(&solo_lock);solo_invalidate(&solo);uint64_t gen=solo.generation;ReleaseSRWLockExclusive(&solo_lock);
 original(page,mode);uintptr_t id=(uintptr_t)rdptr(game+0x15a4a80);int type=id?int32val((void*)(id+16)):-1;
 AcquireSRWLockExclusive(&solo_lock);solo_page_commit(&solo,gen,GetCurrentThreadId(),mode,id,type);ReleaseSRWLockExclusive(&solo_lock);
 snapshot("conquest_page_after",page,-1,-1,-1,mode);
}
/* Observe result preparation and both identified result-builder call sites. */
static uintptr_t WINAPI observe_result_prepare(void *a,void *b){
 typedef uintptr_t (WINAPI *F)(void*,void*);snapshot("result_prepare",0,-1,-1,-1,-1);return ((F)(game+0x6f6a80))(a,b);
}
static uintptr_t WINAPI observe_result_builder(void *a,void *b,void *c,uintptr_t d){
 typedef uintptr_t (WINAPI *F)(void*,void*,void*,uintptr_t);snapshot("result_builder_enter",0,-1,-1,-1,-1);
 uintptr_t r=((F)(game+0xd0f130))(a,b,c,d);snapshot("result_builder_return",0,-1,-1,-1,-1);return r;
}
static const unsigned char gate_site[]={0xe8,0x51,0x66,0,0,0x8b,0xd0,0x48,0x8b,0xcf,0xe8,0x77,0x67,0,0,0x48,0x8b,0xcf,0xe8,0xcf,0x66,0,0,0xb3,0x01,0x48,0x8b};
static const unsigned char result_site[]={0xe8,0x21,0x1e,0x9a,0xff,0x48,0x8d,0x54,0x24,0x70,0x48,0x8b,0x48,0x48,0xe8,0x83,0xa4,0x7a,0xff,0xe8,0x7e,0x87,0x9e,0xff};
static const unsigned char gate_entry[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0xe8,0x42,0xc9,0xa5,0xff,0x84,0xc0,0x75,0x75};
static const unsigned char result_entry[]={0x48,0x8b,0xc4,0x57,0x48,0x81,0xec,0xe0,0,0,0,0x48,0x89,0x58,0x08,0x48};
static int verified(void){return !compare(game+0xc3a49a,gate_site,sizeof(gate_site))&&!compare(game+0xd0f4aa,result_site,sizeof(result_site))&&!compare(game+0xc40b80,gate_entry,sizeof(gate_entry))&&!compare(game+0x4b9940,result_entry,sizeof(result_entry));}
static unsigned char *alloc_near(void){uintptr_t center=(uintptr_t)game; for(uintptr_t d=0x2000000;d<0x70000000;d+=0x10000){for(int sign=0;sign<2;sign++){uintptr_t p=sign?center-d:center+d;if(p<0x10000)continue;unsigned char *x=VirtualAlloc((void*)p,0x1000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(x)return x;}}return NULL;}
static void thunk(unsigned char *p,void *dest){const unsigned char head[]={0xff,0x25,0,0,0,0};copy(p,head,6);copy(p+6,&dest,8);}
static int lifecycle_bridges(unsigned char *bridge,const unsigned char *init_entry,const unsigned char *end_entry){
 copy(bridge+0x300,init_entry,5);bridge[0x305]=0xe9;int64_t di=(int64_t)(uintptr_t)(game+0xc0b195)-(int64_t)(uintptr_t)(bridge+0x30a);if(di<INT32_MIN||di>INT32_MAX)return 0;int32_t ri=(int32_t)di;copy(bridge+0x306,&ri,4);original_init=(void*)(bridge+0x300);
 copy(bridge+0x340,end_entry,9);int64_t dc=(int64_t)(uintptr_t)(game+0x5e4190)-(int64_t)(uintptr_t)(bridge+0x349);if(dc<INT32_MIN||dc>INT32_MAX)return 0;int32_t rc=(int32_t)dc;copy(bridge+0x345,&rc,4);bridge[0x349]=0xe9;int64_t de=(int64_t)(uintptr_t)(game+0xc0b2d9)-(int64_t)(uintptr_t)(bridge+0x34e);if(de<INT32_MIN||de>INT32_MAX)return 0;int32_t re=(int32_t)de;copy(bridge+0x34a,&re,4);original_end=(void*)(bridge+0x340);
 return 1;
}
static DWORD WINAPI start(void *unused){
 (void)unused;game=(unsigned char*)GetModuleHandleW(NULL);int achievements=setting(L"achievements",0),xp=setting(L"xp",0);
 xp_enabled=xp;logmsg("START solo Conquest candidate; original Steam eligibility remains unchanged");
 static const unsigned char init_entry[]={0x48,0x89,0x5c,0x24,0x08};
 static const unsigned char end_entry[]={0x48,0x83,0xec,0x28,0xe8,0xb7,0x8e,0x9d,0xff};
 static const unsigned char page[]={0xff,0x90,0xb8,0,0,0};
 static const unsigned char prep[]={0xe8,0xf3,0xe4,0x9e,0xff};
 static const unsigned char builder1[]={0xe8,0xeb,0x6a,0,0};
 static const unsigned char builder2[]={0xe8,0xf7,0x68,0,0};
 if(!hash_ok()){logmsg("REFUSED executable SHA-256 mismatch or unreadable");return 0;}
 if(!verified()||compare(game+0xc0b190,init_entry,5)||compare(game+0xc0b2d0,end_entry,9)||compare(game+0x17f2cf,page,6)||compare(game+0xd08588,prep,5)||compare(game+0xd08640,builder1,5)||compare(game+0xd08834,builder2,5)||(achievements&&(compare(game+0x9c6b64,block,sizeof(block))||compare(game+0x69d1c0,collector,sizeof(collector))))){logmsg("REFUSED instruction signature mismatch");return 0;}
 struct Patch {uintptr_t rva;unsigned size;void *target;DWORD protection;unsigned char bytes[10];} patches[]={
  {0xc3a4ac,5,(void*)observe_gate,0,{0}}, {0xd0f4b8,5,(void*)observe_result,0,{0}},
  {0x17f2cf,6,(void*)observe_page,0,{0}},
  {0xd08588,5,(void*)observe_result_prepare,0,{0}}, {0xd08640,5,(void*)observe_result_builder,0,{0}}, {0xd08834,5,(void*)observe_result_builder,0,{0}},
  {0xc0b190,5,(void*)scoped_init,0,{0}}, {0xc0b2d0,9,(void*)scoped_end,0,{0}},
  {0x9c6b7f,5,0,0,{0}}};
 const unsigned hooks=8,total=achievements?9:8;unsigned char *bridge=alloc_near();if(!bridge){logmsg("REFUSED bridge allocation");return 0;}
 for(unsigned i=0;i<hooks;i++){thunk(bridge+i*16,patches[i].target);int64_t d=(int64_t)(uintptr_t)(bridge+i*16)-(int64_t)(uintptr_t)(game+patches[i].rva+5);if(d<-2147483648LL||d>2147483647LL){VirtualFree(bridge,0,MEM_RELEASE);logmsg("REFUSED bridge range");return 0;}int32_t rel=(int32_t)d;patches[i].bytes[0]=i>=6?0xe9:0xe8;copy(patches[i].bytes+1,&rel,4);for(unsigned j=5;j<patches[i].size;j++)patches[i].bytes[j]=0x90;}
 for(unsigned i=0;i<5;i++)patches[8].bytes[i]=0x90;
 /* Entry trampolines preserve both original prologues and relocate teardown's call. */
 if(!lifecycle_bridges(bridge,init_entry,end_entry))goto bridge_fail;
 if(!xp_gate_copy(bridge+0x400,(uintptr_t)(bridge+0x400),(uintptr_t)(game+0xc40b80),game+0xc40b80))goto bridge_fail;xp_copy=(void*)(bridge+0x400);
 DWORD oldbridge;if(!VirtualProtect(bridge,0x1000,PAGE_EXECUTE_READ,&oldbridge)){VirtualFree(bridge,0,MEM_RELEASE);logmsg("REFUSED bridge protection");return 0;}FlushInstructionCache(GetCurrentProcess(),bridge,0x500);
 HANDLE ss=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);static HANDLE ts[1024];unsigned count=0,protected=0;int done=0;THREADENTRY32 e={sizeof(e)};
 if(ss==INVALID_HANDLE_VALUE||!Thread32First(ss,&e))goto cleanup;
 do{if(e.th32OwnerProcessID==GetCurrentProcessId()&&e.th32ThreadID!=GetCurrentThreadId()){
  if(count==1024)goto cleanup;HANDLE t=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,e.th32ThreadID);if(!t)goto cleanup;
  if(SuspendThread(t)==(DWORD)-1){CloseHandle(t);goto cleanup;}ts[count++]=t;CONTEXT c={0};c.ContextFlags=CONTEXT_CONTROL;if(!GetThreadContext(t,&c))goto cleanup;
  for(unsigned i=0;i<total;i++)if(c.Rip>=(DWORD64)(game+patches[i].rva)&&c.Rip<(DWORD64)(game+patches[i].rva+patches[i].size))goto cleanup;
 }}while(Thread32Next(ss,&e));
 if(!verified()||compare(game+0xc0b190,init_entry,5)||compare(game+0xc0b2d0,end_entry,9)||compare(game+0x17f2cf,page,6)||compare(game+0xd08588,prep,5)||compare(game+0xd08640,builder1,5)||compare(game+0xd08834,builder2,5)||(achievements&&(compare(game+0x9c6b64,block,sizeof(block))||compare(game+0x69d1c0,collector,sizeof(collector)))))goto cleanup;
 for(;protected<total;protected++)if(!VirtualProtect(game+patches[protected].rva,patches[protected].size,PAGE_EXECUTE_READWRITE,&patches[protected].protection))goto cleanup;
 for(unsigned i=0;i<total;i++){copy(game+patches[i].rva,patches[i].bytes,patches[i].size);FlushInstructionCache(GetCurrentProcess(),game+patches[i].rva,patches[i].size);}done=1;
 cleanup:while(protected){--protected;DWORD dummy;VirtualProtect(game+patches[protected].rva,patches[protected].size,patches[protected].protection,&dummy);}while(count){ResumeThread(ts[--count]);CloseHandle(ts[count]);}if(ss!=INVALID_HANDLE_VALUE)CloseHandle(ss);
 if(!done){VirtualFree(bridge,0,MEM_RELEASE);logmsg("REFUSED unable to install all observation hooks");return 0;}
 logmsg("ACTIVE scoped candidate; XP requires matching solo Conquest origin and session");
 unsigned long long last=~0ULL,lastmgr=~0ULL;int lasttype=-99;for(;;){unsigned long long p=rdptr(game+0x15a4a80),mgr=rdptr(game+0x15a83d0);int t=p?int32val((void*)(uintptr_t)(p+16)):-1;if(p!=last||t!=lasttype||mgr!=lastmgr){snapshot("session_change",0,-1,-1,-1,-1);last=p;lasttype=t;lastmgr=mgr;}Sleep(1000);}
 bridge_fail: VirtualFree(bridge,0,MEM_RELEASE);logmsg("REFUSED trampoline or gate relocation");return 0;
}
BOOL WINAPI DllMain(HINSTANCE m,DWORD reason,LPVOID reserved){(void)reserved;if(reason==DLL_PROCESS_ATTACH){self=m;DisableThreadLibraryCalls(m);HANDLE t=CreateThread(NULL,0,start,NULL,0,NULL);if(t)CloseHandle(t);}return TRUE;}

#ifdef DIAGNOSTIC_TEST
__declspec(dllexport) int WINAPI test_lifecycle_bridges(void){
 const unsigned char a[]={0x48,0x89,0x5c,0x24,0x08},b[]={0x48,0x83,0xec,0x28,0xe8,0xb7,0x8e,0x9d,0xff};
 unsigned char *p=alloc_near();DWORD old;if(!p||!lifecycle_bridges(p,a,b)||!VirtualProtect(p,0x1000,PAGE_EXECUTE_READ,&old))return 0;
 FlushInstructionCache(GetCurrentProcess(),p,0x400);return 1;
}
__declspec(dllexport) uintptr_t WINAPI test_bridge_init(int mode){return original_init(mode);}
__declspec(dllexport) uintptr_t WINAPI test_bridge_end(void){return original_end();}
static uintptr_t WINAPI fake_session_init(int mode){uintptr_t p=(uintptr_t)(game+0x8000);copy(game+0x15a4a80,&p,8);copy((void*)(p+16),&mode,4);return 77;}
static uintptr_t WINAPI fake_session_end(void){uintptr_t p=0;copy(game+0x15a4a80,&p,8);return 88;}
__declspec(dllexport) void WINAPI test_scoped_setup(void){original_init=fake_session_init;original_end=fake_session_end;xp_copy=(void*)(game+0x100000);xp_enabled=1;AcquireSRWLockExclusive(&solo_lock);solo_invalidate(&solo);ReleaseSRWLockExclusive(&solo_lock);}
__declspec(dllexport) uintptr_t WINAPI test_scoped_init(int mode){return scoped_init(mode);}
__declspec(dllexport) uintptr_t WINAPI test_scoped_end(void){return scoped_end();}
__declspec(dllexport) void WINAPI test_scoped_enabled(int enabled){xp_enabled=enabled;}
__declspec(dllexport) uintptr_t WINAPI test_prepare(void *a,void *b){return observe_result_prepare(a,b);}
__declspec(dllexport) uintptr_t WINAPI test_builder(void *a,void *b,void *c,uintptr_t d){return observe_result_builder(a,b,c,d);}
__declspec(dllexport) void WINAPI test_page(void *p,int mode){observe_page(p,mode);}
__declspec(dllexport) void WINAPI test_set_game(void *base){game=base;}
__declspec(dllexport) int WINAPI test_setting(const wchar_t *key){return setting(key,0);}
__declspec(dllexport) int WINAPI test_runtime_policy(void){SessionEvidence e={0x1000,1,1,1,1,0};return policy_runtime(&e,0x1000,1);}
__declspec(dllexport) int WINAPI test_patch_stub(unsigned char *site){unsigned char *p=alloc_near();if(!p)return 0;thunk(p,(void*)observe_gate);DWORD old;if(!VirtualProtect(p,0x1000,PAGE_EXECUTE_READ,&old))return 0;int64_t d=(int64_t)(uintptr_t)p-(int64_t)(uintptr_t)(site+5);if(d<-2147483648LL||d>2147483647LL)return 0;int32_t v=(int32_t)d;site[0]=0xe8;copy(site+1,&v,4);FlushInstructionCache(GetCurrentProcess(),site,5);return 1;}
__declspec(dllexport) void WINAPI test_gate(void *obj){observe_gate(obj);}
__declspec(dllexport) unsigned char WINAPI test_result(void *selfp,void *obj){return observe_result(selfp,obj);}
#endif
