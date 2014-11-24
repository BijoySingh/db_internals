/* raid0.h for RAID 0 */

#define RAID_READ 0
#define RAID_WRITE 1
#define MAX_ARRAY 1000
#define BACKUP_THRESHOLD 750
#define BACKUP_LOWER_THRESHOLD 500


/****************************************************
*****************************************************
	DEBUG START
*****************************************************
*****************************************************/

typedef struct WriteEntry{
	int pagenum;
	int file_descriptor;
} WriteEntry;

typedef struct RAID0
{
	//Even Buffer
	struct WriteEntry buffer_even[MAX_ARRAY];	
	int buffer_even_count; 
	int buffer_even_start;
	WriteEntry even_priority;

	//Odd Buffer
	struct WriteEntry buffer_odd[MAX_ARRAY];	
	int buffer_odd_count; 
	int buffer_odd_start;
	WriteEntry odd_priority;

} RAID0; //Raid0SubController


/****************************************************
*****************************************************
	DEBUG STOP
*****************************************************
*****************************************************/

RAID0 Raid0SubController;

void R0_constructor(){
	int i = 0;	
	for (;i<MAX_ARRAY;i++){
		Raid0SubController.buffer_even[i].pagenum = -1;
		Raid0SubController.buffer_odd[i].pagenum = -1;
		Raid0SubController.buffer_even[i].file_descriptor = -1;
		Raid0SubController.buffer_odd[i].file_descriptor = -1;
	}
	Raid0SubController.buffer_odd_count = 0;
	Raid0SubController.buffer_even_count = 0;
	Raid0SubController.buffer_even_start = 0;
	Raid0SubController.buffer_odd_start = 0;

	Raid0SubController.even_priority.pagenum = -1;
	Raid0SubController.odd_priority.pagenum = -1;
	Raid0SubController.even_priority.file_descriptor = -1;
	Raid0SubController.odd_priority.file_descriptor = -1;
}

void R0_AddEvenEntry(int fd,int pagenum){
	WriteEntry bfe;
	bfe.pagenum = pagenum;
	bfe.file_descriptor = fd;
	Raid0SubController.buffer_even[
		(Raid0SubController.buffer_even_start + 
			Raid0SubController.buffer_even_count) % MAX_ARRAY] = bfe;
	Raid0SubController.buffer_even_count += 1;
}
void R0_AddOddEntry(int fd,int pagenum){
	WriteEntry bfe;
	bfe.pagenum = pagenum;
	bfe.file_descriptor = fd;
	Raid0SubController.buffer_odd[
		(Raid0SubController.buffer_odd_start + 
			Raid0SubController.buffer_odd_count) % MAX_ARRAY] = bfe;
	Raid0SubController.buffer_odd_count += 1;
}

void R0_AddModifyEntry(int fd,int page){
	//printf("Modified Entry Of %d,%d in Buffer\n",fd,page);
}

void R0_Input(int fd,int pagenum,int priority){
	if(pagenum % 2 == 0 && priority == 0)
		R0_AddEvenEntry(fd,pagenum);
	else
		R0_AddOddEntry(fd,pagenum);

	if(pagenum % 2 == 0 && priority == 1)
		R0_AddModifyEntry(fd,pagenum);
	else
		R0_AddModifyEntry(fd,pagenum);
	
	if(pagenum % 2 == 0 && priority == 2){
		Raid0SubController.even_priority.file_descriptor = fd;
		Raid0SubController.even_priority.pagenum = pagenum;
	}
	else{
		Raid0SubController.odd_priority.file_descriptor = fd;
		Raid0SubController.odd_priority.pagenum = pagenum;	
	}	
}

void R0_Backup(int file,int pagenum){
	fprintf(log_file, "R0,W,%d,%d,%d,%d\n",file,pagenum,pagenum%2,pagenum%2);
	printf("RAID 0 : Granted Access To Write(Backup) To Disk %d For %d,%d\n",pagenum%2,file,pagenum);
}

void R0_Step(){
	//EVEN
	if(Raid0SubController.even_priority.file_descriptor != -1){
		//backup the priority
		R0_Backup(Raid0SubController.even_priority.file_descriptor,Raid0SubController.even_priority.pagenum);
		//reset priority
		Raid0SubController.even_priority.file_descriptor = -1;
		Raid0SubController.even_priority.pagenum = -1;					
	}else{
		WriteEntry bf_even = Raid0SubController.buffer_even[Raid0SubController.buffer_even_start];
		if(bf_even.file_descriptor != -1){
			//backup the first
			R0_Backup(bf_even.file_descriptor,bf_even.pagenum);
			//send message to controller
			confirm_backup(bf_even.file_descriptor,bf_even.pagenum);
			//update variables
			Raid0SubController.buffer_even_start = (Raid0SubController.buffer_even_start + 1) % MAX_ARRAY;
			Raid0SubController.buffer_even_count -= 1;
		}
	}

	//ODD
	if(Raid0SubController.odd_priority.file_descriptor != -1){
		//backup the priority
		R0_Backup(Raid0SubController.odd_priority.file_descriptor,Raid0SubController.odd_priority.pagenum);
		//reset priority
		Raid0SubController.odd_priority.file_descriptor = -1;
		Raid0SubController.odd_priority.pagenum = -1;					
	}else{
		WriteEntry bf_odd = Raid0SubController.buffer_odd[Raid0SubController.buffer_odd_start];
		if(bf_odd.file_descriptor != -1){
			//backup the first
			R0_Backup(bf_odd.file_descriptor,bf_odd.pagenum);
			//send message to controller
			confirm_backup(bf_odd.file_descriptor,bf_odd.pagenum);
			//update variables
			Raid0SubController.buffer_odd_start = (Raid0SubController.buffer_odd_start + 1) % MAX_ARRAY;
			Raid0SubController.buffer_odd_count -= 1;
		}
	}	
}
