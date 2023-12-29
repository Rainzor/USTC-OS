#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/syscall.h>
#define TASK_COMM_LEN 16
#define TOP_SIZE 20
#define CYCLE 180
void sort(pid_t* pid_list, char * comm_list, int *time_list, int* state_list,int size);
int main(void){
    int pnum,last_pnum;
    int time=CYCLE;
    int *time_list;
    int *last_time_list;
    int *runtime_list;
	pid_t *pid_list;
    pid_t *last_pid_list;
	char *comm_list;
   // char *last_comm_list;
    int * state_list;	
	syscall(332, &pnum);//get process num
    pid_list = (int*)malloc((pnum+10)*sizeof(pid_t));
    last_pid_list = (int*)malloc((pnum+10)*sizeof(pid_t));

   // last_comm_list=(char*)malloc(TASK_COMM_LEN*(pnum+10)*sizeof(char));
    comm_list = (char*)malloc(TASK_COMM_LEN*(pnum+10)*sizeof(char));

    time_list= (int*)malloc((pnum+10)*sizeof(int)); //us
    runtime_list= (int*)malloc((pnum+10)*sizeof(int)); //us
    last_time_list= (int*)malloc((pnum+10)*sizeof(int)); //us

    state_list = (int*)malloc((pnum+10)*sizeof(int)); 

    syscall(333, last_pid_list, last_comm_list, last_time_list, state_list);//get the new info             
    while(time){
        sleep(1);
    	system("clear");
    	last_pnum=pnum;
        syscall(332, &pnum);//get process num
       	syscall(333, pid_list, comm_list, time_list, state_list);//get the new info
        for (size_t i = 0; i < pnum; i++){
            if(last_pid_list[i]==pid_list[i])
                runtime_list[i]=time_list[i]-last_time_list[i];
            else{
            	size_t j=0;
            	while(last_pid_list[j]!=pid_list[i] || j!=last_pnum)
            		j++;
            	if(j==last_pnum)
               		runtime_list[i]=-1;// this process won't be print
                else
                	runtime_list[i]=time_list[i]-last_time_list[j];
			}
            last_time_list[i]=time_list[i];
            last_pid_list[i]=pid_list[i];
        }
        sort(pid_list,comm_list,runtime_list, state_list,pnum);
        time--;
    }
	free(pid_list);
    free(last_pid_list);
    free(last_comm_list);
    free(comm_list);
    free(time_list);
    free(state_list);  
    free(last_time_list);
    free(runtime_list); 
	return 0;
}
void sort(pid_t* pid_list, char * comm_list, int *runtime_list, int * state_list,int size){
    int i,j,t;    
    int max_index,index;
    double pers;
    char *command;
    printf("process number is %d\n",size);
    printf("PID              COMMAND     CPU   ISRUNNING\n");
    for(i=0; i<TOP_SIZE;i++){
    	max_index=0;
        for(j=1; j<size; j++)
        	if(runtime_list[j]>runtime_list[max_index]) 
            	max_index=j;     
        index=max_index;
        command=comm_list+index*TASK_COMM_LEN;
        pers=runtime_list[index]/10000.000; //time is of us;
        printf("%-3d %20s %6.2f%%  %10d\n", pid_list[index], command, pers, state_list[index]);  
        runtime_list[index]=-1;            
    }   
}

