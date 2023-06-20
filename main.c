#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include "log.h"
#include "pcb.h"


//global var
int shmid;
int turn_id;
int semid;
char *fname = "test.out";

//prototype for signal handler
void sig_handler(int signo);
void kill_shm();
void kill_sem();
void semlock();
void semunlock();

int 
main(int argc, char *argv[]){
	//init var
	int c, hflag, sflag, lflag, iflag, tflag;
	
	//char *fname = "test.out";	//for -l switch, default is logfile.txt
	int option_index = 0;		//for getopt_long
	int err_flag = 0;		//for getopt_long
	char *strbuff = "3";		//handling default value for -n

	char *endptr;			//for strtol
	long value;			//for strtol

	int pid = getpid();		//for pid
	int sig_num;			//for signal
	int temp_pid;

	int i;				//process nth number
	long process_num = 5;		//number of process
	char *max_writes = "3";		//maximum number of writing
	long timer = 20;		//time to terminate
	char text[20];
	int pcb_index;			//index to store new pid in pcb
	char str[256];			

	struct timespec tps;

	//int shmid;
	pcb *p;
	osc *o;

	signal(SIGINT, sig_handler);	//handling signal

	opterr = 0;

	//initialize flags
	hflag = 0;
	sflag = 0;
	lflag = 0;
	iflag = 0;
	tflag = 0;

	//declare long options
	struct option options[] = {
		{"help", 0, 0, 'h'},
		{0,0,0,0}
	};

	//getopt_long will be used to accept long switch
	//short ones will be h,n,l
	if (argc >1){
		while(( c = getopt_long(argc,argv, ":hs:l:i:t:", options,&option_index)) != -1)
			switch (c)
				{
					//-h, --help for information about switch
					case 'h':
						hflag = 1;
						break;
					//-n to print different integer on the logfile
					case 's':
						sflag = 1;

						//strtol to check int or not
						value = strtol(optarg,&endptr, 10);
						if(*endptr !='\0')
							{
								printf("oss : you must type digits after -s, not %s",stderr);
								return 1;
							}
						process_num = value;
						if(process_num > MAXP)
							{
								fprintf(stderr,"oss : cannot spawn more than %s processes\n",MAXP);
								return 1;
							}
						break;
					//-l to change logfile name
					case 'l':
						lflag = 1;
						if(strncmp(optarg,"-",1) == 0)
							{
								perror("oss : you must type file name after -l switch.");
								return 1;
							}
						fname = (char*) malloc(strlen(optarg)+1);
						strcpy(fname,optarg);
						break;
					//-t for timer
					case 't':
						tflag = 1;
						value = strtol(optarg,&endptr,10);
						if(*endptr != '\0')
							{
								printf("oss : you must type digits after -t, not %s",stderr);
								return 1;
							}
						timer = value;
						break;
					//errors in getopt
					case '?':
						err_flag = 1;
						printf("Unknown option : -%c is found\n", optopt);
						break;
				}
	}
	//if errflag is on, just exit
	if (err_flag ==1)
		{
			return 1;
		}

	//if iflag is on
	//print simple definition and then term
	if (hflag == 1)
		{
			printf("this program is designed for testing log library function\n");
			printf("-h --help\n print options\n");
			printf("-s [value]\n to spawn [value] of slave processes\n");
			printf("-l [filename]\n change logfile name to [filename]\n");
			printf("-t [value]\n change the termination timer for oss process to [value] second\n");
			return 0;
		}

	//shmget to create shared memories
	//first one for counting
	//second for the algorithm
        if ((shmid = shmget((key_t)12348888, sizeof(osc), 0600|IPC_CREAT)) == -1)
                {
                        perror("oss : fail to create a shared memory\n");
                        create_log("oss : fail to create a shared memory\n");
                        savelog(fname);
                        return 1;
                }
        else
                {
                        create_log("oss : created a shared memory");
                        savelog(fname);
                        clearlog();
                }

        if ((turn_id = shmget((key_t)88881234, sizeof(pcb)*20, 0600|IPC_CREAT)) == -1)
                {
                        perror("oss : fail to create a shared memory for the peterson algorithm\n");
                        create_log("oss : fail to create a shared memory for the peterson algorithm\n");
                        savelog(fname);
                        return 1;
                }
        else
                {
                        create_log("oss : created a second shared memory");
                        savelog(fname);
                        clearlog();
                }

	//attatching shared memories
        if ((o = (osc *)shmat (shmid, NULL, 0)) == -1)
                {
                        perror("oss : fail to attatch the shared memory\n");
                        create_log("oss : fail to attatch the shared memory");
                        savelog(fname);
                        return 1;
                }
        else
                {
                        create_log("oss : success to attatch the shared memory");
                        savelog(fname);
			clearlog();
                }

        if ((p = (pcb *)shmat(turn_id,NULL,0)) == -1)
                {
                        perror("oss : fail to attatch the second shared memory\n");
                        create_log("oss : fail to attatch the second shared memory");
                        savelog(fname);
                        return 1;
                }
        else
                {
                        create_log("oss : success to attatch the second shared memory");
                        savelog(fname);
			clearlog();
                }

	//semaphore
	if((semid = semget((key_t)11114444,1,0600|IPC_CREAT)) == -1)
		{
			perror("oss : fail to create seamphore\n");
			create_log("oss : fail to create semaphore");
			savelog(fname);
			clearlog();
			exit(1);
		}
	else
		{
			create_log("oss : created semaphore");
			savelog(fname);
			clearlog();
		}

	//init pcb
	int k;
	for (k = 0; k<MAXP; k++)
		{
			p->pid[k] = -1;
			p->flag[k] = 0;
		}

	//assign oss process to 0th slot
	p->pid[0] = pid;
	p->num_proc = process_num;
	p->r_process = 100;
	i = 0;
	
	//init clock
	o->sec = 0;
	o->nsec = 0;

	//init start time in real system clock
	clock_gettime(CLOCK_REALTIME, &tps);

	//forking
	for(k = 1; k<process_num+1; k++)
		{
			pcb_index = k;		//assing k value to pcb_index ;will be used later

			//log
			fflush(stdout);
			create_log("oss : forking a child process");
			savelog(fname);
			clearlog();
			
			//forking
			temp_pid = fork();
			
			//fork result
			if (temp_pid == -1)
				{
					perror("oss : fail to fork a child process\n");
					create_log("oss : fail to fork a child process");
					perror("oss : move to terminating sequence\n");
					create_log("oss : move to terminating sequence\n");
					savelog(fname);
					clearlog();
					
					//kill shm & sem
					kill_shm();
					kill_sem();
					
					perror("oss : oss is terminated due to fail for forking processes\n");
					create_log("oss : oss is terminated due to fail for forking processes");
					savelog(fname);
					return 1;
				}
			else if(temp_pid == 0)
				{
					if (lflag == 1)
						{
							snprintf(text,sizeof(text), "%d", k);
							//
							if((execl("./slave", "slave",text, "-i", max_writes, "-l", fname, NULL)) == -1)
								{
									perror("slave : fail to exec the process image\n");
									create_log("slave : fail to exec the process image");
									perror("process : move to terminating sequence\n");
									create_log("process : move to terminating sequence");
									savelog(fname);
									clearlog();
									
									kill_shm();
									kill_sem();
									
									perror("process : process is terminated due to fail to exec its process image\n");
									create_log("process : process is terminated due to fail for exec its process image");
									savelog(fname);
									return 1;
								}
						}
					else
						{
							snprintf(text,sizeof(text), "%d", k);
							//place for child
							//exec
							if((execl("./slave", "slave",text , "-i", max_writes,"-l",fname, NULL)) == -1)
								{
									//getting error 
									perror("slave : fail to exec the process image\n");
									create_log(" slave: fail to exec the process image");
									perror("process : move to terminating sequence\n");
									create_log("process : move to terminating sequence");
									savelog(fname);
									clearlog();

									kill_shm();
									kill_sem();

									perror("process : process is terminated due to fail to exec its process image\n");
									create_log("process : process is terminated due to fail for exec its process image");
                                                                        savelog(fname);

									return 1;
								}
						}
				}
			else
				{
					//parent process' work
					p->pid[k] = temp_pid;
					p->flag[k] = true;
					p->r_process--;
				}
		}
	//start the timer
	do
		{
			if(p->num_proc < process_num)
				{
					p->num_proc++;
					pcb_index++;
					temp_pid = fork();
					fflush(stdout);
					create_log("oss : forking a child process");
					savelog(fname);
					clearlog();

					if (temp_pid == -1)
						{
                		                        perror("oss : fail to fork a child process\n");
 		                                        create_log("oss : fail to fork a child process");
							perror("oss : move to terminating sequence\n");
							create_log("oss : move to terminating sequence\n");
							savelog(fname);
							clearlog();
							kill_shm();
							kill_sem();
							perror("oss : oss is terminated due to fail for forking processes\n");
							create_log("oss : oss is terminated due to fail for forking processes");

                                        		return 1;
						}
 		                       else if(temp_pid == 0)
                                		{
							if (lflag == 1)
                                                		{
 		                                                       snprintf(text,sizeof(text), "%d", pcb_index);
                                                        
				                                         if((execl("./slave", "slave",text, "-i", max_writes, "-l", fname, NULL)) == -1)
        		                                                        {
                        		                                                perror("slave : fail to exec the process image\n");
                                        		                                create_log("slave : fail to exec the process image");
											perror("process : move to terminating sequence\n");
											create_log("process : move to terminating sequence");

                                                       			                savelog(fname);
											clearlog();
											kill_shm();
											kill_sem();
											perror("process : process is terminated due to fail to exec its process image\n");
											create_log("process : process is terminated due to fail for exec its process image");
											return 1;
                                                                		}
 		                                               }
                		                        else
                               			                 {
 		                                                       snprintf(text,sizeof(text), "%d", pcb_index);

                		                                        if((execl("./slave", "slave",text , "-i", max_writes,"-l",fname, NULL)) == -1)
 		                                                               {
											perror("slave : fail to exec the process image\n");
                                		                                        create_log(" slave: fail to exec the process image");
											perror("process : move to terminating sequence\n");
											create_log("process : move to terminating sequence");

                                                		                        savelog(fname);
											clearlog();
											kill_shm();
											kill_sem();
											perror("process : process is terminated due to fail to exec its process image\n");
											create_log("process : process is terminated due to fail for exec its process image");

                                                                		        return 1;
 		                                                               }
                		                                }
                                		}
 		                        else
                		                {
                                        		//parent's job, record to pcb
							p->pid[pcb_index] = temp_pid;
							p->flag[pcb_index] = true;
							p->r_process--;
                		                }
				}

			//rflag = 1 means there is a new message to read
			if (o->rflag == 1)
				{
					
					//in cs
					semlock();
					struct timespec tpe;
					clock_gettime(CLOCK_REALTIME, &tpe);
					fprintf(stderr,"Master : at my time %d second, %d nano second\n%s ",tpe.tv_sec-tps.tv_sec,tpe.tv_nsec-tps.tv_nsec,o->st);
					snprintf(str,sizeof(str),"oss : at my time %d second, %d nano second\n %s\n",tpe.tv_sec-tps.tv_sec,tpe.tv_nsec-tps.tv_nsec,o->st);
					create_log(str);
					savelog(fname);
					clearlog();
					memset(str,0,sizeof(str));

					o->rflag = 0;
					semunlock();
				}
			//fprintf(stderr, "sec = %d, nsec = %d\n",o->sec,o->nsec);
			//increment 1~10 nanosec
			o->nsec += rand()%5+1;

			//convert every 10000000 nano second to second
			if(o->nsec >= 100000000)
				{
					o->nsec -= 100000000;
					o->sec++;
				}
		}while((timer>o->sec)||(timer==o->sec));

	fprintf(stderr,"oss : exceed the time limit\n");
	fprintf(stderr,"oss : move to terminating sequence\n");
	create_log("oss : exceed the time limit");
	create_log("oss : move to terminating sequence");
	savelog(fname);
	clearlog();
	//do termination
	for (k = 1; k <= pcb_index;k++)
		{
			kill(p->pid[k], SIGKILL);
		}
	kill_shm();
	kill_sem();
	create_log("oss : terminate program");
	savelog(fname);
	return 0;
}

//signal handler
void 
sig_handler(int signo)
{
	fprintf(stderr, "oss : receive a signal to terminate");
	create_log("oss : receive a signal to terminate");
	savelog(fname);
	clearlog();
	kill_shm();
	kill_sem();
	sleep(1);
	exit (1);
}

//function for killing a shared memory
void 
kill_shm()
{
	if((shmctl(shmid, IPC_RMID, 0)) == -1)
		{
			perror("oss : fail to kill the shared memory\n");
			create_log("oss : fail to kill the shared memory");
			savelog(fname);
			clearlog();
		}
	else
		{
			fprintf(stderr,"oss : success to kill the shared memory\n");
			create_log("oss : success to kill the shared memory");
			savelog(fname);
			clearlog();
		}
	if((shmctl(turn_id, IPC_RMID, 0)) == -1)
		{
			perror("oss : fail to kill the second shared memory\n");
			create_log("oss : fail to kill the second shared memory");
			savelog(fname);
			clearlog();
		}
	else
		{
			fprintf(stderr,"oss : success to kill the second shared memory\n");
			create_log("oss : success to kill the second shared memory");
			savelog(fname);
			clearlog();
		}
}

void
kill_sem()
{
	if((semctl(semid, 0, IPC_RMID)) == -1)
		{
			perror("oss : fail to kill semaphore\n");
			create_log("oss : fail to kill semaphore");
			savelog(fname);
			clearlog();
		}
	else
		{
			perror("oss : success to kill the semaphore\n");
			create_log("oss : success to kill the semaphore");
			savelog(fname);
			clearlog();
		}
}

void
semlock()
{
	struct sembuf semo;
	semo.sem_num = 0;
	semo.sem_op = 1;
	semo.sem_flg = 0;
	if((semop(semid, &semo,1)) == -1)
		{
			perror("oss : fail to lock semaphore\n");
			create_log("oss : fail to lock semaphore");
			savelog(fname);
			clearlog();
			exit(1);
		}
}

void
semunlock()
{
	struct sembuf semo;
	semo.sem_num = 0;
	semo.sem_op = -1;
	semo.sem_flg = 0;
	if ((semop(semid,&semo, 1)) == -1)
		{
			perror("oss : fail to operate the semaphore\n");
			create_log("oss : fail to operate the semaphore");
			savelog(fname);
			clearlog();
			exit(1);
		}
}
