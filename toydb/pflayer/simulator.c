#include <stdio.h>
#include <string.h>
#include "simulator.h"
#include "raid01.h"
#include "raid0.h"

void call(int id, int fd, int pagenum, int read_or_write) {
	Raid01SubController.R01_Input(id, read_or_write, fd, pagenum);

	list[id].count++;
}

void instruction_executed(int id) {
	list[id].count--;

	if (list[id].count == 0)
		print(id);
}

int map(char* type, int fd, int pagenum) {
	max_id++;
	list[max_id].fd = fd;
	list[max_id].op_name = type;
	list[max_id].pagenum = pagenum;
	list[max_id].count = 0;

	return max_id;
}

void result(int id, int* pagepointer, char** pagebuf, int pagenum, int data) {
	list[id].resultpagepointer = pagepointer;
	list[id].pagebuf = pagebuf;
	list[id].resultpagenum = pagenum;
	list[id].data = data;
}

void print(int id) {
	if (strcmp(list[id].type, "close_file"))
		printf("%s%s", "Closed file ", list[id].fd);

	else if (strcmp(list[id].type, "get_first_page")) {
		list[id].resultpagepointer = &(list[id].resultpagenumber);
		list[id].pagebuf = &(list[id].data);
		printf("%s%s", "Fetched first page of file ", list[id].fd);
	}

	else if (strcmp(list[id].type, "get_this_page")) {
		list[id].pagebuf = &(list[id].data);
		printf("%s%s%s%s", "Fetch page ", list[id].pagenumber, " of file ", list[id].fd);
	}

	else if (strcmp(list[id].type, "alloc_page")) {
		list[id].resultpagepointer = &(list[id].resultpagenumber);
		list[id].pagebuf = &(list[id].data);
		printf("%s%s", "Allocated page for file ", list[id].fd);
	}

	else
		printf("%s%s%s%s", "Disposed off page ", list[id].pagenumber, " of file ", list[id].fd);
}

void create(char* fname, int fd) {
	file_structure[fd].valid = true;
}

void destroy(char* fname) {
	file_structure[fd].valid = false;
	file_structure[fd].pages[0] = file_structure[fd].backed_up[0] = file_structure[fd].buffer[0] = file_structure[fd].pages[1] = file_structure[fd].backed_up[1] = file_structure[fd].buffer[1] = 0;
}

void increment(int fd, int pagenum) {
	file_structure[fd].pages[pagenum % 2] = max(file_structure[fd].pages[pagenum % 2], pagenum);
}

//void dispose(int fd, int pagenum) {
//
//}

void request_backup(int parity, int disk) {
	file_structure[curr_file[parity]].buffer += 2;
	Raid0SubController.R0_Input(curr_file[parity], curr_file[parity].buffer, 0);	//Note that disk is not being used

	if (file_structure[curr_file[parity]].buffer == file_structure[curr_file[parity]].pages)
		file_increment(parity);
}

void request_forced_backup(int parity, int fd, int pagenum) {
	//int parity = pagenum % 2;

	if (file_structure[fd].buffer[parity] < pagenum)
		;
	else if (file_structure[fd].backed_up[parity] < pagenum)
		Raid0SubController.R0_Input(fd, pagenum, 1);
	else
		Raid0SubController.R0_Input(fd, pagenum, 2);

	//ignore curr_file < fd || curr_file == fd && curr_file.buffer < pagenum
	//priority 1 curr_file == fd && curr_file.backed_up < fd && curr_file.buffer > fd
	//priority 2
	//if (curr_file < fd || (curr_file == fd && curr_file.backed_up < fd && curr_file.buffer > fd)
}

void file_increment(int parity) {
	int temp = curr_file[parity];

	while (!(file_structure[temp].valid))
		curr_file[parity]++;
}

void confirm_backup(int fd, int pagenum) {
	file_structure[fd].backed_up[pagenum % 2] = pagenum;
}

//void step() {
//
//}