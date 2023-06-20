//resubmitting old files
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "log.h"

typedef struct list_struct {
	data_t item;
	struct list_struct *next;
} log_t;

static log_t *headptr = NULL;
static log_t *tailptr = NULL;

//90% same as the program 2.7 in pg44-45
//nothing special, just add a line for perror
int 
addmsg (data_t data) 
{
	log_t *newnode;
	int nodesize;
	
	nodesize = sizeof(log_t) + strlen(data.string) + 1;
	if ((newnode = (log_t *)(malloc(nodesize))) == NULL)
		{
			perror("loglib.c : fail to malloc a newnode\n");
			return -1;
		}
	
	newnode->item.time = data.time;
	newnode->item.string = (char *)newnode + sizeof(log_t);
	strcpy(newnode->item.string, data.string);
	newnode->next = NULL;
	if ( headptr == NULL)
		headptr = newnode;
	else 
		tailptr->next = newnode;
	tailptr = newnode;
	return 0;
}

void 
clearlog (void) 
{
	//decl variable
	log_t *newnode = headptr;

	//free the mem space
	while (newnode != NULL)
		{
			//if(headptr->next != NULL)
				{
					newnode = headptr->next;
					free(headptr);
					headptr = newnode;
				}
			//else
				//free(headptr);
		}
}

char 
*getlog (void)
{
	//declare variables
	//length for the sum of strings, will b used for malloc
	//str to store strings for returning 
	log_t *newnode;
	unsigned int length = 0;
	char *str;
	struct tm *timeinfo;	//for timestamp

	//assign headptr to newnode
	newnode = headptr;

	//check the total length size of the string (sum of)
	//it will be used for malloc the str variable
	while (newnode != NULL)
		{
			timeinfo = localtime(&newnode->item.time);
			length += strlen(asctime(timeinfo)) + 1;
			length += strlen(newnode->item.string) + 1;
			newnode = newnode->next;
		}

	//mem allocation
	str = (char*)malloc(length + 1);
	strcpy(str, "\0");
	//reset the position
	newnode = headptr;

	//now strcat string to the str var for returning
	while (newnode != NULL)
		{
			timeinfo = localtime(&newnode->item.time);
			strcat(str, asctime(timeinfo));
			strcat(str, newnode->item.string);
			strcat(str, "\n");		//add newline
			newnode = newnode->next;
		}

	//if str is NULL, it means "something is wrong!"
	//return NULL
	if (str == NULL)
		{
			perror("loglibl.c : fail to get log msg\n");
			return NULL;
		}
	return str;
}

int 
savelog (char *filename) 
{
	//variable declaration
	FILE *fp;
	log_t *newnode;
	struct tm * timeinfo;

	//file open
	//if opening is failed, return -1
	if((fp = fopen(filename, "a")) == NULL)
		{
			perror("loglib.c : cannot open the log file\n");
			return -1;
		}

	//point to head
	newnode = headptr;

	//append string to the file
	while(newnode != NULL)
		{
			timeinfo = localtime (&newnode->item.time);	//put time stamp
			fputs(asctime(timeinfo),fp);			//as human readable version
			fputs("\t",fp);					
			fputs(newnode->item.string,fp);			//put string
			fputs("\n",fp);
			newnode = newnode->next;					
		}
	
	//close file
	if(fclose(fp) != 0)
		{
			perror("loglib.c : fail to close the log file\n ");
			return -1;
		}
	return 0;
}

//create log function
//this one is not in the temp
//it will receive string then put it into the queue
void 
create_log(char *msg)
{
	//decl variable
	data_t data;

	//put time into data 
	time (&data.time);

	//mem alloc
	data.string = (char*)malloc(strlen(msg) +1);

	//string copy 
	strcpy (data.string, msg);

	//now pass it into addmsg function for rest
	addmsg(data);

	//free memory
	free(data.string);
}
