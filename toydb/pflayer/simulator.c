#include <stdio.h>
#include <string.h>
#include "simulator.h"
#include "raid01.h"
#include "raid0.h"

void call(int id, int fd, int pagenum, int read_or_write) {
	R01_Input(id, read_or_write, fd, pagenum);

	DiskController.list[id].count++;
}

void instruction_executed(int id) {
	DiskController.list[id].count--;

	if (DiskController.list[id].count == 0)
		print(id);
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
	if (strcmp(DiskController.list[id].type, "close_file"))
		printf("%s%s", "Closed file ", DiskController.list[id].fd);

	else if (strcmp(DiskController.list[id].type, "get_first_page")) {
		DiskController.list[id].resultpagepointer = &(DiskController.list[id].resultpagenumber);
		DiskController.list[id].pagebuf = &(DiskController.list[id].data);
		printf("%s%s", "Fetched first page of file ", DiskController.list[id].fd);
	}

	else if (strcmp(DiskController.list[id].type, "get_this_page")) {
		DiskController.list[id].pagebuf = &(DiskController.list[id].data);
		printf("%s%s%s%s", "Fetch page ", DiskController.list[id].pagenumber, " of file ", DiskController.list[id].fd);
	}

	else if (strcmp(DiskController.list[id].type, "alloc_page")) {
		DiskController.list[id].resultpagepointer = &(DiskController.list[id].resultpagenumber);
		DiskController.list[id].pagebuf = &(DiskController.list[id].data);
		printf("%s%s", "Allocated page for file ", DiskController.list[id].fd);
	}

	else
		printf("%s%s%s%s", "Disposed off page ", DiskController.list[id].pagenumber, " of file ", DiskController.list[id].fd);
}

void create(char* fname, int fd) {
	DiskController.file_structure[fd].valid = true;
}

void destroy(char* fname) {
	DiskController.file_structure[fd].valid = false;
	DiskController.file_structure[fd].pages[0] = DiskController.file_structure[fd].backed_up[0] = DiskController.file_structure[fd].buffer[0] = DiskController.file_structure[fd].pages[1] = DiskController.file_structure[fd].backed_up[1] = DiskController.file_structure[fd].buffer[1] = 0;
}

void increment(int fd, int pagenum) {
	DiskController.file_structure[fd].pages[pagenum % 2] = max(DiskController.file_structure[fd].pages[pagenum % 2], pagenum);
}

//void dispose(int fd, int pagenum) {
//
//}

void request_backup(int parity, int disk) {
	DiskController.file_structure[DiskController.curr_file[parity]].buffer += 2;
	R0_Input(DiskController.curr_file[parity], DiskController.curr_file[parity].buffer, 0);	//Note that disk is not being used

	if (DiskController.file_structure[DiskController.curr_file[parity]].buffer == DiskController.file_structure[DiskController.curr_file[parity]].pages)
		file_increment(parity);
}

void request_forced_backup(int parity, int fd, int pagenum) {
	//int parity = pagenum % 2;

	if (DiskController.file_structure[fd].buffer[parity] < pagenum)
		;
	else if (DiskController.file_structure[fd].backed_up[parity] < pagenum)
		R0_Input(fd, pagenum, 1);
	else
		R0_Input(fd, pagenum, 2);

	//ignore curr_file < fd || curr_file == fd && curr_file.buffer < pagenum
	//priority 1 curr_file == fd && curr_file.backed_up < fd && curr_file.buffer > fd
	//priority 2
	//if (curr_file < fd || (curr_file == fd && curr_file.backed_up < fd && curr_file.buffer > fd)
}

void file_increment(int parity) {
	int temp = DiskController.curr_file[parity];

	while (!(DiskController.file_structure[temp].valid))
		DiskController.curr_file[parity]++;
}

void confirm_backup(int fd, int pagenum) {
	DiskController.file_structure[fd].backed_up[pagenum % 2] = pagenum;
}

//void step() {
//
//}
