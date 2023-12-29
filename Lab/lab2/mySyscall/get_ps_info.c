SYSCALL_DEFINE4(ps_info, pid_t __user*, pid_list, char __user*, comm_list,int __user*, time_list, int __user*, state_list){
	struct task_struct* task;
	int i=0;
	int pers;
	int state;
	printk("[Syscall] ps_info\n");
	for_each_process(task){
		state=(task->state==0);
		pers=(task->se.sum_exec_runtime)/1000;
		copy_to_user(pid_list+i, &task->pid, sizeof(pid_t));
		copy_to_user(comm_list+i*16, &task->comm, 16*sizeof(char));
		copy_to_user(time_list+i, &pers, sizeof(int));
		copy_to_user(state_list+i, &state, sizeof(int));
		i++;
	}
	return 0;
}
