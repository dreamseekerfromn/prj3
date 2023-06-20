#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "log.h"
#include "pcb.h"

//global var
int shmid;
int turn_id;
int semid;
char *fname = "test.out";
int proc_num;

//prototype for signal handler
void sig_handler(int signo);
void kill_shm();
void kill_sem();
void semlock();
void semunlock();

int 
main(int argc, char *argv[]){
	fflush(stdout);
	//init var
	int c, iflag, lflag, err_flag;
	//char *fname = "test.out";	//for -l switch, default is logfile.txt
	char str[256];			//for strtol
	char *strbuff = "3";		//handling default value for -n
	
	char *endptr;			//for strtol
	long value;			//for strtol

	int pid = getpid();		//for pid
	int sig_num;			//for signal

	int k;				//loop
	int i = atoi(argv[1]);		//will store process #
	int max_writing;		//max writes
	int *num;

	int l_sec;			//last access - sec
	int l_nsec;			//last access - nano sec
	int i_sec;			//initial access -sec
	int i_nsec;			//initial access -nano
	int remain_t;			//remaining job

	int option_index = 0;
	opterr = 0;
	iflag = 0;
	lflag = 0;
	err_flag = 0;

	proc_num = i;

	//time info
	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	//get nano second
	struct timespec tps, tpe;

	//seed random
	srand(time(NULL) ^ (getpid()<<16) );

	//int shmid;
	osc *o;
	pcb *p;

	signal(SIGINT, sig_handler);	//handling signal

        //using atoi b/c process always pass the right type. free to use atoi w/o additional checking
        max_writing = atoi(argv[3]);
        fname = (char *)malloc(strlen(argv[5])+1);
        strcpy(fname,argv[5]);

	snprintf(str,sizeof(str),"process %d : exec success, now start the process");
	create_log(str);
	memset(str,0,sizeof(str));
	savelog(fname);        

	//create a shared memory
	if ((shmid = shmget((key_t)12348888, sizeof(int), 0600)) == -1)
		{
			perror("process : fail to get a shared memory\n");
			snprintf(str,sizeof(str),"process %d : fail to get a shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"process %d : got a shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			clearlog();
			memset(str,0,sizeof(str));
		}

	if ((turn_id = shmget((key_t)88881234, sizeof(int)*20, 0600)) == -1)
		{
			perror("process : fail to get a shared memory for the peterson algorithm\n");
			snprintf(str,sizeof(str),"process %d : fail to get a shared memory for the peterson algorithm\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"process%d : get a second shared memory\n", proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			memset(str,0,sizeof(str));
			clearlog();

		}

	//attatching shared memory
	//first shm for data
	if ((o = (osc *)shmat (shmid, NULL, 0)) == -1)
		{
			perror("process : fail to attatch the shared memory\n");
			snprintf(str,sizeof(str),"process %d : fail to attatch the shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"process %d : success to attatch the shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			memset(str,0,sizeof(str));
			clearlog();
		}

	//second for the algorithm
	if ((p = (pcb *)shmat(turn_id,NULL,0)) == -1)
		{
			perror("process : fail to attatch the second shared memory\n");
			snprintf(str,sizeof(str),"process %d : fail to attatch the second shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"process %d : success to attatch the second shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			memset(str,0,sizeof(str));
			clearlog();
		}

	//semaphore
	if((semid = semget((key_t)11114444,1, 0600)) == -1)
		{
			snprintf(str,sizeof(str),"process %d : fail to create semaphore\n",proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			memset(str,0,sizeof(str));
			clearlog();
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"process %d : got the semaphore\n",proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			memset(str,0,sizeof(str));
			clearlog();
		}

	//initialize remainder
	remain_t = rand()%100000+1;

	//in cs, initialize start time
	semlock();
	clock_gettime(CLOCK_REALTIME, &tps);
	l_sec = o->sec;
	l_nsec = o->nsec;
	fprintf(stderr,"process %d (pid : %d) : \n job length = %d \t start time = %d:%d\n\n",i,getpid(),remain_t,l_sec,l_nsec);
	snprintf(str,sizeof(str),"process %d (pid : %d) : \n job length = %d \t start time = %d:%d",i,getpid(),remain_t,l_sec,l_nsec);
	create_log(str);
	savelog(fname);
	clearlog();
	memset(str,0,sizeof(str));
	semunlock();

	while(1)
		{
			semlock();
			//critical section
			i_sec = l_sec;
			i_nsec = l_nsec;

			l_sec = o->sec;
			l_nsec = o->nsec;

			int tmp = (l_sec - i_sec)*100000000 + (l_nsec - i_sec);
			if((remain_t - tmp) <= 0)
				{
					clock_gettime(CLOCK_REALTIME, &tpe);
					snprintf(str,sizeof(str),"process number %d (pid:%d) is going to terminate because it reached oss clock = %d:%d\n\t\t took %d sec, %d nano sec in real time \n\n",i,getpid(),l_sec,l_nsec,tpe.tv_sec-tps.tv_sec,tpe.tv_nsec-tps.tv_nsec);
					//snprintf(str,sizeof(str),"process number %d (pid:%d) is going to terminate because it reached oss clock = %d: %d\n\n",i,getpid(),l_sec,l_nsec);
					memset(o->st,0,sizeof(o->st));
					strcpy(o->st,str);
					o->rflag = 1;
					fflush(stdout);
					create_log(str);
					savelog(fname);
					clearlog();
					semunlock();
					break;
				}
			else
				{
					snprintf(str,sizeof(str),"process number %d (pid:%d) reads oss clock = %d:%d\n\n",i,getpid(),l_sec,l_nsec);
					memset(o->st,0,sizeof(o->st));
					strcpy(o->st,str);
					o->rflag = 1;
					//fprintf(stderr,"%s",o->st);
					fflush(stdout);
					create_log(str);
					memset(str,0,sizeof(str));
					savelog(fname);
					clearlog();

					remain_t -=tmp;			//recalculate remaining job
					//sleep(rand()%2+1);
					semunlock();
					//critical section end
					//remainder
				}
		}

	//terminating, record the result to pcb
	semlock();
	snprintf(str,sizeof(str),"process number %d terminate with osc = %d:%d\n\n",i,l_sec,l_nsec);
	memset(str,0,sizeof(str));
	//strcpy(o->st,str);
	//o->rflag = 1;
	fflush(stdout);
	create_log(str);
	memset(str,0,sizeof(str));
	savelog(fname);
	clearlog();

	p->flag[i] = 0;
	p->sec[i] = l_sec;
	p->nsec[i] = l_nsec;
	p->num_proc--;
	semunlock();

	if(shmdt(o) == -1)
		{
			perror("process : shmdt failed\n");
			snprintf(str,sizeof(str),"process %d : shmdt failed\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"process %d : shmdt success\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}

	savelog(fname);
	fprintf(stderr,"process %d (pid:%d) is terminated successfully\n\n",proc_num,getpid());
	return 0;
}

//signal handler
void 
sig_handler(int signo)
{
	char str[256];
	fprintf(stderr,"process : receive a signal,terminating\n",proc_num);
	snprintf(str,sizeof(str),"process %d : receive a signal, terminating\n",proc_num);
	fflush(stdout);
	create_log(str);
	memset(str,0,sizeof(str));
	kill_shm();
	kill_sem();
	savelog(fname);
	sleep(1);
	exit (1);
}

//function for killing a shared memory
void 
kill_shm()
{
	char str[256];
	if((shmctl(shmid, IPC_RMID, 0)) == -1)
		{
			fprintf(stderr,"process %d : fail to kill the shared memory\n",proc_num);
			snprintf(str,sizeof(str),"process %d : fail to kill the shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}
	else
		{
			fprintf(stderr,"process %d: success to kill the shared memory\n",proc_num);
			snprintf(str,sizeof(str),"process %d : success to kill the shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}
	if((shmctl(turn_id, IPC_RMID, 0)) == -1)
		{
			fprintf(stderr,"process %d: fail to kill the second shared memory\n",proc_num);
			snprintf(str,sizeof(str),"process %d : fail to kill the second shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}
	else
		{
			fprintf(stderr,"process %d: success to kill the second shared memory\n",proc_num);
			snprintf(str,sizeof(str),"process %d : success to kill second shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}
}

void
semlock()
{
	char str[256];
	struct sembuf semo;
	semo.sem_num = 0;
	semo.sem_op = 1;
	semo.sem_flg = 0;
	if((semop(semid, &semo, 1)) == -1)
		{
			fprintf(stderr,"process %d : fail to lock semaphore\n",proc_num);
			snprintf(str,sizeof(str),"process %d : fail to lock semaphore\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
			exit(1);
		}
	else
		{
                        snprintf(str,sizeof(str),"process %d : success to lock\n",proc_num);
                        fflush(stdout);
                        create_log(str);
                        memset(str,0,sizeof(str));
                        savelog(fname);
                        clearlog();

		}
}

void
semunlock()
{
	char str[256];
	struct sembuf semo;
	semo.sem_num = 0;
	semo.sem_op = -1;
	semo.sem_flg = 0;
	if((semop(semid,&semo, 1)) == -1)
		{
			fprintf(stderr,"process %d : fail to unlock semaphore\n",proc_num);
                        snprintf(str,sizeof(str),"process %d : fail to unlock seamphore",proc_num);
                        fflush(stdout);
                        create_log(str);
                        memset(str,0,sizeof(str));
                        savelog(fname);
                        clearlog();
			exit(1);
		}
	else
		{
                        snprintf(str,sizeof(str),"process %d : success to unlock semaphore\n",proc_num);
                        fflush(stdout);
                        create_log(str);
                        memset(str,0,sizeof(str));
                        savelog(fname);
                        clearlog();
		}
}

void
kill_sem()
{
	char str[256];
	if((semctl(semid, 0, IPC_RMID)) == -1)
                {
			fprintf(stderr,"process %d : fail to kill semaphore\n",proc_num);
                        snprintf(str,sizeof(str),"process %d : fail to kill semaphore\n",proc_num);
                        fflush(stdout);
                        create_log(str);
                        memset(str,0,sizeof(str));
                        savelog(fname);
                        clearlog();
                }
        else
                {
                        snprintf(str,sizeof(str),"process %d : success to kill semaphore\n",proc_num);
                        fflush(stdout);
                        create_log(str);
                        memset(str,0,sizeof(str));
                        savelog(fname);
                        clearlog();

                }
}
