# Homework 4

> PB20020480 王润泽

内存管理就是操作系统对内存的划分与动态分配。

##### :one:Explain the following terms：

**Fragmentation fault:**当用户去非法访问内存块，此区域由于特殊的限制（禁止访问或修改），因而触发操作系统的段错误。

:a:

**TLB:**Translation lookaside buffer,本质是页表的缓存，由于页表本身数目过多，所以页表也被存在内存里，这会导致查找物理地址时，需要二次访问内存。所以在硬件上，根据空间局部性原理，将一部分最近访问的物理地址与虚拟地址直接建立一一对应关系，存在TBL中，用于改进虚拟地址到物理地址转换速度的缓存，里面存放的是一些页表文件,文件记录了虚拟地址到物理地址的转换表。

**Page fault:**当CPU真正的访问使用内存时，发现访问的page不在内存中有对应，这时会触发page fault，分配内存。

**Demand paging:**按需调页，即在alloc时只分配虚拟内存，而不是把物理内存也分配，当实际发生页面访问时，通过page fault中断，分配物理内存。

##### :two:Introduce the concept of thrashing, and explain under what circumstance thrashing will happen

:a:

thrashing:抖动现象，频繁的页面置换，导致页面置换的时间长于进程本身执行的时间。

发生的原因：当增加进程数目过多，而每个进程所拥有的页框（page frame）较少时，对某一个具体进程来说，会发生刚刚置换出去的页面又马上需要被访问，这就增加了IO的开销，出现thrashing现象。

##### :three:Consider a paging system with the page table stored in memory

a. If a memory reference takes 50 nanoseconds, how long does a paged memory reference take?

b. If we add TLBs, and 75 percent of all page-table references are found in the TLBs, what is the  effective memory reference time? (Assume that finding a page-table entry in the TLBs takes 2  nanoseconds, if the entry is present.)

:a:

a.由于页表本身也存在内存中，所以想要在内存中获得数据，需要二次查找地址，即总耗时：
$$
\begin{align*}
t=100ns
\end{align*}
$$
b.如果加入TBL，首先会在TBL中查找，如果失败，再在内存中重新查找页表，所以有
$$
\begin{align*}
	t=0.75\times(2+50)+0.25\times(2+50+50)=64.5ns
\end{align*}
$$

##### :four:Assume a program has just referenced an address in virtual memory. Describe a scenario how  each of the following can occur: (If a scenario cannot occur, explain why.)

:a:

case1 TLB miss with no page fault:当TBL中没有缓存对应的虚拟地址，对应的page在内存中。

case2 TLB miss and page fault:当TBL中没有缓存对应的虚拟地址，同时page也不在内存中

case3 TLB hit and no page fault：TBL中缓存了对应的页表项，且内存中存有此页对应的物理地址数据

case4 TLB hit and page fault:不会发生，因为TLB存放的都是实际在**物理内存中存在的页**与虚拟地址对应关系。如果发生了page fault，则说明page不在内存中，那么一开始就不会在TLB中有映射，TLB hit是不会发生的。

##### :five:Assume we have a demand-paged memory. The page table is held in registers. It takes 8  milliseconds to service a page fault if an empty page is available or the replaced page is not  modified, and 20 milliseconds if the replaced page is modified. Memory access time is 100  nanoseconds. Assume that the page to be replaced is modified 70 percent of the time. What is the  maximum acceptable page-fault rate for an effective access time of no more than 200 nanoseconds?

:a:由题意，70%的page fault被用于页面置换，30%被用于初始分配，则设page-fault rate=p
$$
\begin{align*}
0.2\mu s=0.7p\times20000\mu s +0.3p\times8000\mu s+(1-p)\times0.1\mu s
\end{align*}
$$
可得$p=6\times 10^{-6}$

##### :six:Consider the following page reference string: 7, 2, 3, 1, 2, 5, 3, 4, 6, 7, 7, 1, 0, 5, 4, 6, 2, 3, 0, 1. Assuming demand paging with three frames, how many page faults would occur for the following  replacement algorithms?

<img src="D:\ComputerScience\cs_OS_Spring_2022\Operating Systems\homework\p4.jpg" alt="p4" style="zoom:80%;" />

##### :seven:Explain what Belady’s anomaly is, and what is the feature of stack algorithms which never  exhibit Belady’s anomaly?

 Belady’s anomaly:一般来说，增加frames，可以提高算法的容错，减少page-fault次数，但有些算法，增加frames时，可能会出现page-fault反而增加的情况，比如FIFO算法。

而对于stack algorithms则不会出现这种情况，stack algorithms保证了，增加frame后的任意时刻page table中的元素都包含增加frame之前的元素，即$S_n\subset S_{n+1}$,这样可以既提高了容错，又不会出现Belady’s anomaly