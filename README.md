# memekit

Memekit is a Linux kernel rootkit for low-level memeage.

The rootkit hijacks the open system call, causing the kernel to open the specified
meme file instead of any other PNG file. It currently only works on files ending in ".png" but
the source code can easily be modified to support other file extensions.

This is a silly tool I created to learn more about Linux rootkits. It was compiled and run on a
system running Ubuntu 14.04 with Linux kernel version 4.4.

It is inspired by
[kernelroll](https://github.com/fpletz/kernelroll) by [fpletz](https://github.com/fpletz) which is a
rootkit for an older kernel version.

## Functionality

This rootkit has the following functionality:

- Ability to hide itself from lsmod output
- Ability to hide itself from the /proc/modules and /sys/module directories
- Hijacks the open() system call to call a malicious version

## Finding the sys_call_table

In order to hijack a system call, we need the address of the sys_call_table. The code uses
`kallsyms_lookup_name` which works on all kernels below 5.7+.

For Linux kernel versions 5.7+ the `kallsyms_lookup_name` function is not exported, so look in
System.map file to find the address of your sys_call_table.

```
# grep sys_call_table /boot/System.map-$(uname -r)
ffffffff82000300 R sys_call_table
```

## Installation

Compile and install memekit like any other Linux Kernel Module:

```
# make
# insmod memekit.ko memefile=/path/to/meme.png
```

Verify that the module successfully loaded:

```
# dmesg | grep memekit
```

Unload the module to return the system to normal:

```
# rmmod memekit
```

## Resources

- [Modern Linux Rootkits 101](http://turbochaos.blogspot.com/2013/09/linux-rootkits-101-1-of-3.html)
- [The Linux Kernel Module Programming Guide](https://tldp.org/LDP/lkmpg/2.6/html/index.html)
- [WP: Safe or Not](http://vulnfactory.org/blog/2011/08/12/wp-safe-or-not/)
