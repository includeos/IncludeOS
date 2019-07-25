/**
 * Master thesis
 * by Alf-Andre Walla 2016-2017
 *
**/
#ifdef PLATFORM_x86_solo5
extern "C" void solo5_exec(const char*, size_t);
#else
static void* HOTSWAP_AREA = (void*) 0x8000;
extern "C" void  hotswap(char*, const uint8_t*, int, void*, void*);
extern "C" char  __hotswap_length;
extern "C" void  hotswap64(char*, const uint8_t*, int, uint32_t, void*, void*);
extern uint32_t  hotswap64_len;
extern void      __x86_init_paging(void*);
#endif
