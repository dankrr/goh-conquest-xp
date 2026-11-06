#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
__declspec(dllimport) uintptr_t WINAPI test_prepare(void*,void*);
__declspec(dllimport) uintptr_t WINAPI test_builder(void*,void*,void*,uintptr_t);
__declspec(dllimport) void WINAPI test_set_game(void*);
__declspec(dllimport) int WINAPI test_setting(const wchar_t*);
__declspec(dllimport) int WINAPI test_runtime_policy(void);
__declspec(dllimport) void WINAPI test_gate(void*);
__declspec(dllimport) int WINAPI test_patch_stub(unsigned char*);
__declspec(dllimport) unsigned char WINAPI test_result(void*,void*);
__declspec(dllimport) void WINAPI test_page(void*,int);
__declspec(dllimport) int WINAPI test_lifecycle_bridges(void);
__declspec(dllimport) uintptr_t WINAPI test_bridge_init(int);
__declspec(dllimport) uintptr_t WINAPI test_bridge_end(void);
__declspec(dllimport) void WINAPI test_scoped_setup(void);
__declspec(dllimport) uintptr_t WINAPI test_scoped_init(int);
__declspec(dllimport) uintptr_t WINAPI test_scoped_end(void);
__declspec(dllimport) void WINAPI test_scoped_enabled(int);
static void WINAPI fake_page_entry(void *p,int mode){(void)p;test_scoped_init(mode);}
void mainCRTStartup(void){
 unsigned char *base=VirtualAlloc(NULL,0x1707000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);if(!base)ExitProcess(10);
 const unsigned char gate[]={0xfe,0x81,0x2e,0x01,0x00,0x00,0xc3};
 const unsigned char result[]={0xfe,0x41,0x10,0xb0,0x01,0xc3};
 for(unsigned i=0;i<sizeof(gate);i++)base[0xc40b80+i]=gate[i];
 for(unsigned i=0;i<sizeof(result);i++)base[0x4b9940+i]=result[i];
 unsigned char *object=base+0x1000,*record=base+0x2000;object[0x12e]=2;*(int*)(record+0x68)=124;
 *(void**)(object+0x730)=base+0x3000;*(void**)(object+0x738)=base+0x3010;
 Sleep(2000);
 wchar_t path[MAX_PATH];DWORD n=GetModuleFileNameW(GetModuleHandleW(L"diagtest.dll"),path,MAX_PATH);if(!n||n>=MAX_PATH)ExitProcess(15);while(n&&path[n-1]!=L'\\')n--;if(!n||n+18>=MAX_PATH)ExitProcess(16);const wchar_t suffix[]=L"goh_scoped.ini";for(unsigned i=0;i<sizeof(suffix)/sizeof(suffix[0]);i++)path[n+i]=suffix[i];
 if(!WritePrivateProfileStringW(L"features",L"achievements",L"0",path)||!WritePrivateProfileStringW(L"features",L"xp",L"0",path))ExitProcess(17);
 if(test_setting(L"achievements")||test_setting(L"xp"))ExitProcess(18);
 if(!WritePrivateProfileStringW(L"features",L"achievements",L"1",path)||!WritePrivateProfileStringW(L"features",L"xp",L"1",path))ExitProcess(19);
 if(!test_setting(L"achievements")||!test_setting(L"xp")||test_runtime_policy())ExitProcess(20);DeleteFileW(path);
 test_set_game(base);
 const unsigned char resultparent[]={0xfe,0x41,0x13,0x48,0x8b,0xc2,0xc3};
 for(unsigned i=0;i<sizeof(resultparent);i++){base[0xd0f130+i]=resultparent[i];base[0x6f6a80+i]=resultparent[i];}
 if(test_prepare(object,record)!=(uintptr_t)record||object[0x13]!=1)ExitProcess(23);
 if(test_builder(object,record,base+0x7000,7)!=(uintptr_t)record||object[0x13]!=2)ExitProcess(24);
 unsigned char *page=base+0x4000,*vtable=base+0x5000,*pagefn=base+0x6000;
 const unsigned char pagestub[]={0xfe,0x41,0x10,0x89,0x51,0x14,0xc3};for(unsigned i=0;i<sizeof(pagestub);i++)pagefn[i]=pagestub[i];
 *(void**)page=vtable;*(void**)(vtable+0xb8)=pagefn;test_page(page,4);if(page[0x10]!=1||*(int*)(page+0x14)!=4)ExitProcess(22);
 unsigned char *code=base+0xc3a4a8;const unsigned char pro[]={0x48,0x83,0xec,0x28};const unsigned char post[]={0x48,0x83,0xc4,0x28,0xc3};
 for(unsigned i=0;i<4;i++)code[i]=pro[i];for(unsigned i=0;i<5;i++)code[9+i]=post[i];
 if(!test_patch_stub(code+4))ExitProcess(14);((void (WINAPI*)(void*))code)(object);if(object[0x12e]!=3)ExitProcess(11);
 if(test_result(object,record)!=1 || object[0x10]!=1)ExitProcess(12);if(*(int*)(record+0x68)!=124)ExitProcess(13);
 const unsigned char candidate[]={0xfe,0x81,0x2e,0x01,0x00,0x00,0xfe,0x81,0x2e,0x01,0x00,0x00,0xc3};
 for(unsigned i=0;i<sizeof(candidate);i++)base[0x100000+i]=candidate[i];
 *(void**)(vtable+0xb8)=(void*)fake_page_entry;test_scoped_setup();object[0x12e]=0;
 test_gate(object);if(object[0x12e]!=1)ExitProcess(25);
 test_page(page,1);test_gate(object);if(object[0x12e]!=3)ExitProcess(26);
 test_scoped_enabled(0);test_gate(object);if(object[0x12e]!=4)ExitProcess(27);test_scoped_enabled(1);
 if(test_scoped_end()!=88)ExitProcess(28);test_scoped_init(1);test_gate(object);if(object[0x12e]!=5)ExitProcess(29);
 test_page(page,4);test_gate(object);if(object[0x12e]!=6)ExitProcess(30);
 const unsigned char initret[]={0xb8,77,0,0,0,0xc3},endremainder[]={0x48,0x83,0xc4,0x28,0xc3},subcall[]={0xb8,88,0,0,0,0xc3};
 for(unsigned i=0;i<sizeof(initret);i++)base[0xc0b195+i]=initret[i];
 for(unsigned i=0;i<sizeof(endremainder);i++)base[0xc0b2d9+i]=endremainder[i];
 for(unsigned i=0;i<sizeof(subcall);i++)base[0x5e4190+i]=subcall[i];
 if(!test_lifecycle_bridges()||test_bridge_init(1)!=77||test_bridge_end()!=88)ExitProcess(31);
 ExitProcess(0);
}
