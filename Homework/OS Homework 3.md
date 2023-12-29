# Homework 3

> PB20020480 王润泽

##### :one:  List all the requirements of the entry and exit implementation  when solving the critical-section problem. Analyze whether strict  alternation satisfies all the requirements

:a: Requirements:

1. 互斥（Mutual exclusion）：即不同的进程不能同时进入关键区（critical section）
2. 不能提前假设各个进程在关键区内运行的时间，以及使用CPU的个数
3. 有进展的（Progress）:处于关键区之外的进程不应该阻塞其他进程进入关键区
4. 有限等待（Bounded waiting）:不应该出现永久等待的情况



For strict alternation

满足互斥、不限制各个进程速度以及有限等待。但是不满足Progress。

如图

<img src="C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20220420225353601.png" alt="image-20220420225353601" style="zoom:50%;" />

考虑这样一种情况，当Process0 先进行完毕，turn=1,处于remainder section; 这时发生，context switching，Process1运行，执行完毕，turn=0, 进入下一次循环时，Process0仍然处于 remainder section, 此时Process1被阻塞, 不能连续两次进入关键区 。即Progress不满足

##### :two:For Peterson’s solution, prove that it satisfies the three  requirements of a solution for the critical section problem.

:a:Peterson的改进主要是添加了interested[2]的数据结构，用于判断该进程是否需要进入关键区。同时提供一种礼让机制，优先让另一个进程运行。

实现代码

<img src="C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20220420234429904.png" alt="image-20220420234429904" style="zoom:50%;" />

互斥：while(turn == other && interested[other]==TRUE)，保证了，当另一个进程访问关键区时，此进程将一直保持busy的状态。

有进展：比如当进程A即将进入等待区时，此时turn=other成立。如果进程B处于reminder section, 那么其interested[B]的值一定为False；如果B也即将进入等待区，会修改interested[B]的值，两行代码的时间可以忽略不计，此时turn的值被改为A，所以对于进程A，turn=other不成立，A也不会被阻塞，始终有进展。

有限等待：由于一般情况下，critical section 中的代码是很少量的，也只有A进入关键区时，B才会被阻塞，那么就保障了，等待时间是有限的

##### :three:What is deadlock? List the four requirements of deadlock

:a:死锁现象是指：在多任务环境下，多个进程会对有限的资源进行竞争。如果出现了，一些进程至少需要两份资源才能进行任务，而未达到运行条件，保持等待状态，那么可能会出现循环等待，永远无法执行的情况。

Four Requirements

1.互斥

2.一个进程至少占有一份资源同时又在等待其他被占用的资源

3.进程不能抢占已被分配还未释放的资源

4.循环等待

##### :four:Consider the following snapshot of a system:

<img src="C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20220425230129124.png" alt="image-20220425230129124" style="zoom:60%;" />

**Using the banker’s algorithm, determine whether or not each of  the following states is unsafe. If the state is safe, illustrate the  order in which the threads may complete. Otherwise, illustrate  why the state is unsafe**

**a. Available = (2, 2, 2, 3)**

**b. Available = (4, 4, 1, 1)** 

**c. Available = (3, 0, 1, 4)** 

**d. Available = (1, 5, 2, 2)**

:a:

| Need time | A    | B    | C    | D    |
| --------- | ---- | ---- | ---- | ---- |
| $T_0$     | 3    | 1    | 1    | 4    |
| $T_1$     | 2    | 3    | 1    | 2    |
| $T_2$     | 2    | 4    | 1    | 1    |
| $T_3$     | 1    | 4    | 2    | 2    |
| $T_4$     | 2    | 1    | 1    | 1    |

a. is a **safe** state and an order is : T4, T0, T1, T2, T3

*Available = (2, 2, 2, 3)->(3,2,2,4)->(4,4,2,6)->(4,5,3,8)->(5,7,7,8)->(6,9,7,9)*

b. is a **safe** state and an order is : T4, T1, T0, T2, T3

 *Available = (4, 4, 1, 1)->(5,4,1,2)->(5,5,2,4)->(6,7,2,6)->(7,9,6,6)->(8,11,6,7)*

c. is an **unsafe** state. Because it cannot be available for any task' need . Especially for B resources 

d. is a **safe** state and an order is : T3, T1, T2, T0, T4

*Available = (1, 5, 2, 2)->(2, 7, 2, 3)->(2, 8, 3, 5)->(3, 10, 7, 5)->(4, 12, 7, 7)->(5, 12, 7, 8)*

##### :five:What is semaphore? Explain the functionalities of semaphore in  process synchronization.

:a:**信号量**指的是一个共享对象，一般当作integer类型，但在具体使用时，也可以是一个结构体类型，主要用于标识可用资源数的多少。

如下代码：

```c
//entry
void down(semaphore *s){
    //no interrupt
    while(*s==0){
        // enable interrupt
        // sleep(s)
        // no interrupt
    };
    *s=*s-1;
    //enable interrupt
}
//exit
void up(semaphore *s){
    //no interrupt
    if(*s==0)
        //wake();
    *s=*s+1;
    //enable interrupt
}
```

多程序设计中，通过信号量的值来控制是否进程的等待与从等待队列中释放，在Entry和Exit中分别得到应用，因而实现了互斥作用。同时，由于其对可用资源数的计数，可以保证了对进程对资源访问的先后顺序，实现了进程同步。同时在具体函数实现时，又可以避免忙等现象。

例如：有两个先后顺序的任务A、B, 以及两个可以“并行”运行进程P1、P2，共享信号量s，初值为1。这时，在程序中设置顺序

```c
semaphore empty=1, mutex=1, full=0;

down(&empty);
down(&mutex);
A();
up(&mutex);
up(&full);

down(&full);
down(&mutex);
B();
up(&mutex);
up(&empty);
```

这样可以保证在A执行完毕前，B不会被执行, 实现了同步与互斥。



##### :six:Please use semaphore to provide a deadlock-free solution to  address the dining philosopher problem.

:a:下面用C程序简要说明

```c
#define N 5
#define LEFT ((i+N-1)%N)
#define RIGHT ((i+1)%N)

typedef int semaphore;
enum {HUNRGRY=0, EATING, THINKING} state[N];
semaphore mutex = 1;
semaphore s[N] = {0};
//down() 和 up() 与课件实现方式相同
void philosopher(int i){
    think();
    try_take(i);//entry
    eat();//criticalsection
    put_and_wake(i);//exit
}

void try_take(int i){
 	down(&mutex);//互斥锁
    state[i]=HUNGRY;
    printf("[Philosopher %d] Hungry.\n", i + 1);
	test(i);//尝试拿起左右的“筷子”，但这需要检验相邻的哲学家的条件
    up(&mutex);//互斥锁
    down(&s[i]);//检查此时能否进入关键区，如果状态不是1,则说明此时不能EATING,进入等待队列
}

void test(int i){
	if(s[i] == HUNGRY && s[LEFT] != EATING && s[RIGHT] != EATING){
        s[i] == EATING;
        printf("[Philosopher %d] Pickup chopstick %d and %d.\n", i,LEFT,RIGHT);
        up(&s[i])//实现同步，如果i在等待队列，则唤醒
    }
    //else 阻塞
}

void put(int i){
    down(&mutex);
    state[i]=THINkING;
    printf("[Philosopher %d] Putdown chopstick %d and %d.\n", i, LEFT, RIGHY);
	printf("[Philosopher %d] Thinking.\n", i);
    up(&s[LEFT]);//唤醒可能还在在等待的相邻的“哲学家”，保证不会一直阻塞，实现同步
    up(&s[RIGHT]);
    up(&mutex);
}

```



##### :seven:Consider the following set of processes, with the length of the  CPU burst time given in milliseconds:

<img src="C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20220426173324384.png" alt="image-20220426173324384" style="zoom:67%;" />

**The processes are assumed to have arrived in the order P1, P2,  P3, P4, P5, all at time 0**

:a:

a).<img src="D:\ComputerScience\cs_OS_Spring_2022\Operating Systems\homework\h3_p1.png" alt="h3_p1" style="zoom: 33%;" />

b). Turnaround Time

|       | FCFS | SJF  | Priority |  RR  |
| :---: | :--: | :--: | :------: | :--: |
| $P_1$ |  10  |  19  |    16    |  19  |
| $P_2$ |  11  |  1   |    1     |  2   |
| $P_3$ |  13  |  4   |    18    |  7   |
| $P_4$ |  14  |  2   |    19    |  4   |
| $P_5$ |  19  |  9   |    16    |  14  |

c). Waiting Time

|       | FCFS | SJF  | Priority |  RR  |
| :---: | :--: | :--: | :------: | :--: |
| $P_1$ |  0   |  9   |    6     |  9   |
| $P_2$ |  10  |  0   |    0     |  1   |
| $P_3$ |  11  |  2   |    16    |  5   |
| $P_4$ |  13  |  1   |    18    |  3   |
| $P_5$ |  14  |  4   |    1     |  9   |

d). SJF，最短时间优先调度平均等待时间最短

e).

|              | Pros                                   | Cons                                                         |
| ------------ | -------------------------------------- | ------------------------------------------------------------ |
| **FCFS**     | 公平性好                               | 对到达顺序敏感，等待（响应）时间往往很长，性能低; 某种程度上公平性差 |
| **SJF**      | 等待时间短                             | 响应时间可能很长，比如：长时间作业被很多短时间作业阻塞       |
| **Priority** | 根据进程的重要性不同，可以灵活调整顺序 | 固定顺序的优先级，可能导致等待时间过长，导致进程饿死         |
| **RR**       | 整体更加公平，响应时间短，用户体验更好 | 增加了上下文切换的I/O开销，导致CPU-I/O 平衡性减弱            |

##### :eight:Illustrate the key ideas of rate-monotonic scheduling and earliest deadline-first scheduling. Give an example to illustrate under  what circumstances rate-monotonic scheduling is inferior to  earliest-deadline-first scheduling in meeting the deadlines  associated with processes?

:a:

Rate-monotonic scheduling:速率单调调度，定义速率为：1/p（p为周期性时间段period），这样每当有两个进程冲突需要择优时，选择速率最快的进程优先执行。

Earliest-deadline-first scheduling: 依据当前时间到截至时间的用时，决定优先级，即优先执行最近，最紧迫deadline的进程

比如：两个进程同时开始，$p_1=100, p_2=150, t_1=50, t_2=55$,这样Process1比Process2有更高的速率。

在rate-monotonic scheduling 中:t=0, P1 调度 --> t=50, P2调度 --> t=100, P1抢占P2，P1调度--> t=150，P2重新调度，但已过DDL，出现问题

在earliest-deadline-first scheduling中：t=0, P1调度 --> t=50, P2调度 --> t=100, 由于此时P1剩余时间为100，而P2为50，所以P2优先级更高, 不发生切换-->t=105, P1调度。如此往复，保证了任务的完成性
