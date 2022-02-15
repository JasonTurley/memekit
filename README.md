# 

Memekit is a Linux kernel rootkit for low-level memeage.

The rootkit hijacks the open system call, causing the kernel to open the specified
meme file instead of any other PNG file.

It is a silly tool I created to learn more about Linux rootkits. It is inspired by
[kernelroll](https://github.com/fpletz/kernelroll) by [fpletz](https://github.com/fpletz).

## Functionality

- Ability to hide itself 
- Hijacks the open() system call

## Installation

First, find the address of your sys_call_table:

```
# grep sys_call_table /boot/System.map-$(uname -r)
```

Memekit is compile and installed like a normal Linux Kernel Module. 

```
# make
# insmod memekit.ko memefile=/path/to/meme.png
```

To verify that the module successfully loaded, run:

```
# dmesg | grep memekit
```

To unload the module, enter:

```
# rmmod memekit
```

## Resources

- [Modern Linux Rootkits 101](http://turbochaos.blogspot.com/2013/09/linux-rootkits-101-1-of-3.html)
- [The Linux Kernel Module Programming Guide](https://tldp.org/LDP/lkmpg/2.6/html/index.html)
