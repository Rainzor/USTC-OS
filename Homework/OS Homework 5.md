# OS Homework 5

> PB20020480 王润泽
>

**:one:Consider a RAID organization comprising five disks in total, how many blocks are accessed in order to perform the following operations for RAID-5 and RAID-6?**

**a. An update of one block of data** 

**b. An update of seven continuous blocks of data. Assume that the seven contiguous blocks begin at a boundary of a stripe.**

:a:

a. 只更新一个块

对RAID-5 进行更新和异或操作,更新$A_1,A_p$

$ RMW：A_p'= A_p\oplus A_1\oplus A_1'$，只要读$A_1,A_p$，要写$A_1',A_p'$，只访问两个块

$ RRW：A_p'= A_1’\oplus A_2\oplus A_3\oplus A_4$,要访问5个块

对RAID-6 进行更新和异或操作，分别更新$A_1,A_p,A_q$,

$ RMW：A_p'= A_p\oplus A_1\oplus A_1',A_q'= A_q\oplus c^1A_1\oplus c^1A_1'$,共访问3个块

$ RRW：A_p'= A_1’\oplus A_2\oplus A_3,A_p'= c^1A_1’\oplus c^2A_2\oplus c^3A_3$,共访问5个块

b.更新连续的7个块：

对RAID-5 进行更新和异或操作,即Ap,A1,A2,A3,A4,Bp,B1,B2,B3

$RMW: A_p'= A_p\oplus[\oplus_i^4 (A_i\oplus A_i')], B_p'= B_p\oplus[\oplus_i^3 (B_i\oplus B_i')]$访问5+4个块，即9个

$ RRW:A_p'= A_1’\oplus A_2'\oplus A_3'\oplus A_4',B_p'= B_1’\oplus B_2'\oplus B_3'\oplus B_4$共访问5+5=10个块

对RAID-6 进行更新和异或操作,即Ap, Aq, A1, A2, A3； Bp, Bq, B1, B2, B3；Cp, Cq, C1

同理RMW, 要访问5+5+3=13个块

RRW，要访问5+5+5=15个块



**:two:Suppose that a disk drive has 5,000 cylinders, numbered 0 to 4999. The drive is currently serving  a request at cylinder 2150, and the previous request was at cylinder 1805. The queue of pending  requests, in FIFO order, is: 2069, 1212, 2296, 2800, 544, 1618, 356, 1523, 4965, 3681 Starting  from the current head position, what is the total distance (in cylinders) that the disk arm moves to  satisfy all the pending requests for each of the following disk-scheduling algorithms?**

:a:

从题干上看，应该是若是扫描，应该先以顺序读取的顺序。

FCFS: 2150-2069+2069-1212+2296-1212+2800-2296+2800-544+1618-544+1618-356+1523-356+4965-1523+4965-3681=13011

SSTF: 最短距离优先顺序：2150，2069，2296，2800，3681，4965，1618，1523，1212，544，356。总路程：7586

SCAN: 扫描顺序：2150，2296，2800，3681，4965，4999，2069，1618，1523，1212，544，356。总路程：7492

LOOK:不扫到结尾处：2150，2296，2800，3681，4965，2069，1618，1523，1212，544，356。总路程：7424

C-SCAN: 只顺序循环扫描：2150，2296，2800，3681，4965，4999，0，356，544，121，1523，1618，2069。总路程：9917

C-Look：不扫到结尾的循环扫描：2150，2296，2800，3681，4965，356，544，121，1523，1618，2069。总路程：9137



**:three:Explain what open-file table is and why we need it.**

:a:Open-file Table本质是一个对已打开文件属性（元数据）的缓存，在内核空间里存储文件的属性，形成一个系统层级的已打开文件信息表。

作用：当我们要再次访问，读写文件时，可以避免再次在磁盘中寻找文件位置，而是借助open-file table缓存，直接获取文件索引，减少在外存目录遍历，达到减少IO开销的作用。同时也能获得文件当前状态，以保证文件读写的安全性。



**:four:What does “755” mean for file permission?**

:a:先将八进制的755，改成2进制：111_101_101

对应的是文件拥有者(file's owner)，可读可写可执行

文件所属组的其他用户（file's group）和组外其他用户（others），可读可执行，不可写



**:five:Explain the problems of using continuous allocation for file system layout and how to solve them.**

:a:当仅仅使用连续分配的文件空间布局时，容易形成外部碎片化，空间利用率低下；且文件的增长较为困难。

解决方式：主要是将文件内部切片化，并通过FAT表来索引查找文件内部数据的位置，这样可以解决外部碎片化与文件增长问题



**:six:What are the advantages of the variation of linked allocation that uses a FAT to chain together the  blocks of a file? What is the major problem of FAT?**

:a:Advantages: 使用FAT表可以动态管理文件内部数据，当把FAT表缓存到内存中后，通过寻找文件内某位置的指针，在内存能够快速查找定位文件内部数据位置，相当于实现了随机访问的，提高了IO的效率。

Problem：当把整个FAT表缓存到内存中时，相当于空间换时间，但由于程序访问空间局部性，大部分缓存空间有浪费。



**:seven:Consider a file system similar to the one used by UNIX with indexed allocation, and assume that  every file uses only one block. How many disk I/O operations might be required to read the  contents of a small local file at /a/b/c in the following two cases? Should provide the detailed  workflow**

**a. Assume that none of the disk blocks and inodes is currently being cached.**

**b. Assume that none of the disk blocks is currently being cached but all inodes are in memory**

:a:

a. 当磁盘数据与inodes不在内存中缓存时，则遍历访问文件要经过内存与磁盘的IO有：

1，访问root文件目录找到/a在inode序号；

2，访问inodes, 找到/a位置；

3，访问/a对应的数据块，得到a/b的inode序号；

4，访问inodes,  找到/a/b位置；

5，访问a/b序号数据块，得到a/b/c的inode序号；

6，访问inodes, 找到a/b/c文件位置；

7，访问数据库，得到a/b/c文件内容

共7次IO

b.当inodes缓存在内存中时，就避免了从外存中访问inodes，减少了IO开销，即没有了2，4，6步骤，所以共4次IO



**:eight:Consider a file system that uses inodes to represent files. Disk blocks are 8-KB in size and a pointer  to a disk block requires 4 bytes. This file system has 12 direct disk blocks, plus single, double,  and triple indirect disk blocks. What is the maximum size of a file that can be stored in this file  system?**

:a:一个disk block，可以存储$2^{13}/2^2=2^{11}=2048$个指针，文件系统有12个直接索引，1个一级索引，1个二级索引，1个三级索引。

所以可以计算得：$12\times8KB+2^{11}\times8KB+2^{22}\times8KB+2^{33}\times8KB\approx2^{33}\times2^3B=64TB$



**:nine:What is the difference between hard link and symbolic link?**

:a:本质是以不同的方式指向同一文件

**硬链接**：

在文件目录中创建一个新的目录条目，拥有新的文件名称，指向一个已存在的文件。

但inode号不改变，并不会创建新的文件内容。

对应inode中的link count要加1，相当于此文件拥有两个文件路径。

**符号链接**：

在文件目录中创建一个新的目录条目，拥有新的文件名称和inode号。

inode指向的数据块中存储着已存在文件的路径，可以快速访问到该文件

已存在文件的inode中link count不改变。



**:keycap_ten: What is the difference between data journaling and metadata journaling? Explain the operation  sequence for each of the two journaling methods.**

:a:先说一下二者的操作顺序：

数据日志：1.往数据日志中写入TXB，元数据，数据；

​					2，往数据日志中写入TXE，标识提交日志；

​					3.向文件系统中写入元数据和数据；

​					4，释放数据日志表内容

元数据日志：1往文件系统中写入数据，往元数据日志中写入TXB,元数据；

​						2，往元数据日志中写入TXE，标识提交日志；

​						3，向文件系统中写入元数据；

​						4，释放元数据日志内容。

区别：1，数据日志数据写了两次，元数据日志数据写了一次。

​			2，数据日志是先写日志提交后再写文件系统；元数据日志先将数据写入文件系统，元数据写入日志，日志提交后才将元数据写入文件系统，不写数据



**11.What are the three I/O control methods?**

轮询，中断，直接内存访问（DMA）

**12.What services are provided by the kernel I/O subsystem?**

I/O调度，缓冲，缓存，Spooling假脱机，错误处理与IO保护，电源管理等等

