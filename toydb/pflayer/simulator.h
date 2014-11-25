#ifndef SIMULATOR_H
#define SIMULATOR_H

#define MAX_ARRAY 1000
#define MAX_BUFFER 70
#include<stdbool.h>

typedef struct input {
	int fd;
	char* op_name;
	int pagenum;
	int resultpagenum;
	int* resultpagepointer;
	int count;
	char* data;
	char** pagebuf;
} input;

typedef struct file {
	bool valid;
	int pages[2];
	int backed_up[2];
	int buffer[2];
	char* fname;
} file;

typedef struct System_sim {
	int max_id;
	int max_file;
	input list[1000];
	file file_structure[1000];	//indexed by uid
	int fd_to_uid[1000];
	int curr_file[2];
	bool recovering;
	//int backup_file[2];
} System_sim; 

System_sim DiskController;

void call(int id, int fd, int pagenum, int read_or_write);
void instruction_executed(int id);
int map(char* type, int fd, int pagenum);
void result(int id, int* pagepointer, char** pagebuf, int pagenum, int data);
void create(char* fname);
void destroy(char* fname);
void increment(int fd, int pagenum);
void request_backup(int parity, int disk);
void request_forced_backup(int parity, int uid, int pagenum);
void confirm_backup(int uid, int pagenum);
void System_sim_constructor();
void file_constructor(char* fname, int uid);
void failure();
void recover();

FILE* log_file;
#endif
