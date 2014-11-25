/* testpf.c */
#include <stdio.h>
#include "pf.h"
#include "pftypes.h"
#include "simulator.h"

#define FILE1	"file1"
#define FILE2	"file2"
#define FILE3	"file3"

main()
{
	int error;
	int i;
	int pagenum,*buf;
	int *buf1,*buf2;
	int fd1,fd2;
	int write_count = 0;
	int max_write_count = 5;
	
	log_file = fopen("raid_log.txt","w+");
	System_sim_constructor(); //Initialise the values
	R01_Constructor();	//Initialise the RAID 01 Disk
	R0_Constructor();	//Initialise the RAID 0 Disk
	
	printf("Creating File 1 \n");
	PF_CreateFile(FILE1);
	printf("File1 Created \n");
	
	timestep(5);
	
	PF_CreateFile(FILE2);
	printf("File2 Created \n");
	timestep(5);

	writefile(FILE1);
	readfile(FILE1);

	writefile(FILE2);
	readfile(FILE2);

	int fd_1,fd_2;
	/* open both files */
	fd1=PF_OpenFile(FILE1);
	printf("Opened File1\n");
	timestep(3);

	fd2=PF_OpenFile(FILE2);
	printf("Opened File2\n");
	timestep(3);
	
	/* 
		Get rid of records  1,3,5,.... from file 1
		Get rid of records  0,2,4,.... from file 2
	*/		
	for (i=0; i < PF_MAX_BUFS; i++){
		if (i & 1){
			RAIDPF_DisposePage(fd1,i);
			printf("Disposed Page %d of File1\n",i);
		}
		else {
			RAIDPF_DisposePage(fd2,i);
			printf("Disposed Page %d of File2\n",i);
		}
		if(write_count == 0){
			timestep(1);
		}
		write_count = (write_count + 1)%max_write_count;
	}

	RAIDPF_CloseFile(fd1);
	printf("Special Closed File1\n");

	RAIDPF_CloseFile(fd2);
	printf("Closed File2\n");

	timestep(100);
	R01_Destructor();	//Close the RAID 01 Disk (to flush the log file into raid_log.txt)
}


writefile(fname)
char *fname;
{
int i;
int fd,pagenum;
int *buf;
int error;
int write_count = 0;
int max_write_count = 5;

	fd=PF_OpenFile(fname);
	printf("Opened File %s was Given File Descriptor %d\n",fname,fd);

	for (i=0; i < PF_MAX_BUFS; i++){
		RAIDPF_AllocPage(fd,&pagenum,&buf);
		*((int *)buf) = i;
		printf("Allocated Page %d\n",pagenum);
		if(write_count == 0){
			timestep(1);
		}
		write_count = (write_count + 1)%max_write_count;
	}

	//RAIDPF_AllocPage(fd,&pagenum,&buf);

	for (i=0; i < PF_MAX_BUFS; i++){
		PF_UnfixPage(fd,i,TRUE);
		if(write_count == 0){
			timestep(1);
		}
		write_count = (write_count + 1)%max_write_count;
	}

	printf("Trying To Close File %s\n",fname);
	RAIDPF_CloseFile(fd);
	printf("Closed The File\n");
}


readfile(fname)
char *fname;
{
int error;
int *buf;
int pagenum;
int fd;

	printf("Opening File %s\n",fname);
	fd=PF_OpenFile(fname);
	printf("File %s opened with File Descriptor %d\n",fname,fd);

	printfile(fd);
	RAIDPF_CloseFile(fd);
}

printfile(fd)
int fd;
{
int error;
int *buf;
int pagenum;
int print_count = 0;
int max_print_count = 5;

	printf("Reading File with File Descriptor %d\n",fd);
	
	pagenum = -1;
	while ((error=RAIDPF_GetNextPage(fd,&pagenum,&buf))== PFE_OK){
		printf("\tGot Page %d, %d\n",pagenum,*buf);
		PF_UnfixPage(fd,pagenum,FALSE);
		if(print_count == 0){
			timestep(1);
		}
		print_count = (print_count + 1)%max_print_count;
	}
	printf("File %d EOF Reached\n",fd);
}

timestep(time)
int time;
{
	int i;
	for(i=0;i<time;i++){
		DC_step();
	}
}