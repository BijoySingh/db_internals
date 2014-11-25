#include <stdio.h>
#include <string.h>
#include "simulator.h"
#include "raid01.h"
#include "raid0.h"

void call(int id, int fd, int pagenum, int read_or_write) {
	int uid = DiskController.fd_to_uid[fd];
	R01_Input(id, read_or_write, uid, pagenum);

	DiskController.list[id].count++;
}

void instruction_executed(int id) {
	DiskController.list[id].count--;

	if (DiskController.list[id].count == 0){
		print(id);
	}
}

int map(char* type, int fd, int pagenum) {
	DiskController.max_id++;
	DiskController.list[DiskController.max_id].fd = fd;
	DiskController.list[DiskController.max_id].op_name = type;
	DiskController.list[DiskController.max_id].pagenum = pagenum;
	DiskController.list[DiskController.max_id].count = 0;
	return DiskController.max_id;
}

void result(int id, int* pagepointer, char** pagebuf, int pagenum, int data) {
	DiskController.list[id].resultpagepointer = pagepointer;
	DiskController.list[id].pagebuf = pagebuf;
	DiskController.list[id].resultpagenum = pagenum;
	DiskController.list[id].data = data;
}

void print(int id) {

	if (strcmp(DiskController.list[id].op_name, "close_file")){
		printf("%s%d\n", "Closed file ", DiskController.list[id].fd);
	}

	else if (strcmp(DiskController.list[id].op_name, "get_first_page")) {
		DiskController.list[id].resultpagepointer = &(DiskController.list[id].resultpagenum);
		DiskController.list[id].pagebuf = &(DiskController.list[id].data);
		printf("%s%d\n", "Fetched first page of file ", DiskController.list[id].fd);
	}

	else if (strcmp(DiskController.list[id].op_name, "get_this_page")) {
		DiskController.list[id].pagebuf = &(DiskController.list[id].data);
		printf("%s%d%s%d\n", "Fetch page ", DiskController.list[id].pagenum, " of file ", DiskController.list[id].fd);
	}

	else if (strcmp(DiskController.list[id].op_name, "alloc_page")) {
		DiskController.list[id].resultpagepointer = &(DiskController.list[id].resultpagenum);
		DiskController.list[id].pagebuf = &(DiskController.list[id].data);
		printf("%s%d\n", "Allocated page for file ", DiskController.list[id].fd);
	}

	else if(strcmp(DiskController.list[id].op_name, "dispose_page"))
		printf("%s%s%s%s\n", "Disposed off page ", DiskController.list[id].pagenum, " of file ", DiskController.list[id].fd);

	else {
		DiskController.list[id].resultpagepointer = &(DiskController.list[id].resultpagenum);
		DiskController.list[id].pagebuf = &(DiskController.list[id].data);
		printf("%s%s%s\n", "Fetched page after ", DiskController.list[id].pagenum, " for file ", DiskController.list[id].fd);
	}
}

void create(char* fname) {
	printf("%d\n",DiskController.max_file);
	int uid = fname_to_uid(fname);

	if (uid == (1 + DiskController.max_file))
		DiskController.max_file++;
	printf("%s %d\n",fname,uid);
	//DiskController.max_file = DiskController.max_file < fd ? fd : DiskController.max_file;
	file_constructor(fname, uid);

	if(DiskController.curr_file[0] == -1)
		DiskController.curr_file[0] = uid;
	else
		DiskController.curr_file[0] = DiskController.curr_file[0] > uid ? uid : DiskController.curr_file[0];
	
	if(DiskController.curr_file[1] == -1)
		DiskController.curr_file[1] = uid;	
	else
		DiskController.curr_file[1] = DiskController.curr_file[1] > uid ? uid : DiskController.curr_file[1];

	printf("NEW CURRY %d\n",DiskController.curr_file[0]);
}

void destroy(char* fname) {
	int uid;	
	for(uid = 0; uid < 1000; uid++){
		if(DiskController.file_structure[uid].valid && (DiskController.file_structure[uid].fname == fname)){

			DiskController.file_structure[uid].valid = false;
			break;
		}
	}
}

void DC_open(char* fname, int fd) {

	DiskController.fd_to_uid[fd] = fname_to_uid(fname);
}

void increment(int fd, int pagenum) {
	int uid = DiskController.fd_to_uid[fd];
	// printf("BEFORE %d %d %d ",uid,pagenum,DiskController.file_structure[uid].pages[pagenum % 2]);
	DiskController.file_structure[uid].pages[pagenum % 2] = DiskController.file_structure[uid].pages[pagenum % 2] < pagenum ? pagenum : DiskController.file_structure[uid].pages[pagenum % 2];
	// printf("AFTER %d\n",DiskController.file_structure[uid].pages[pagenum % 2]);
}

void dispose(int uid, int pagenum) {

	int parity = pagenum%2;
	if (DiskController.file_structure[uid].pages[parity] == pagenum) {
		DiskController.file_structure[uid].pages[parity] -= 2;

		DiskController.file_structure[uid].buffer[parity] = DiskController.file_structure[uid].buffer[parity] <= pagenum ? DiskController.file_structure[uid].buffer[parity] : pagenum;
		DiskController.file_structure[uid].backed_up[parity] = DiskController.file_structure[uid].backed_up[parity] <= pagenum ? DiskController.file_structure[uid].backed_up[parity] : pagenum;

		if ((DiskController.curr_file[parity] == uid) && (DiskController.file_structure[uid].buffer[parity] == DiskController.file_structure[uid].pages[parity]))
			file_increment(parity);

	}
}

void file_increment(int parity) {

	int temp = DiskController.curr_file[parity];
	if(temp == DiskController.max_file){
		printf("HERE\n");
		if(temp!=-1){
		printf("THERE\n");
			R01_BackupComplete();
		}
		DiskController.curr_file[parity] = -1;
		return;
	}
	DiskController.curr_file[parity]++;
	while (!(DiskController.file_structure[DiskController.curr_file[parity]].valid)) {
		if (DiskController.curr_file[parity] == DiskController.max_file) {
			DiskController.curr_file[parity] = -1;

			if (temp != -1)
				R01_BackupComplete();

			break;
		}
			DiskController.curr_file[parity]++;
	}
}

void request_backup(int parity, int disk) {
	if (DiskController.curr_file[parity] == -1)
		return;

	printf("AUTOMATED %d\n",DiskController.curr_file[parity]);

	DiskController.file_structure[DiskController.curr_file[parity]].buffer[parity] += 2;
	R0_Input(DiskController.curr_file[parity], DiskController.file_structure[DiskController.curr_file[parity]].buffer[parity], 0);	//Note that disk is not being used
	
	if (DiskController.file_structure[DiskController.curr_file[parity]].buffer[parity] >= DiskController.file_structure[DiskController.curr_file[parity]].pages[parity]){
		file_increment(parity);
	}
}

void request_forced_backup(int parity, int uid, int pagenum) {
	//int parity = pagenum % 2;
	printf("FORCED %d\n",DiskController.curr_file[parity]);

	if (DiskController.file_structure[uid].buffer[parity] < pagenum)
		;
	else if (DiskController.file_structure[uid].backed_up[parity] < pagenum)
		R0_Input(uid, pagenum, 1);
	else
		R0_Input(uid, pagenum, 2);

	//ignore curr_file < uid || curr_file == uid && curr_file.buffer < pagenum
	//priority 1 curr_file == uid && curr_file.backed_up < uid && curr_file.buffer > uid
	//priority 2
	//if (curr_file < uid || (curr_file == uid && curr_file.backed_up < uid && curr_file.buffer > uid)
}

void confirm_backup(int uid, int pagenum) {
	if (DiskController.file_structure[uid].pages[pagenum % 2] >= pagenum)
		DiskController.file_structure[uid].backed_up[pagenum % 2] = pagenum;
}

void DC_step() {
	if(!DiskController.recovering) {
		if((R01_Step() + R0_Step()) > MAX_BUFFER) {
			fprintf(log_file,"STALL\n");
			DC_step();
		}
	}else{
		fprintf(log_file,"RECOVERING\n");
	}
}

void System_sim_constructor() {
	DiskController.max_id = -1;
	DiskController.max_file = -1;
	DiskController.curr_file[0] = DiskController.curr_file[1] = -1;
	DiskController.recovering = false;
}

int fname_to_uid(char* fname) {
	int i;
	for (i = 0; i <= DiskController.max_file;) {
		if (DiskController.file_structure[i].fname == fname) {
			break;
		}
		i++;
	}

	return i;
}

void file_constructor(char* fname, int uid) {
	DiskController.file_structure[uid].valid = true;
	DiskController.file_structure[uid].pages[0] = -2; DiskController.file_structure[uid].pages[1] = -1;
	DiskController.file_structure[uid].backed_up[0] = -2; DiskController.file_structure[uid].backed_up[1] = -1;
	DiskController.file_structure[uid].buffer[0] = -2; DiskController.file_structure[uid].buffer[1] = -1;
	DiskController.file_structure[uid].fname = fname;
}

void failure() {
	fprintf(log_file,"FAILURE\n");
	int i;

	for(i = 0; i <= DiskController.max_file; i++) {
		DiskController.file_structure[i].backed_up[0] = -2; DiskController.file_structure[i].backed_up[1] = -1;
		DiskController.file_structure[i].buffer[0] = -2; DiskController.file_structure[i].buffer[1] = -1;
	} 
	R0_DeleteBuffer();
	DiskController.recovering = true;
}

void recover() {
	char str[100];
   	char s[1];
   	s[0] = ',';

	while (fgets(str, 60, log_file)) {
		char* token;
	   	char** strings;
		int i = 0;
		token = strtok(str, s);
		strcpy(strings[0], token);
		i++;

		if(strcmp(token, "R01")) {

		while (token != NULL) {
			token = strtok(NULL, s);
			strcpy(strings[i], token);
			i++;
		}

		int uid = atoi(strings[2]);
		int page = atoi(strings[3]);
		int parity = page % 2;
		
		DiskController.file_structure[uid].backed_up[parity] = DiskController.file_structure[uid].backed_up[parity] < page ? page : DiskController.file_structure[uid].backed_up[parity];
		DiskController.file_structure[uid].buffer[parity] = DiskController.file_structure[uid].backed_up[parity];
	}}

	fprintf(log_file,"RECOVERED\n");
	DiskController.recovering = false;
}