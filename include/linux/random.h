/*
 * include/linux/random.h
 *
 * Include file for the random number generator.
 */
#ifndef _LINUX_RANDOM_H
#define _LINUX_RANDOM_H

#include <linux/bug.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/once.h>

#include <uapi/linux/random.h>

struct random_ready_callback {
	struct list_head list;
	void (*func)(struct random_ready_callback *rdy);
	struct module *owner;
};

extern void add_device_randomness(const void *, size_t);
extern void add_bootloader_randomness(const void *, size_t);

#if defined(LATENT_ENTROPY_PLUGIN) && !defined(__CHECKER__)
static inline void add_latent_entropy(void)
{
	add_device_randomness((const void *)&latent_entropy,
			      sizeof(latent_entropy));
}
#else
static inline void add_latent_entropy(void) {}
#endif

extern void add_input_randomness(unsigned int type, unsigned int code,
				 unsigned int value) __latent_entropy;
extern void add_interrupt_randomness(int irq) __latent_entropy;

extern void get_random_bytes(void *buf, size_t nbytes);
extern int wait_for_random_bytes(void);
extern int __init rand_initialize(void);
extern bool rng_is_initialized(void);
extern int add_random_ready_callback(struct random_ready_callback *rdy);
extern void del_random_ready_callback(struct random_ready_callback *rdy);
extern size_t __must_check get_random_bytes_arch(void *buf, size_t nbytes);

#ifndef MODULE
extern const struct file_operations random_fops, urandom_fops;
#endif

u32 get_random_u32(void);
u64 get_random_u64(void);
static inline unsigned int get_random_int(void)
{
	return get_random_u32();
}
static inline unsigned long get_random_long(void)
{
#if BITS_PER_LONG == 64
	return get_random_u64();
#else
	return get_random_u32();
#endif
}

/* Calls wait_for_random_bytes() and then calls get_random_bytes(buf, nbytes).
 * Returns the result of the call to wait_for_random_bytes. */
static inline int get_random_bytes_wait(void *buf, size_t nbytes)
{
	int ret = wait_for_random_bytes();
	get_random_bytes(buf, nbytes);
	return ret;
}

#define declare_get_random_var_wait(var) \
	static inline int get_random_ ## var ## _wait(var *out) { \
		int ret = wait_for_random_bytes(); \
		if (unlikely(ret)) \
			return ret; \
		*out = get_random_ ## var(); \
		return 0; \
	}
declare_get_random_var_wait(u32)
declare_get_random_var_wait(u64)
declare_get_random_var_wait(int)
declare_get_random_var_wait(long)
#undef declare_get_random_var

unsigned long randomize_page(unsigned long start, unsigned long range);

/*
 * This is designed to be standalone for just prandom
 * users, but for now we include it from <linux/random.h>
 * for legacy reasons.
 */
#include <linux/prandom.h>

#ifdef CONFIG_ARCH_RANDOM
# include <asm/archrandom.h>
#else
static inline bool __must_check arch_get_random_long(unsigned long *v)
{
	return false;
}
static inline bool __must_check arch_get_random_int(unsigned int *v)
{
	return false;
}
static inline bool __must_check arch_get_random_seed_long(unsigned long *v)
{
	return false;
}
static inline bool __must_check arch_get_random_seed_int(unsigned int *v)
{
	return false;
}
#endif

/*
 * Called from the boot CPU during startup; not valid to call once
 * secondary CPUs are up and preemption is possible.
 */
#ifndef arch_get_random_seed_long_early
static inline bool __init arch_get_random_seed_long_early(unsigned long *v)
{
	WARN_ON(system_state != SYSTEM_BOOTING);
	return arch_get_random_seed_long(v);
}
#endif

#ifndef arch_get_random_long_early
static inline bool __init arch_get_random_long_early(unsigned long *v)
{
	WARN_ON(system_state != SYSTEM_BOOTING);
	return arch_get_random_long(v);
}
#endif

#endif /* _LINUX_RANDOM_H */
