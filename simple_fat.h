





// Look for 'FAT16' to find boot sector.  it will be at offset 0x36
void fat_locate_boot_sector (void);
void fat_get_partition_info(void);
void fat_initialize_disk(void);
void fat_create_file (void);
void fat_make_sounds_directory (void);
void fat_write_file_entry (void);
void fat_write_file_data (void);
unsigned int fat_count_files (void);
unsigned int fat_update_fat(unsigned int cluster, unsigned int val);
unsigned int fat_flush_fat(void) ;
void fat_locate_sfs(void);
void fat_clean_root_directory(void);

void fat_check_for_sounds_directory(void);
unsigned int fat_get_used_space(void) ;
unsigned int fat_get_total_space(void) ;
unsigned int fat_get_file_start_address(unsigned int file_num);
unsigned int fat_get_file_end_address(unsigned int file_num);
unsigned int fat_get_num_files(void);



