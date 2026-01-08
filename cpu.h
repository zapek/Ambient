#ifndef AMBIENT_CPU_H
#define AMBIENT_CPU_H
/*
 * $Id: cpu.h,v 1.10 2015/02/01 14:14:10 piru Exp $
 */

/*
 * Cache flags.
 */
#define	CACHEFLAGF_AVAILABLE    0x1
#define	CACHEFLAGF_ACTIVE       0x2
#define	CACHEFLAGF_INST         0x4
#define	CACHEFLAGF_DATA         0x8
#define	CACHEFLAGF_UNIFIED      0x10
#define	CACHEFLAGF_INSTSNOOP    0x20
#define	CACHEFLAGF_DATASNOOP    0x40
#define	CACHEFLAGF_UNIFIEDSNOOP 0x80
#define	CACHEFLAGF_INSTLOCKED   0x100
#define	CACHEFLAGF_DATALOCKED   0x200
#define	CACHEFLAGF_COPYBACK     0x10000

/*
 * cpu_cache_stream_start() flags.
 */
#define CCSSF_WRITES     (1 << 0UL) /* use when data will be read and written */
#define CCSSF_TRANSCIENT (1 << 1UL) /* use when data should be flushed straight to RAM to preserve L2 */
#define CCSSF_CHANNEL1   (1 << 2UL) /* use channel 1 (0 is default) (XXX: should get rid of it and have some allocchan-like function) */
#define CCSSF_CHANNEL2   (1 << 3UL) /* use channel 2 */
#define CCSSF_CHANNEL3   (1 << 4UL) /* use channel 3 */

/*
 * These enums indexes the array in cpu.c.
 * *must* be kept synced properly and have the
 * exact same number of entries or the world
 * will fall.
 */
enum {
	CPUID_601,
	CPUID_603,
	CPUID_603e,
	CPUID_603r,
	CPUID_603ev,
	CPUID_604,
	CPUID_604e,
	CPUID_604r,
	CPUID_604ev,
	CPUID_740_750_a,
	CPUID_745_755,
	CPUID_750CX_a,
	CPUID_750CX_b,
	CPUID_750CXe,
	CPUID_750FX,
	CPUID_750GL,
	CPUID_750GX,
	CPUID_740_750_b,
	CPUID_7400,
	CPUID_7410,
	CPUID_7451,
	CPUID_7450,
	CPUID_7455,
	CPUID_7447,
	CPUID_7457,
	CPUID_82xx,
	CPUID_970,
	CPUID_7447A,
	CPUID_7448,
	CPUID_970FX,
	CPUID_970MP,
	CPUID_5200,
	CPUID_5200LE,
	/* insert here */
	CPUID_NONE,
};

/*
 * Since OpenFirmware sucks I provide
 * fixes as I know better, har.
 */
#define CPUF_L2_256  (1 << 0UL) /* 256kB L2 cache */
#define CPUF_L3_NONE (1 << 1UL) /* no L3 cache */
#define CPUF_L2_512  (1 << 2UL) /* 512kB L2 cache */

ULONG cpu_init(void);
void cpu_cleanup(void);

ULONG cpu_id(ULONG version, ULONG revision);
CONST_STRPTR cpu_name(ULONG id);
ULONG cpu_flags(ULONG id);
void cpu_cachebuf(APTR obj, ULONG flags);
#if USE_CPU_CACHEHINTS
void cpu_cache_stream_start(CONST_APTR ptr, ULONG blockcount, ULONG blocksize, LONG stride, ULONG flags);
void cpu_cache_stream_stop(ULONG channel);
void cpu_cache_zero(APTR ptr, ULONG size);
APTR cpu_cache_malloc(ULONG size);
void cpu_cache_free(APTR ptr);
#endif

#endif /* AMBIENT_CPU_H */
