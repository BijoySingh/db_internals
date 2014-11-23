#ifndef SIMULATOR_H
#define SIMULATOR_H
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
	input list[1000];
	file file_structure[1000];
	int curr_file[2];
	//int backup_file[2];
} System_sim; 

System_sim DiskController;

void call(int id, int fd, int pagenum, int read_or_write);
void instruction_executed(int id);
int map(char* type, int fd, int pagenum);
void result(int id, int* pagepointer, char** pagebuf, int pagenum, int data);
void create(char* fname, int fd);
void destroy(char* fname);
void increment(int fd, int pagenum);
void request_backup(int parity, int disk);
void request_forced_backup(int parity, int fd, int pagenum);
void confirm_backup(int fd, int pagenum);

#endif
