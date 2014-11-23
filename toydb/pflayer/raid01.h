/* raid01.h for RAID 01 */

#define RAID_READ 0
#define RAID_WRITE 1
#define MAX_ARRAY 1000
#define BACKUP_THRESHOLD 750
#define BACKUP_LOWER_THRESHOLD 500

#define DISK_00 0
#define DISK_01 1
#define DISK_10 2
#define DISK_11 3

// SystemSimulator

/****************************************************	
	EACH BUFFER LOCATION CAN STORE THIS DATA
****************************************************/

struct BufferEntry{
	int id = -1;
	int pagenum = -1;
	int read_or_write = -1;
	int file_descriptor = -1;
};

/****************************************************
	RAID01 SUB CONTROLLER CLASS
****************************************************/

struct RAID01
{
	//Even Buffer
	struct BufferEntry buffer_even[MAX_ARRAY];	
	int buffer_even_count; int buffer_even_start;
	bool buffer_even_backup_mode = true;

	//Odd Buffer
	struct BufferEntry buffer_odd[MAX_ARRAY];	
	int buffer_odd_count; int buffer_odd_start;
	bool buffer_even_backup_mode = true;
} Raid01SubController;


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
	DiskController.request_backup(even_or_odd,disk);
}
void R01_PerformForcedBackup(int even_or_odd,int file,int page){
	DiskController.request_forced_backup(even_or_odd,file,page);
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
	PERFORM AN INSTRUTION ON DISK
****************************************************/

void R01_PerformInstruction(id,even_or_odd,write_or_read,disk_number){
	DiskController.instruction_executed(id);
	if(write_or_read == 0)
		printf("RAID 01 : Granted Request To Read From Disk %d\n",disk_number);
	if(write_or_read == 1)
		printf("RAID 01 : Granted Request To Write To Disks %d and %d\n",disk_number,disk_number+2);
}

/****************************************************
	PERFORM STEP TO DO ALL THE THINGS
****************************************************/

void R01_Step(){
	//EVEN
	BufferEntry bf_even = Raid01SubController.buffer_even[Raid01SubController.buffer_even_start];
	if(bf_even.read_or_write == RAID_READ){
		//FIRST INSTRUCTION IS READ
		//Perform Read
		R01_PerformInstruction(bf_even.id,0,RAID_READ,DISK_00);
		
		if(!buffer_even_backup_mode){
			//CANT BACKUP
			BufferEntry bf_even_next = Raid01SubController.buffer_even[(Raid01SubController.buffer_even_start+1)%MAX_ARRAY];
			if(bf_even_next.read_or_write == RAID_READ){
				//NEXT INSTRUCTION IS READ
				//Perform Read
				R01_PerformInstruction(bf_even_next.id,0,RAID_READ,DISK_10);
				//reset variables
				R01_UseBuffer(0);
				R01_UseBuffer(0);
			}else{
				//NEXT INSTRUCION CANT BE READ
				//do even backup from here
				R01_PerformBackup(0,DISK_10);
				//reset variables
				R01_UseBuffer(0);			
			}
		}else{
			//do even backup from here
			R01_PerformBackup(0,DISK_10);
			//Reseting variables
			R01_UseBuffer(0);
		}
	}else if(bf_even.read_or_write == RAID_WRITE){
		//Perform Write
		R01_PerformInstruction(bf_even.id,0,RAID_WRITE,DISK_00);
		//request to backup this data
		R01_PerformForcedBackup(0,bf_even.file_descriptor,bf_even.pagenum);
		//Reseting variables
		R01_UseBuffer(0);
	}else{
		//Send 2 backups
		R01_PerformBackup(0,DISK_00);
		R01_PerformBackup(0,DISK_10);
	}

	//ODD
		if(bf_odd.read_or_write == RAID_READ){
		//FIRST INSTRUCTION IS READ
		//Perform Read
		R01_PerformInstruction(bf_odd.id,1,RAID_READ,DISK_01);
		
		if(!buffer_odd_backup_mode){
			//CANT BACKUP
			BufferEntry bf_odd_next = Raid01SubController.buffer_odd[(Raid01SubController.buffer_odd_start+1)%MAX_ARRAY];
			if(bf_odd_next.read_or_write == RAID_READ){
				//NEXT INSTRUCTION IS READ
				//Perform Read
				R01_PerformInstruction(bf_odd_next.id,0,RAID_READ,DISK_11);
				//reset variables
				R01_UseBuffer(1);
				R01_UseBuffer(1);
			}else{
				//NEXT INSTRUCION CANT BE READ
				//do odd backup from here
				R01_PerformBackup(1,DISK_11);
				//reset variables
				R01_UseBuffer(1);			
			}
		}else{
			//do odd backup from here
			R01_PerformBackup(1,DISK_11);
			//Reseting variables
			R01_UseBuffer(1);
		}
	}else if(bf_odd.read_or_write == RAID_WRITE){
		//Perform Write
		R01_PerformInstruction(bf_odd.id,1,RAID_WRITE,DISK_01);
		//request to backup this data
		R01_PerformForcedBackup(0,bf_odd.file_descriptor,bf_odd.pagenum);
		//Reseting variables
		R01_UseBuffer(1);
	}
	else{
		//Send 2 backups
		R01_PerformBackup(1);
		R01_PerformBackup(1);
	}

}
