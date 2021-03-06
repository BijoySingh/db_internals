/* raid01.h for RAID 01 */

#define RAID_READ 0
#define RAID_WRITE 1
#define BACKUP_THRESHOLD  (int)MAX_BUFFER*0.8
#define BACKUP_LOWER_THRESHOLD (int)MAX_BUFFER*0.5

#define DISK_00 0
#define DISK_01 1
#define DISK_10 2
#define DISK_11 3

//FORMAT: RAID,INSTRUCTION(R,W,BR),FD,PAGE,ODD_EVEN,DISK

/****************************************************	
	EACH BUFFER LOCATION CAN STORE THIS DATA
****************************************************/

typedef struct BufferEntry{
	int id;
	int pagenum;
	int read_or_write;
	int file_descriptor;
} BufferEntry;

/****************************************************
	RAID01 SUB CONTROLLER CLASS
****************************************************/

typedef struct RAID01
{
	bool backup_disk_attached;
	bool backup_complete;

	//Even Buffer
	struct BufferEntry buffer_even[MAX_ARRAY];	
	int buffer_even_count; int buffer_even_start;
	bool buffer_even_backup_mode; // = true;

	//Odd Buffer
	struct BufferEntry buffer_odd[MAX_ARRAY];	
	int buffer_odd_count; int buffer_odd_start;
	bool buffer_odd_backup_mode; // = true;
} RAID01;

RAID01 Raid01SubController;
void R01_Destructor(){
	fclose(log_file);
}
void R01_Constructor(){
	int i;
	for(i =0;i<MAX_ARRAY;i++){
		Raid01SubController.buffer_even[i].id=-1;
		Raid01SubController.buffer_even[i].pagenum=-1;
		Raid01SubController.buffer_even[i].file_descriptor=-1;
		Raid01SubController.buffer_even[i].read_or_write=-1;
		Raid01SubController.buffer_odd[i].id=-1;
		Raid01SubController.buffer_odd[i].pagenum=-1;
		Raid01SubController.buffer_odd[i].file_descriptor=-1;
		Raid01SubController.buffer_odd[i].read_or_write=-1;
	}
	Raid01SubController.buffer_even_count = 0;
	Raid01SubController.buffer_odd_count = 0;
	Raid01SubController.buffer_even_start = 0;
	Raid01SubController.buffer_odd_start = 0;
	Raid01SubController.buffer_even_backup_mode = true;
	Raid01SubController.buffer_odd_backup_mode = true;

	Raid01SubController.backup_disk_attached = false;
	Raid01SubController.backup_complete = false;
}

void R01_ActivateBackup(){
	Raid01SubController.backup_disk_attached = true;
}

void R01_BackupComplete(){
	printf("Backup Complete Requested\n");
	Raid01SubController.backup_complete = true;	
}

/****************************************************
INSERTING ENTRIES INTO BUFFERS
****************************************************/

void R01_AddEvenEntry(int id,int read_or_write,int fd,int pagenum){
	BufferEntry bfe;
	bfe.id = id;
	bfe.pagenum = pagenum;
	bfe.file_descriptor = fd;
	bfe.read_or_write = read_or_write;
	Raid01SubController.buffer_even[
		(Raid01SubController.buffer_even_start + 
			Raid01SubController.buffer_even_count) % MAX_ARRAY] = bfe;
	Raid01SubController.buffer_even_count += 1;
	if(Raid01SubController.buffer_even_count > BACKUP_THRESHOLD)
		Raid01SubController.buffer_even_backup_mode = false;
}

void R01_AddOddEntry(int id,int read_or_write,int fd,int pagenum){
	BufferEntry bfe;
	bfe.id = id;
	bfe.pagenum = pagenum;
	bfe.file_descriptor = fd;
	bfe.read_or_write = read_or_write;
	Raid01SubController.buffer_odd[
		(Raid01SubController.buffer_odd_start + 
			Raid01SubController.buffer_odd_count) % MAX_ARRAY] = bfe;
	Raid01SubController.buffer_odd_count += 1;
	if(Raid01SubController.buffer_odd_count > BACKUP_THRESHOLD)
		Raid01SubController.buffer_odd_backup_mode = false;
}

void R01_Input(int id,int read_or_write,int fd,int pagenum){
	if(pagenum % 2 == 0)
		R01_AddEvenEntry(id,read_or_write,fd,pagenum);
	else
		R01_AddOddEntry(id,read_or_write,fd,pagenum);
}


/****************************************************
	PERFORM BACKUP
****************************************************/

void R01_PerformBackup(int even_or_odd,int disk){
	printf("RAID 01 : Granted Request To Backup From Disk %d\n",disk);
	fprintf(log_file, "R01,BR,-1,-1,%d,%d\n",even_or_odd,disk);
	request_backup(even_or_odd,disk);
}
void R01_PerformForcedBackup(int even_or_odd,int file,int page){
	printf("RAID 01 : Request Forced Backup From Disk \n");
	fprintf(log_file, "R01,BR,%d,%d,%d,%d\n",file,page,even_or_odd,-1);
	request_forced_backup(even_or_odd,file,page);
}

/****************************************************
	UPDATE BUFFER PARAMETERS AFTER STEPPING
****************************************************/

void R01_UseBuffer(int even_or_odd){
	if(even_or_odd == 0){
		Raid01SubController.buffer_even[Raid01SubController.buffer_even_start].read_or_write = -1;
		Raid01SubController.buffer_even_start = (Raid01SubController.buffer_even_start+1)%MAX_ARRAY;	
		Raid01SubController.buffer_even_count -= 1;
		if(Raid01SubController.buffer_even_count < BACKUP_LOWER_THRESHOLD){
			Raid01SubController.buffer_even_backup_mode = true;
		}
	}else{
		Raid01SubController.buffer_odd[Raid01SubController.buffer_odd_start].read_or_write = -1;
		Raid01SubController.buffer_odd_start = (Raid01SubController.buffer_odd_start+1)%MAX_ARRAY;	
		Raid01SubController.buffer_odd_count -= 1;
		if(Raid01SubController.buffer_odd_count < BACKUP_LOWER_THRESHOLD){
			Raid01SubController.buffer_odd_backup_mode = true;
		}
	}
}
/****************************************************
	PERFORM AN INSTRUCTION ON DISK
****************************************************/

void R01_PerformInstruction(BufferEntry buffer_entry,int even_or_odd,
	int write_or_read,int disk_number){
	instruction_executed(buffer_entry.id);
	if(write_or_read == 0){
		printf("RAID 01 : Granted Request To Read From Disk %d\n",disk_number);
		fprintf(log_file, "R01,R,%d,%d,%d,%d\n",
			buffer_entry.file_descriptor,
			buffer_entry.pagenum,even_or_odd,disk_number);
	}
	if(write_or_read == 1){
		fprintf(log_file, "R01,W,%d,%d,%d,%d\n",
			buffer_entry.file_descriptor,
			buffer_entry.pagenum,even_or_odd,disk_number);
		printf("RAID 01 : Granted Request To Write To Disks %d and %d\n",disk_number,disk_number+2);
	}
}

/****************************************************
	PERFORM STEP TO DO ALL THE THINGS
****************************************************/

int R01_Step(){
	fprintf(log_file,"STEP\n");
	//EVEN

	/**************************************************
		ALGORITHM
		-> count != 0 check
		-> if instruction is READ
		  -> Perform the current instruction
		  -> if I cant backup(not yet backing up/backup full/fully backed up)
		  	->if next is read, perform read
		  	->else backup anyways if I can(not fully backed up backup)
		  -> else backup
		-> if write, write and do forced backup
		-> else do double backup	
	***************************************************/

	if(Raid01SubController.buffer_even_count != 0){

	BufferEntry bf_even = Raid01SubController.buffer_even[Raid01SubController.buffer_even_start];
	if(bf_even.read_or_write == RAID_READ){
		//FIRST INSTRUCTION IS READ
		//Perform Read
		R01_PerformInstruction(bf_even,0,RAID_READ,DISK_00);
		
		if(!Raid01SubController.buffer_even_backup_mode || !Raid01SubController.backup_disk_attached || Raid01SubController.backup_complete){
			//CANT BACKUP
			BufferEntry bf_even_next = Raid01SubController.buffer_even[(Raid01SubController.buffer_even_start+1)%MAX_ARRAY];
			if(bf_even_next.read_or_write == RAID_READ){
				//NEXT INSTRUCTION IS READ
				//Perform Read
				R01_PerformInstruction(bf_even_next,0,RAID_READ,DISK_10);
				R01_UseBuffer(0);
			}else if(Raid01SubController.backup_disk_attached && !Raid01SubController.backup_complete){
				//NEXT INSTRUCION CANT BE READ
				//do even backup from here
				R01_PerformBackup(0,DISK_10);							
			}
		}else{
			//do even backup from here
			if(!Raid01SubController.backup_complete)
				R01_PerformBackup(0,DISK_10);
		}
		//Reseting variables
		R01_UseBuffer(0);
	}else if(bf_even.read_or_write == RAID_WRITE){
		//Perform Write
		R01_PerformInstruction(bf_even,0,RAID_WRITE,DISK_00);
		//request to backup this data
		if(Raid01SubController.backup_disk_attached)
			R01_PerformForcedBackup(0,bf_even.file_descriptor,bf_even.pagenum);
		//Reseting variables
		R01_UseBuffer(0);
	}else{
		//Send 2 backups
		if(Raid01SubController.backup_disk_attached && !Raid01SubController.backup_complete){
			R01_PerformBackup(0,DISK_00);
			R01_PerformBackup(0,DISK_10);
		}
	}}else{
		if(Raid01SubController.backup_disk_attached && !Raid01SubController.backup_complete){
			printf("BACKUPPP%d\n",Raid01SubController.backup_complete);
			R01_PerformBackup(0,DISK_00);
			R01_PerformBackup(0,DISK_10);
		}
	}

	//ODD
	if(Raid01SubController.buffer_odd_count != 0){
	BufferEntry bf_odd = Raid01SubController.buffer_odd[Raid01SubController.buffer_odd_start];
		if(bf_odd.read_or_write == RAID_READ){
		//FIRST INSTRUCTION IS READ
		//Perform Read
		R01_PerformInstruction(bf_odd,1,RAID_READ,DISK_01);
		
		if(!Raid01SubController.buffer_odd_backup_mode || !Raid01SubController.backup_disk_attached || Raid01SubController.backup_complete){
			//CANT BACKUP
			BufferEntry bf_odd_next = Raid01SubController.buffer_odd[(Raid01SubController.buffer_odd_start+1)%MAX_ARRAY];
			if(bf_odd_next.read_or_write == RAID_READ){
				//NEXT INSTRUCTION IS READ
				//Perform Read
				R01_PerformInstruction(bf_odd_next,0,RAID_READ,DISK_11);
				//reset variables
				R01_UseBuffer(1);
			}else if(Raid01SubController.backup_disk_attached && !Raid01SubController.backup_complete){
				//NEXT INSTRUCION CANT BE READ
				//do odd backup from here
				R01_PerformBackup(1,DISK_11);
			}
		}else{
			//do odd backup from here
			if(!Raid01SubController.backup_complete)
				R01_PerformBackup(1,DISK_11);
		}
		R01_UseBuffer(1);
	}else if(bf_odd.read_or_write == RAID_WRITE){
		//Perform Write
		R01_PerformInstruction(bf_odd,1,RAID_WRITE,DISK_01);
		//request to backup this data
		if(Raid01SubController.backup_disk_attached){
			R01_PerformForcedBackup(1,bf_odd.file_descriptor,bf_odd.pagenum);
		}
		//Reseting variables
		R01_UseBuffer(1);
	}
	else{
		//Send 2 backups
		if(Raid01SubController.backup_disk_attached && !Raid01SubController.backup_complete){
			R01_PerformBackup(1,DISK_01);
			R01_PerformBackup(1,DISK_11);
		}
	}}else{
		//Send 2 backups
		if(Raid01SubController.backup_disk_attached && !Raid01SubController.backup_complete){
			R01_PerformBackup(1,DISK_01);
			R01_PerformBackup(1,DISK_11);
		}
	}
	return Raid01SubController.buffer_odd_count + Raid01SubController.buffer_even_count;
}

