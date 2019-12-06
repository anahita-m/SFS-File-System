#include <stdbool.h>

#define MAXFILENAME 20

/*************************** API METHODS *********************************/
void mksfs(int fresh);
int sfs_getnextfilename(char *fname);
int sfs_getfilesize(const char *path);
int sfs_fopen(char *name);
int sfs_fcreate(char *name);
int sfs_fclose(int fileID);
int sfs_frseek(int fileID, int loc);
int sfs_fwseek(int fileID, int loc);
int sfs_fread(int fileID, char *buf, int length);
int sfs_fwrite(int fileID, char *buf, int length);
int sfs_remove(char *file);

/******************************* INITIALIZATION METHODS ***********************/
void init_superblock();
void init_bmap();
void init_inode_table();
void init_fdt();
void init_root_dir();
void set_bmap();
void set_inode_sb();

/***************************** HELPER METHODS ********************************/
void addInode(int i, int j, char* fname);
int checkFileName(char *name);
void write_to_disk();
int add_file_descriptor_entry(int inode);
int bitmap_blocks_needed_for_write(int fileID, int num_blocks);

typedef struct inode {
    bool notInUse;
    int mode;
    int link_cnt;
    int size;
    int pointer[12];
    int indirect;
} inode_t;

typedef struct fd{
    bool notInUse;
    int inode;
    int rptr;
    int wptr;
} fd_entry_t;

typedef struct super_block{
    int magic;
    int block_size;
    int file_system_size;
    int inode_table_length;
    int root_inode;
} super_block_t;

typedef struct directory{
    bool notInUse;
    int inode;
    //filename is maxlength + 1 to account for null terminator
    char file_name[MAXFILENAME+1];
}dir_entry_t;

