// if a /SOUNDS/ folder exists, call fat_count_files before anything else.  
// then fat_write_file_data(), fat_write_file_entry() can be called to create new files in /SOUNDS/

#include "LPC21xx.h"
#include "interface.h"
#include "system.h"
#include "simple_fat.h"
#include "sd_raw.h"
#include "printf.h"

extern unsigned char sdBuf[];
extern unsigned int sfs_size;  // in bytes
extern unsigned int sfs_start;  // in sectors
extern unsigned int sfs_audio_start;  // in sectors
extern unsigned int sfs_audio_size;  // in bytes

// these are addresses in sectors
unsigned int addr_boot_sector = 0;
unsigned int addr_FAT_1 = 0;
unsigned int addr_FAT_2 = 0;
unsigned int addr_root_dir = 0;
unsigned int addr_data_start = 0;
unsigned int addr_sounds_directory = 0;

// fat params
unsigned int total_sectors = 0;
unsigned int num_reserved_sectors = 0;
unsigned int sectors_per_fat = 0;
unsigned int sectors_per_cluster = 0;
unsigned int num_root_entries = 0;
unsigned int num_fats = 0;
unsigned int audio_data_max_space = 0;

// keep track of which sector of the fat is being updated
unsigned char current_fat_sector_cache[512]; 
unsigned int current_fat_sector_offset = 0; 
unsigned int first_update = 1;

// keep track of the current file that is being written
unsigned int file_write_sector_location = 0;
unsigned int file_write_cluster_location = 0;
unsigned int file_write_size = 0;
unsigned int file_write_start_cluster = 0;

//    
unsigned int total_files = 0;

static void fat_write_wav_header(unsigned int sector, unsigned int size);
// smooths end of a recording
static void smooth_end(unsigned int sector);

// Look for 'FAT16' to find boot sector.  it will be at offset 0x36
void fat_locate_boot_sector (void) {
	  unsigned int i = 0;

// TODO:  make sure we error out if the block (sector) size is not 512....
    // initialize mem card
    // error out if there is a problem
    if (sd_raw_init())
        printf("card found \n\r");
    else
        error_out(NO_CARD_ERROR); // card not found error

    printf("searching for boot sector\r\n");

    for (i =0; i < 1024; i++){    // search 1024 sectors
        sd_raw_read(i * 512, sdBuf, 512);
        if ((sdBuf[54] == 'F') && (sdBuf[55] == 'A') && (sdBuf[56] == 'T') && (sdBuf[57] == '1') && (sdBuf[58] == '6')){
              printf("FAT16 boot sector found at sector: %d\r\n",  i);
              addr_boot_sector = i;
              return;
        }	
    }
    error_out(NO_FAT_ERROR);
}

void fat_get_partition_info(void){
	
	sd_raw_read(addr_boot_sector * 512, sdBuf, 512);

	sectors_per_cluster = sdBuf[0xd];
	num_reserved_sectors = sdBuf[0xe];
	num_fats = sdBuf[0x10];
	num_root_entries = sdBuf[0x11] | (sdBuf[0x12] << 8);
	sectors_per_fat = sdBuf[0x16] | (sdBuf[0x17] << 8);
	total_sectors = sdBuf[0x20] | (sdBuf[0x21] << 8) | (sdBuf[0x22] << 16) | (sdBuf[0x23] << 24);

	addr_FAT_1 = addr_boot_sector + num_reserved_sectors;
	addr_FAT_2 = addr_FAT_1 + sectors_per_fat;
	addr_root_dir = addr_FAT_2 + sectors_per_fat;
	addr_data_start = addr_root_dir + ((num_root_entries * 32) / 512);

	printf("address FAT 1: %d\r\n", addr_FAT_1);
	printf("address FAT 2: %d\r\n", addr_FAT_2);
	printf("address root dir: %d\r\n", addr_root_dir);
	printf("address  data start: %d\r\n", addr_data_start);
	printf("number sectors per cluster: %d\r\n", sectors_per_cluster);
	printf("number reserved sectors: %d\r\n", num_reserved_sectors);
	printf("number of FAT: %d\r\n", num_fats);
	printf("number root entries: %d\r\n", num_root_entries);
	printf("number sectors per FAT: %d\r\n", sectors_per_fat);
	printf("total sectors in data area: %d \r\n", total_sectors - (addr_data_start - addr_boot_sector));
	printf("total sectors in partition: %d \r\n", total_sectors);
    printf("disk size: %d M\r\n", (sectors_per_fat * sectors_per_cluster * 512 * 256));
  
    //TODO: some arithmetic to check fat checks out, and return error if not
	total_sectors = total_sectors - num_reserved_sectors - (num_fats * sectors_per_fat) - ((num_root_entries * 32) / 512);

    // the sounds directory table
    // starts one cluster into data region, data region starts at sector 2, so this is sector 3
    addr_sounds_directory = addr_data_start + sectors_per_cluster;

    // use this for max available space. take away 2 sectors of clusters from the fat table, so it never even gets close to filled up 
    audio_data_max_space = (sectors_per_fat - 2) * 256 * sectors_per_cluster * 512;
	
    printf("total calculated sectors in data area: %d \r\n", total_sectors);
    printf("audio data max space: %d \r\n", audio_data_max_space);
}

void fat_initialize_disk(void){
	unsigned int i = 0;
	for (i = 0; i < 512; i++)
		sdBuf[i] = 0;
	// erase FATs and root directory
	for (i = addr_FAT_1; i < addr_data_start; i++){
	   sd_raw_write(i * 512, sdBuf, 512);
  }
	printf("initialized\r\n");
}

// creates sounds directory
void fat_make_sounds_directory (void){
	unsigned int i = 0;

    for (i = 0; i < 512; i++) {
        sdBuf[i] = 0;
    }
	// make entry in root directory
	sdBuf[0] = 'S';
	sdBuf[1] = 'O';
	sdBuf[2] = 'U';
	sdBuf[3] = 'N';
	sdBuf[4] = 'D';
	sdBuf[5] = 'S';
	sdBuf[6] = 0x20;
	sdBuf[7] = 0x20;
	
    sdBuf[8] = 0x20;
	sdBuf[9] = 0x20;
	sdBuf[10] = 0x20; 
    sdBuf[11] = 0x10;   // sub direc
    sdBuf[12] = 0x0;  
    sdBuf[13] = 0x0;  // time
    sdBuf[14] = 0x0;
    sdBuf[15] = 0x0;

    sdBuf[16] = 0x0;
	sdBuf[17] = 0x0;
	sdBuf[18] = 0x0;
	sdBuf[19] = 0x0;
	sdBuf[20] = 0;
	sdBuf[21] = 0;
	sdBuf[22] = 0x0;
	sdBuf[23] = 0x0;
	
    sdBuf[24] = 0x0;
	sdBuf[25] = 0x0;   
	sdBuf[26] = 3;    // cluster low
    sdBuf[27] = 0;         // cluster high
    sdBuf[28] = 0;
    sdBuf[29] = 0;
    sdBuf[30] = 0;
    sdBuf[31] = 0;

    // make directory entry
	sd_raw_write(addr_root_dir * 512, sdBuf, 512);

	// now write directory, add dot entries, zero out rest
    for (i = 0; i < 512; i++) {
        sdBuf[i] = 0;
    }
    // . points to self
    sdBuf[0] = 0x2e;
	sdBuf[1] = 0x20;
	sdBuf[2] = 0x20;
	sdBuf[3] = 0x20;
	sdBuf[4] = 0x20;
	sdBuf[5] = 0x20;
	sdBuf[6] = 0x20;
	sdBuf[7] = 0x20;
    sdBuf[8] = 0x20;
	sdBuf[9] = 0x20;
	sdBuf[10] = 0x20; 
    sdBuf[11] = 0x10;   // sub direc
	sdBuf[26] = 3;    // cluster low
    sdBuf[27] = 0;         // cluster high
    // .. points to parent
    sdBuf[0 + 32] = 0x2e;
	sdBuf[1 + 32] = 0x2e;
	sdBuf[2 + 32] = 0x20;
	sdBuf[3 + 32] = 0x20;
	sdBuf[4 + 32] = 0x20;
	sdBuf[5 + 32] = 0x20;
	sdBuf[6 + 32] = 0x20;
	sdBuf[7 + 32] = 0x20;
    sdBuf[8 + 32] = 0x20;
	sdBuf[9 + 32] = 0x20;
	sdBuf[10 + 32] = 0x20; 
    sdBuf[11 + 32] = 0x10;   // sub direc
	sdBuf[26 + 32] = 0;    // cluster low
    sdBuf[27 + 32] = 0;         // cluster high
            
	sd_raw_write((addr_sounds_directory) * 512, sdBuf, 512);
 
    // zero directory out rest of directory
    for (i = 0; i < 512; i++) {
        sdBuf[i] = 0;
    }
    for (i = 1; i < (sectors_per_cluster * 4); i++) {
	    sd_raw_write((addr_sounds_directory + i) * 512, sdBuf, 512);
    }

	// now fill in fat
    for (i = 0; i < 512; i++) {
        sdBuf[i] = 0x00;   
    }

    // put the correct stuff in clusters 0, 1, 2
    sdBuf[0] = 0xf8; 
    sdBuf[1] = 0xff;
    sdBuf[2] = 0xff;
    sdBuf[3] = 0xff;
    sdBuf[4] = 0x00;
    sdBuf[5] = 0x00;

    // use 4 clusters for directory = 2048 files
    sdBuf[6] = 4;
    sdBuf[7] = 0;

    sdBuf[8] = 5;
    sdBuf[9] = 0;

    sdBuf[10] = 6;
    sdBuf[11] = 0;

    sdBuf[12] = 0xFF;
    sdBuf[13] = 0xFF;

    // write fat
    sd_raw_write((addr_FAT_1) * 512, sdBuf, 512);
    sd_raw_write((addr_FAT_2) * 512, sdBuf, 512);

	printf("wrote directory\r\n");

}

// TODO:  probably better to error out, and have user create the SOUNDS directory by holding down all three buttons
void fat_check_for_sounds_directory() {
    // check if we have SOUNDS as first item of root, if not, create 
	sd_raw_read(addr_root_dir * 512, sdBuf, 512);
    // ever heard of strcmp()  ???? 
    if ((sdBuf[0] != 'S') ||
        (sdBuf[1] != 'O') ||
        (sdBuf[2] != 'U') ||
        (sdBuf[3] != 'N') ||
        (sdBuf[4] != 'D') ||
        (sdBuf[5] != 'S') ||
        (sdBuf[6] != 0x20) ||
        (sdBuf[7] != 0x20) ||
        (sdBuf[8] != 0x20) ||
        (sdBuf[9] != 0x20) ||
        (sdBuf[10] != 0x20))
    {
        printf("SOUNDS directory not found.... \n\r");
    //    fat_make_sounds_directory();
        error_out(NO_SFS_ERROR);
    }
    else {
        printf("SOUNDS directory found\r\n");
    }

    // check that FAT table has 4 clusters allocated, if not, repair
    sd_raw_read((addr_FAT_1) * 512, sdBuf, 512);
    if ((sdBuf[6] != 4) ||
        (sdBuf[7] != 0) ||
        (sdBuf[8] != 5) ||
        (sdBuf[9] != 0) ||
        (sdBuf[10] != 6) ||
        (sdBuf[11] != 0) ||
        (sdBuf[12] != 0xff) ||
        (sdBuf[13] != 0xff))
    {
        printf("not enough space allocated for SOUNDS directory, updating fat...\n\r");
     
        // put the correct stuff in clusters 0, 1, 2
        sdBuf[0] = 0xf8; 
        sdBuf[1] = 0xff;
        sdBuf[2] = 0xff;
        sdBuf[3] = 0xff;
        sdBuf[4] = 0x00;
        sdBuf[5] = 0x00;

        // use 4 clusters for directory = 2048 files
        sdBuf[6] = 4;
        sdBuf[7] = 0;

        sdBuf[8] = 5;
        sdBuf[9] = 0;

        sdBuf[10] = 6;
        sdBuf[11] = 0;

        sdBuf[12] = 0xFF;
        sdBuf[13] = 0xFF;

        // write fat
        sd_raw_write((addr_FAT_1) * 512, sdBuf, 512);
        sd_raw_write((addr_FAT_2) * 512, sdBuf, 512);
   }
   else {
        printf("Sounds directory has 4 clusters\r\n");
   }
}

// check if anything else is in the root folder, blow it away if so
void fat_clean_root_directory(void) {
    unsigned int i,j = 0;
    
    // check first sector, the first entry should be the sounds folder
	sd_raw_read(addr_root_dir * 512, sdBuf, 512);
   
    for (i = 1; i < 16; i++) {
        if (sdBuf[32 * i] != 0) {
            printf("junk in root directory, trashing\r\n");
            for (j = 32; j < 512; j++) {
                sdBuf[j] = 0;
            }
	        sd_raw_write(addr_root_dir * 512, sdBuf, 512);
            break;
        }
    }
    // TODO: check and zero remaining sectors of root directory
}

unsigned int fat_count_files (void){
    unsigned int i,j = 0;
    unsigned int last_file_first_cluster = 0;
    unsigned int last_file_last_cluster = 0;
    unsigned int last_file_first_sector = 0;
    unsigned int last_file_last_sector = 0;
    unsigned int last_file_size = 0;

    unsigned int sector_offset = 0;
    unsigned int entry_offset = 0;


    total_files = 0;
   
    // assume sounds directory is TODO: HOW MANY cluster, 4 for now
    // TODO break this loop if 'S' is not found (we can only deal with continuous lists of S000... files)
    for (i = 0; i < (sectors_per_cluster * 4); i++){
	    sd_raw_read((addr_sounds_directory + i) * 512, sdBuf, 512);
        for (j = 0; j < 16; j++){
              if (sdBuf[j * 32] == 'S') // sound files are named like 'S0000001.wav'
                  total_files++;
        }
    }
    
    // determine next available cluster and sector so we are ready to write more files
    if (total_files){
     
        // first 2 entries are dot entries, and total files points to next available, so last file is at entry total_files + 1
        sector_offset = (total_files + 1) >> 4;               // 16 entries per sector
        entry_offset = ((total_files + 1) & 0xf ) << 5;     // 32 bytes per entry
   
	    sd_raw_read((sector_offset + addr_sounds_directory) * 512, sdBuf, 512);
   
        // bytes 26 and 26 are the first cluster of file
        last_file_first_cluster = (sdBuf[entry_offset + 27] << 8) | sdBuf[entry_offset + 26];
       
        // find first sector of file, subtract 2 data area starts at sector 2
        last_file_first_sector = ((last_file_first_cluster - 2) * sectors_per_cluster) + addr_data_start;
        
        last_file_size = sdBuf[0x1c + entry_offset] & 0xff;
	    last_file_size |= (sdBuf[0x1d + entry_offset] & 0xff) << 8;
	    last_file_size |= (sdBuf[0x1e + entry_offset] & 0xff) << 16;
	    last_file_size |= (sdBuf[0x1f + entry_offset] & 0xff) << 24;

        last_file_last_sector = (last_file_size / 512) + last_file_first_sector;
        last_file_last_cluster = ((last_file_size / 512) / sectors_per_cluster) + last_file_first_cluster;
       
        file_write_cluster_location = last_file_last_cluster + 1;
        file_write_sector_location = ((file_write_cluster_location - 2) * sectors_per_cluster) + addr_data_start ;
        file_write_size = 0;
        file_write_start_cluster = file_write_cluster_location;
    }
    // if no files, start the new one at cluster 10
    else {
        file_write_cluster_location = 10;
        // remember that 10 points to 8 since data starts at cluster 2
        file_write_sector_location = (8 * sectors_per_cluster) + addr_data_start;
        file_write_size = 0;
        file_write_start_cluster = file_write_cluster_location;
        // and create first file
        for (i = 0; i < 512; i++) sdBuf[i] = 0; // fill it with 0
        fat_write_file_data();
        fat_write_file_data();
        fat_write_file_data();
        fat_write_file_data();
        fat_write_file_entry();
    }

    printf("last file cluster start: %d , last file cluster end: %d, last file size %d\r\n", last_file_first_cluster, last_file_last_cluster, last_file_size);
    printf("file write sector: %d , file write cluster: %d\r\n", file_write_sector_location, file_write_cluster_location);

    return total_files;
}

unsigned int fat_get_file_start_address(unsigned int file_num){
    
    unsigned int file_first_cluster = 0;
    unsigned int file_first_sector = 0;

    unsigned int sector_offset = 0;
    unsigned int entry_offset = 0;

    // first 2 entries are dot entries
    sector_offset = (file_num + 2) >> 4;               // 16 entries per sector
    entry_offset = ((file_num + 2) & 0xf ) << 5;     // 32 bytes per entry

    sd_raw_read((sector_offset + addr_sounds_directory) * 512, sdBuf, 512);

    // bytes 26 and 26 are the first cluster of file
    file_first_cluster = (sdBuf[entry_offset + 27] << 8) | sdBuf[entry_offset + 26];
   
    // find first sector of file, subtract 2 data area starts at sector 2
    file_first_sector = ((file_first_cluster - 2) * sectors_per_cluster) + addr_data_start;
   
    return file_first_sector * 512;   
}

unsigned int fat_get_file_end_address(unsigned int file_num){
     
    unsigned int file_first_cluster = 0;
    unsigned int file_first_sector = 0;
    unsigned int file_last_sector = 0;
    unsigned int file_size = 0;

    unsigned int sector_offset = 0;
    unsigned int entry_offset = 0;

    // first 2 entries are dot entries
    sector_offset = (file_num + 2) >> 4;               // 16 entries per sector
    entry_offset = ((file_num + 2) & 0xf ) << 5;     // 32 bytes per entry

    sd_raw_read((sector_offset + addr_sounds_directory) * 512, sdBuf, 512);

    // bytes 26 and 26 are the first cluster of file
    file_first_cluster = (sdBuf[entry_offset + 27] << 8) | sdBuf[entry_offset + 26];
   
    // find first sector of file, subtract 2 data area starts at sector 2
    file_first_sector = ((file_first_cluster - 2) * sectors_per_cluster) + addr_data_start;
    
    file_size = sdBuf[0x1c + entry_offset] & 0xff;
    file_size |= (sdBuf[0x1d + entry_offset] & 0xff) << 8;
    file_size |= (sdBuf[0x1e + entry_offset] & 0xff) << 16;
    file_size |= (sdBuf[0x1f + entry_offset] & 0xff) << 24;

    file_last_sector = (file_size / 512) + file_first_sector;

    return file_last_sector * 512;
}

// has to be called after fat_count_files
unsigned int fat_get_used_space(void) {
    return (file_write_sector_location - addr_data_start) * 512;
}

unsigned int fat_get_total_space(void) {
    return audio_data_max_space;
}

unsigned int fat_get_num_files(void) {
    return total_files;
}


// TODO : make all these functions return errors
void fat_write_file_data(void) {
 
    //TODO:  check file_write_sector_location is in data area

    // write sector with data in sdBuf
    sd_raw_write(file_write_sector_location * 512, sdBuf, 512);
   
    // check to see if the sector is in new cluster
    if ((((file_write_sector_location - addr_data_start) / sectors_per_cluster) + 2) > file_write_cluster_location ) {
        fat_update_fat(file_write_cluster_location, file_write_cluster_location + 1);
        file_write_cluster_location++;
    }
    file_write_sector_location++;
    file_write_size += 512;
}

// creates file entry
void fat_write_file_entry (void){
    unsigned char fname[11];
    unsigned int sector_offset = 0;
    unsigned int entry_offset = 0;

    // TODO error if this gets over limit
    // finish off cluster, increment current cluster
    fat_update_fat(file_write_cluster_location, 0xFFFF); 
    fat_flush_fat();
   
    // write wav header at start of file
    fat_write_wav_header(((file_write_start_cluster - 2) * sectors_per_cluster) + addr_data_start, file_write_size);

    // smooth end of file
    smooth_end(file_write_sector_location - 1);  // this is pointing to next available, so subtract 1 to get last of this file

    sprintf(fname, "S%07dWAV", total_files); 

    // determine entry location. first 2 entries are dot entries
    sector_offset = (total_files + 2) >> 4;               // 16 entries per sector
    entry_offset = ((total_files + 2) & 0xf ) << 5;     // 32 bytes per entry
   
    // read in the info
	sd_raw_read((sector_offset + addr_sounds_directory) * 512, sdBuf, 512);

	// make entry
	sdBuf[0 + entry_offset] = fname[0];
	sdBuf[1 + entry_offset] = fname[1];
	sdBuf[2 + entry_offset] = fname[2];
	sdBuf[3 + entry_offset] = fname[3];
	sdBuf[4 + entry_offset] = fname[4];
	sdBuf[5 + entry_offset] = fname[5];
	sdBuf[6 + entry_offset] = fname[6];
	sdBuf[7 + entry_offset] = fname[7];
	sdBuf[8 + entry_offset] = fname[8];
	sdBuf[9 + entry_offset] = fname[9];
	sdBuf[10 + entry_offset] = fname[10];
	sdBuf[0x1a + entry_offset] = file_write_start_cluster & 0xff;
	sdBuf[0x1b + entry_offset] = (file_write_start_cluster >> 8) & 0xff;

	sdBuf[0x1c + entry_offset] = file_write_size & 0xff;   // file size
	sdBuf[0x1d + entry_offset] = (file_write_size >> 8) & 0xff;
	sdBuf[0x1e + entry_offset] = (file_write_size >> 16) & 0xff;
	sdBuf[0x1f + entry_offset] = (file_write_size >> 24) & 0xff;

    // update directory table
	sd_raw_write((sector_offset + addr_sounds_directory) * 512, sdBuf, 512);

    // update params for next file, increment cluster
    file_write_cluster_location++;
    // start writing 1 sector into file, first sector will get wave header added
    file_write_sector_location = ((file_write_cluster_location - 2) * sectors_per_cluster) + addr_data_start + 1;
    file_write_size = 512;
    file_write_start_cluster = file_write_cluster_location;

  //  printf("wrote file %s\r\n", fname);
    total_files++;
}

// update a entry in the fat table
// sector being updated is cached
unsigned int fat_update_fat(unsigned int cluster, unsigned int val){
    unsigned int sector_offset = 0;
    unsigned int entry_offset = 0;
    
    //TODO : check cluster is in fat

    // 256 entries per sector
    sector_offset = cluster / 256;
    entry_offset = (cluster & 0xff) * 2;

    // the very first time update fat is called, get the working sector
    if (first_update) {
        first_update = 0;
        current_fat_sector_offset = sector_offset;
        // read in sector
        sd_raw_read((addr_FAT_1 + current_fat_sector_offset) * 512, current_fat_sector_cache, 512);
        
        //printf("read first: %d \r\n", current_fat_sector_offset);
    }

    // if we moved to working in a new sector of the fat
    if (sector_offset != current_fat_sector_offset) {
        // write previous sector 
        sd_raw_write((addr_FAT_1 + current_fat_sector_offset) * 512, current_fat_sector_cache, 512);
        sd_raw_write((addr_FAT_2 + current_fat_sector_offset) * 512, current_fat_sector_cache, 512);
       
        //printf("into new sector of fat\n\r");
        
        current_fat_sector_offset = sector_offset;
        // read in new sector
        sd_raw_read((addr_FAT_1 + current_fat_sector_offset) * 512, current_fat_sector_cache, 512);
        //printf("read: %d \r\n", current_fat_sector_offset);
    }

    // update entry
    current_fat_sector_cache[entry_offset] = val & 0xff;
    current_fat_sector_cache[entry_offset + 1] = (val >> 8) & 0xff;
}

// write the current sector of fat
unsigned int fat_flush_fat(void) {
    sd_raw_write((addr_FAT_1 + current_fat_sector_offset) * 512, current_fat_sector_cache, 512);
    sd_raw_write((addr_FAT_2 + current_fat_sector_offset) * 512, current_fat_sector_cache, 512);        
    //printf("wrote: %d \r\n", current_fat_sector_offset);
}

void fat_write_wav_header(unsigned int sector, unsigned int size) {

    unsigned int i = 0;
    for (i = 0; i < 512; i++){
        sdBuf[i] = 0;
    }

    // now write wave header
    sdBuf[0] = 0x52; // R
    sdBuf[1] = 0x49; // I
    sdBuf[2] = 0x46; // F
    sdBuf[3] = 0x46; // F
    sdBuf[4] = (size - 8) & 0xff;
    sdBuf[5] = ((size - 8) >> 8) & 0xff;
    sdBuf[6] = ((size - 8) >> 16) & 0xff;
    sdBuf[7] = ((size - 8) >> 24) & 0xff;
    sdBuf[8] = 0x57;  // W
    sdBuf[9] = 0x41;  // A
    sdBuf[10] = 0x56; // V
    sdBuf[11] = 0x45; // E
    sdBuf[12] = 0x66; // f
    sdBuf[13] = 0x6d; // m
    sdBuf[14] = 0x74; // t
    sdBuf[15] = 0x20;
    sdBuf[16] = 0x10;
    sdBuf[17] = 0x00;
    sdBuf[18] = 0x00;
    sdBuf[19] = 0x00;
    sdBuf[20] = 0x01;
    sdBuf[21] = 0x00;
    sdBuf[22] = 0x01;
    sdBuf[23] = 0x00;
    sdBuf[24] = 0x22;
    sdBuf[25] = 0x56;
    sdBuf[26] = 0x00;
    sdBuf[27] = 0x00;
    sdBuf[28] = 0x44;
    sdBuf[29] = 0xac;
    sdBuf[30] = 0x00;
    sdBuf[31] = 0x00;
    sdBuf[32] = 0x02;
    sdBuf[33] = 0x00;
    sdBuf[34] = 0x10;
    sdBuf[35] = 0x00;
    sdBuf[36] = 0x64; // d
    sdBuf[37] = 0x61; // a
    sdBuf[38] = 0x74; // t
    sdBuf[39] = 0x61; // a
    sdBuf[40] = (size - 44) & 0xff;;
    sdBuf[41] = ((size - 44) >> 8) & 0xff;
    sdBuf[42] = ((size - 44) >> 16) & 0xff;
    sdBuf[43] = ((size - 44) >> 24) & 0xff;

    // write it back
    sd_raw_write(sector * 512, sdBuf, 512);
}

// get last sector of sample and ramp it down
void smooth_end(unsigned int sector){

    short tmp;
	int sample;
	int i;

	sd_raw_read(sector * 512, sdBuf, 512);

	for (i = 0; i < 256; i++){
        // use 16 bit tmp so sign is preserved
        tmp = sdBuf[(i * 2) + 1] << 8;   // get it out
        tmp |= sdBuf[(i * 2)];

        sample = tmp;

        sample *= (255 - i);      // ramp it
        sample /= 256;

        sdBuf[(i * 2) + 1] = sample >> 8;            // and put it back
        sdBuf[(i * 2)] = sample & 0xff;
	}
	sd_raw_write(sector * 512, sdBuf, 512);
}

