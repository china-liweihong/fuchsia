# Avoiding a problem with the SYSRET and IRETQ instructions

On x86-64, the kernel uses the `SYSRET` and `IRETQ` instructions to return
from system calls and interrupts, respectively. We must be careful not to
use a non-canonical return address in these instructions, at least on Intel
CPUs, because this causes the instructions to fault in kernel mode, which
is unsafe. In contrast, on AMD CPUs, `SYSRET` faults in user mode when used
with a non-canonical return address.

One of the problems with these instructions faulting in kernel mode is that
the instructions occur at the end of the interrupt or syscall handling
mechanism, after the `gs` register has been swapped from the kernel
`x86_percpu` variable to a value that is controlled by userspace. When an
exception occurs in kernel mode, the `gs` register is not changed because
it assumes that the current `gs` register belongs to the kernel. This would
lead to the kernel handling the fault using a `x86_percpu` structure
controlled by the user and could easily lead to kernel code execution.

Usually, the lowest non-negative non-canonical address is `0x0000800000000000`
(== 1 << 47).  One way that a user process could cause the syscall return
address to be non-canonical is by mapping a 4k executable page immediately
below that address (at `0x00007ffffffff000`), putting a `SYSCALL` instruction
at the end of that page, and executing the `SYSCALL` instruction.

To avoid this problem:

* We disallow mapping a page when the virtual address of the following page
  will be non-canonical.

* We disallow setting the `RIP` register to a non-canonical address using
  [`zx_thread_write_state()`].

For more background, see "A Stitch In Time Saves Nine: A Case Of Multiple
OS Vulnerability", Rafal Wojtczuk
(https://media.blackhat.com/bh-us-12/Briefings/Wojtczuk/BH_US_12_Wojtczuk_A_Stitch_In_Time_WP.pdf).

[`zx_thread_write_state()`]: /docs/reference/syscalls/thread_write_state.md
