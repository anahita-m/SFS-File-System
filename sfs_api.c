//NAME: ANAHITA MOHAPATRA
//STUDENT ID: 260773967
#include "disk_emu.h"
#include "sfs_api.h"
#include <fuse.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/********************************************************************************************/
/****************************************** DEFINITIONS *************************************/
/*********************************************************************************************/

#define MAGIC_NUMBER 260773967
#define NUM_BLOCKS 1024
#define BLOCK_SIZE 1024
#define MAX_FILE_BLOCKS 268
#define NUM_INODES 100
#define NUM_FILES 100
#define MAX_FILENAME_LENGTH 20
#define DISK_NAME "Anahita_disk"
#define BMAP_BLOCK_FREE 1
#define BMAP_BLOCK_IN_USE 0

/************************************************************************8*********************/
/**************************************** DATA STRUCTURES *************************************/
/**********************************************************************************************/

inode_t inode_table[NUM_INODES];
fd_entry_t fdt[NUM_INODES];
dir_entry_t root_dir[NUM_INODES];
super_block_t superblock;

/************************************************************************************************/
/*************************************** GLOBAL VARIABLES ***************************************/
/************************************************************************************************/

//stores next file for the getnextfilename methhod
int next_file = 0;
//this is our bitmap equivalent
int bmap[NUM_BLOCKS];

/*********************************************************************************************/
/************************************ INITIALIZATION METHODS *********************************/
/********************************************************************************************/

void init_superblock(){
    superblock.block_size = BLOCK_SIZE;
    superblock.file_system_size = NUM_BLOCKS;
    int num_inode_blocks = sizeof(inode_table)/BLOCK_SIZE + 1;
    superblock.inode_table_length = num_inode_blocks;
    superblock.magic = MAGIC_NUMBER;
    superblock.root_inode = 0;
}

void init_bmap(){
    //mark all as free
    for(int i = 0; i < NUM_BLOCKS; i++){
            bmap[i] = BMAP_BLOCK_FREE;
        }
}

void init_inode_table(){
    for(int i = 0; i < NUM_INODES; i++){
        inode_table[i].notInUse = true;
     }
}

void init_fdt(){
    for(int i = 0; i < NUM_INODES; i++){
        fdt[i].notInUse = true;
     }
}

void init_root_dir(){
    for(int i = 0; i < NUM_INODES; i++){
        root_dir[i].notInUse = true;
  }
}

void set_bmap(){
    //allocating superblock block
        bmap[0] = BMAP_BLOCK_IN_USE;
        // allocating bmap block
        bmap[NUM_BLOCKS-1] = BMAP_BLOCK_IN_USE;
        int num_bmap_blocks = sizeof(bmap)/BLOCK_SIZE+1;
        for(int i = NUM_BLOCKS - num_bmap_blocks; i < NUM_BLOCKS ; i++){
            bmap[i] = BMAP_BLOCK_IN_USE;
        }
        //allocating inode blocks
        for(int i = 1; i <= superblock.inode_table_length ; i++){
            bmap[i] = BMAP_BLOCK_IN_USE;
        }
        int bmap_blocks = 0;
        //direct pointers
        for(int i = superblock.inode_table_length+1; i <= superblock.inode_table_length+12; i++){
            if(bmap_blocks == 12){
                break;
            }
            else{
            bmap[i] = BMAP_BLOCK_IN_USE;
            inode_table[superblock.root_inode].pointer[bmap_blocks] = i;
            bmap_blocks++;
        }

    }
}


void set_inode_sb(){
    inode_table[superblock.root_inode].indirect = -1;
    inode_table[superblock.root_inode].link_cnt = 12;
    inode_table[superblock.root_inode].size = sizeof(root_dir);
    inode_table[superblock.root_inode].mode = MAGIC_NUMBER;
}       
/**********************************************************************************************************/
/*************************************** API & HELPER METHODS ********************************************/
/*********************************************************************************************************/


void mksfs(int fresh){
    if(fresh){
        //self-explanatory through method names 
        init_fresh_disk(DISK_NAME, BLOCK_SIZE, NUM_BLOCKS);
         init_inode_table();
         init_fdt();
         init_root_dir();
         init_bmap();
         init_superblock();
         set_bmap();
        //allocating inode to root
        inode_table[superblock.root_inode].notInUse = false;
        set_inode_sb();
        write_to_disk();

    }else{
        //initialize exisiting disk
        init_disk(DISK_NAME, BLOCK_SIZE, NUM_BLOCKS);
        //initiate fd table
        for(int i = 0; i < NUM_INODES; i++){
            fdt[i].notInUse = true;
        }
        //retrieve information from disk 
        //create empty buffers to read data into 
        //calloc automatically initalizes memory allocated to 0 
        void *sb = (void*) calloc(1, BLOCK_SIZE); 
        void *in = (void*) calloc(1, BLOCK_SIZE * superblock.inode_table_length);
        void *rd = (void*) calloc(1, BLOCK_SIZE * sizeof(root_dir)); 
        void *bm = (void*) calloc(1, BLOCK_SIZE * (sizeof(bmap)/BLOCK_SIZE + 1));

        //superblock
        read_blocks(0, 1, sb); //read it 
        memcpy(&superblock, sb, sizeof(superblock));//but need to copy proper size
        //inode table
        read_blocks(1, superblock.inode_table_length, in); //read it
        memcpy(inode_table, in, sizeof(inode_table)); //copy it in
        //read directory
        read_blocks(inode_table[0].pointer[0], 12, rd);
        memcpy(root_dir, rd, sizeof(root_dir));
        //open blocks array
        int num_bmap_blocks = sizeof(bmap)/BLOCK_SIZE + 1;
        read_blocks(NUM_BLOCKS-(num_bmap_blocks), num_bmap_blocks, bm);
        memcpy(bmap, bm, sizeof(bmap));
        
        //free all buffers
        free(sb);
        free(in);
        free(rd);
        free(bm);

    }
}

/************************************************************************************************/
int sfs_getnextfilename(char *fname){
    int i;
    for(i = next_file; i < NUM_INODES; i++){
        //if dir entry is not in use then we reset variable
        if(!root_dir[i].notInUse){
           
          //copy file name into entry 
          strcpy(fname, root_dir[i].file_name);
          i++;
	  //update global variable with next file
	  next_file = i;    
          return 1;
        }
    }
    next_file = 0;
    return 0;
}
/************************************************************************************************/
int sfs_getfilesize(const char *path){
//loop through root directory and compare file names to find file
    for(int i = 0 ; i < NUM_INODES; i++){
        if(!root_dir[i].notInUse){
       int comp = strcmp(root_dir[i].file_name, path);
            if(comp == 0){
                //access inode table to retrieve file size information and return
                return inode_table[root_dir[i].inode].size;
            }
        }
    }
    //error message for a non existing file 
    printf("File does not exist!\n");
    return -1;
}
/************************************************************************************************/
//this method adds an inode entry to the inode table
void addInode(int i, int j, char* fname){
    inode_table[j].mode = MAGIC_NUMBER;
    inode_table[j].link_cnt = 0;
    inode_table[j].size = 0;
    //mark index as being used 
    inode_table[j].notInUse = false;
    //mark root directory entry as being used
    root_dir[i].notInUse = false;
    //copy filename into root directory entry 
    strcpy(root_dir[i].file_name, fname);
    root_dir[i].inode = j;
}
/************************************************************************************************/
//this method opens a file 
int sfs_fopen(char *name){
    //check if file name too long
    if(strlen(name) > MAX_FILENAME_LENGTH){
        printf("File name is invalid, too long!\n");
        return -1;
    }
    int index;
    //check if the file exists
    int fileExists = -1;
    for (int i = 0 ; i < NUM_INODES;i++) {
        if(!root_dir[i].notInUse){
            int comp = strcmp(root_dir[i].file_name, name);
            if (comp == 0) {
                //if file exists, we set this variable equal to 1
                fileExists = 1;
                //index stores the root directory index in which the gile is stored
                index = i;
                break;
            }
        }
    }
    //if the file exists we must check if it is open
    if(fileExists == 1 ){
    for(int i = 0; i < NUM_INODES; i++){
        //if file is in the file descriptor table, it is open: error
        if(!fdt[i].notInUse){
          if(fdt[i].inode  == root_dir[index].inode){
            printf("File already open\n");
            return -1;
        }

        } 
    }
    //get inode
    int inode = root_dir[index].inode;
    //add file to open file descriptor
    int fd = -1;


    for(int j = 0; j < NUM_INODES; j++){
        if(fdt[j].notInUse){
            //mark this index as being used
            fdt[j].notInUse = false;
            fdt[j].inode = inode;
            //set read to begining of file 
            fdt[j].rptr = 0;
            //set write pointer to end of flie
            fdt[j].wptr = inode_table[inode].size;

            //return open file descriptor table index where entry is added
            fd = j;
            break;
        }
    }
    
    if(fd == -1){
        //no more space in open file descriptor table, error
        printf("Could not find space in file directory table!\n");
        return -1;
    }

    return fd;
}
    //if file doesn't exist, we create it
    if (checkFileName(name) == -1){
        printf("File name is invalid!\n");
        return -1;
    }
    return sfs_fcreate(name);
}
/************************************************************************************************/
//this method creates file 
int sfs_fcreate(char* name){
    int dir_index, inode_index = -1;
    //check to see if directory entry is available
    for(int i = 0; i < NUM_INODES; i++){
           //obtain free directory entry
           if(root_dir[i].notInUse){
            dir_index = i;
            break;
        }
    }
    //If there is no more directory space, error
        if(dir_index == -1){
            printf("Could not allocate a free directory entry for file creation!\n");
            return -1;
        }

    //find an available inode entry
        for(int j = 0; j < NUM_INODES; j++) {
            if (inode_table[j].notInUse) {
                inode_index = j;
                break;
            }
        }

        //if there is no more inode table space, error
        if(inode_index == -1){
            printf("Could not allocate a free inode entry for file creation!\n");
            return -1;
        } 

        //allocate inode
        addInode(dir_index, inode_index,name);
        //write updates to disk
        write_to_disk();
        //get inode
        int inode = root_dir[dir_index].inode;
        //add it to open file descriptor table

        int fd = -1;

    for(int i = 0; i < NUM_INODES; i++){
        if(fdt[i].notInUse){
            //mark this index as being used
            fdt[i].notInUse = false;
            fdt[i].inode = inode;
            //set read to begining of file 
            fdt[i].rptr = 0;
            //set write pointer to end of flie
            fdt[i].wptr = inode_table[inode].size;

            //return open file descriptor table index where entry is added
            fd = i;
            break;
        }
    }
    
    if(fd == -1){
        //no more space in open file descriptor table, error
        printf("Could not find space in file directory table!\n");
        return -1;
    }

    return fd;
}
/************************************************************************************************/
int sfs_fclose(int fileID){
    //validate File ID
    if(fileID < 0 || fileID >= NUM_INODES){
        printf("File ID is invalid\n");
        return -1;
    }
    //if the file is open, close it
    if(!fdt[fileID].notInUse){
        fdt[fileID].notInUse = true;
        return 0;
    }
    //file is already closed, error
    printf("File is already closed!\n");
    return -1;
}

int sfs_frseek(int fileID, int loc){
    if(fileID < 0 || fileID >= NUM_INODES){
        printf("File ID is invalid\n");
        return -1;
    }
    //get inode number
    int inode_index = fdt[fileID].inode;  

    //adjust reader pointer
    if(loc >=0 && loc <= inode_table[inode_index].size){
        fdt[fileID].rptr = loc;
    return 0;
    }
    //seek error
    printf("Seek location is out of bounds!\n");
    return -1;
}

int sfs_fwseek(int fileID, int loc){
    if(fileID < 0 || fileID >= NUM_INODES){
        printf("File ID is invalid\n");
        return -1;
    }
    //get inode number
    int inode_index = fdt[fileID].inode;  

    //adjust writer pointer
    if(loc >=0 && loc <= inode_table[inode_index].size){
        fdt[fileID].wptr = loc;
    return 0;
    }
    //seek error
    printf("Seek location is out of bounds!\n");
    return -1;
}
/************************************************************************************************/
//this method validates a given file name 
int checkFileName(char *name){
    char *extension; 
    //file name must be greater than 0
    if(strlen(name) == 0){
            printf("Need filename longer than length 0\n");
            return -1;
        }
        //validates the length of an extension
        extension = strchr(name, '.');
        //validate file name with the constraints
        if((extension != NULL && strlen(extension) <= 4) || extension == NULL){
            //file name is too long, error
            if(strlen(name) > MAX_FILENAME_LENGTH){
                printf("File name too long!\n");
                return -1;
            }else{
              return 0;
            }
        }
        else{
            //invalid file name, error
            printf("Either the file name or file extension is invalid!\n");
            return -1;
        }
    }
/************************************************************************************************/
//this method reads from a file 
int sfs_fread(int fileID, char *buf, int length) {

    //Error Handling
    //ensure that the fileID is valid
    if (fileID < 0 || fileID >= NUM_FILES) {
        printf("File ID is invalid!\n");
        return -1;
    }
    //validate that the length argument is a positive value 
    if (length <= 0) {
        printf("Invalid Length!\n");
        return -1;
    }
    //check to see if the file is open in the open file descript table
    if (fdt[fileID].notInUse) {
        printf("File is not open!\n");
        return -1;
    }

    //inode cache
    inode_t cache = inode_table[fdt[fileID].inode];

    //initialize empty buffer
    int buf_size = cache.link_cnt * BLOCK_SIZE;
    char block_buff[buf_size];
    bzero(block_buff, sizeof(block_buff));
    //indirect pointers array 
    int ind_ptrs[256];
    read_blocks(cache.indirect, 1, ind_ptrs);

    
    //get the block we are currently starting from 
    int block = fdt[fileID].rptr / BLOCK_SIZE;
    //get offset within block
    int offset = fdt[fileID].rptr % BLOCK_SIZE;
  
    if (fdt[fileID].rptr + length > cache.size) {
        //adjust length if it is too large
       length = cache.size + fdt[fileID].rptr;
    }
    int bytes_read = length;
    //Read whole file into block_buff
    int count = cache.link_cnt;
    for (int i = 0; i < count; i++ ){
        //for blocks through indirect pointer
        if(i >= 12) {
            for(int j = i; j < count; j++){
                //indirect pointers
                int block_offset = j * BLOCK_SIZE;
                read_blocks(ind_ptrs[j-12], 1, block_buff + block_offset);
            }
            break;
            
        }else{
            //direct pointers
             int block_offset = i * BLOCK_SIZE;
            read_blocks(cache.pointer[i], 1, block_buff + block_offset);
        }
    }
    //update read pointer
    fdt[fileID].rptr =  fdt[fileID].rptr + bytes_read;
    //write into buffer
    memcpy(buf, block_buff + (block*BLOCK_SIZE) + offset, bytes_read);
    //write_to_disk system cache
    write_to_disk();
    //return the number of bytes that were read 
    return bytes_read;
}
/************************************************************************************************/
//this method writes to a file 
int sfs_fwrite(int fileID, char *buf, int length){
    int blocks_needed;
    //validate file ID
    if(fileID < 0 || fileID >= NUM_FILES){
        printf("Invalid file ID\n");
        return -1;
    }
    //check if file is open
    if(fdt[fileID].notInUse){
        //error if file is not open, cannot write to it 
        printf("File is not open!\n");
        return -1;
    }
    //if file is open, cache inode
    inode_t cache = inode_table[fdt[fileID].inode];
    //get block from which we start writing
    int block = fdt[fileID].wptr/BLOCK_SIZE;
    //get location within block 
    int offset = fdt[fileID].wptr%BLOCK_SIZE;
    //get how many bytes need to be written
    int bytesWritten = length;
    if (length + fdt[fileID].wptr > MAX_FILE_BLOCKS*BLOCK_SIZE){
        //adjust bytes to be written if it exceeds the file size 
        bytesWritten -= length + fdt[fileID].wptr - (MAX_FILE_BLOCKS*BLOCK_SIZE);
    }

    //calculate how many blocks we need to complete write
    int cur_block = (fdt[fileID].wptr + bytesWritten)/BLOCK_SIZE;
    //if the pointer is at the end of the file, we need to go into the next block or use a pointer
    if ((fdt[fileID].wptr + bytesWritten)%BLOCK_SIZE != 0){
        //increment block
        cur_block++;
    } 
    //if we dont have enough pointers, we need another block 
    if(!(cur_block < cache.link_cnt)){
        blocks_needed = cur_block - cache.link_cnt;
    }

    //allocate extra bitmap blocks
    int bmap_provided_blocks = bitmap_blocks_needed_for_write(fileID, blocks_needed);
    //update cache
    cache = inode_table[fdt[fileID].inode];
    //if we couldn't provide all the blocks needed, we are out of space on the bitmap
    if(blocks_needed > bmap_provided_blocks){
        //adjust the number of blocks needed to how many were provided
        blocks_needed = bmap_provided_blocks;
        //update how many bytes we need to write 
        bytesWritten = bytesWritten - (length + fdt[fileID].wptr - (cache.link_cnt*BLOCK_SIZE));
    }

    //initialize empty byffer
    char buffer[(cache.link_cnt)*BLOCK_SIZE];
    bzero(buffer, sizeof(buffer));

    //read file into the created buffer 
     int j = 0;
    while (j < cache.link_cnt){
        if(j < 12) {
            //read diret pointers
            int block_offset = j * BLOCK_SIZE;
            read_blocks(cache.pointer[j], 1, buffer + block_offset);
        }else{
            //read indirect pointer into array 
            int ind_ptrs[256];
            read_blocks(cache.indirect, 1, ind_ptrs);
            //read indirect pointers into buffer if it doesn't exceed link count
            for(int k = j; k < cache.link_cnt; k++){
                int num_ptrs = k - 12;
                int block_offset = k * BLOCK_SIZE;
                read_blocks(ind_ptrs[num_ptrs], 1, buffer + block_offset);
            }
            break; //exit loop 
        }
    j++; //increment
    }

    //write into buffer at offset within current block
    memcpy(buffer+(block*BLOCK_SIZE)+offset, buf, bytesWritten);

    //Write file to buffer
    int i = 0;
    while (i < cache.link_cnt){
        //write direct pointers into buffer
        if(i < 12){
            int block_offset = i * BLOCK_SIZE;
            write_blocks(cache.pointer[i], 1, buffer + block_offset);
        }else{
            //read indirect pointers into indirect pointers array 
            int ind_ptrs[256];
            read_blocks(cache.indirect, 1, ind_ptrs);
            //write indirect pointers from indirect pointers array into buffer
            for(int k = i; k < cache.link_cnt; k++){
                int num_ptrs = k-12;
                int block_offset = k * BLOCK_SIZE;
                write_blocks(ind_ptrs[num_ptrs], 1, buffer + block_offset);
            }
            break; //exit loop 
        }
    i++;
    }
    //update file size and adjust 
    int newPointer = fdt[fileID].wptr + bytesWritten; 
    if(newPointer > cache.size){
        cache.size = newPointer;
    }
    inode_table[fdt[fileID].inode] = cache;
    //update write pointer
    fdt[fileID].wptr =  newPointer;
    //write cache to disk 
    write_to_disk();
    //return the number of bytes written to the file
    return bytesWritten;
}
/************************************************************************************************/
//this method removes a file from the file system 
int sfs_remove(char *file){

     inode_t cache;
    //cache is free 
    cache.notInUse = true;
    int i = 0;
    while(i < NUM_INODES){
        //if directory entry is in use, we compare file names
        if(!root_dir[i].notInUse){
         int comp = strcmp(root_dir[i].file_name, file);
            //if the file exists
            if(comp == 0){
                //check if the file is open
                for(int j = 0; j < NUM_INODES; j++){
                    //if file directory entry is being used
                    if(!fdt[j].notInUse){
                        //compare indodes
                        if(fdt[j].inode == root_dir[i].inode){
                            printf("File is open!\n");
                            return -1;
                        }
                    }
                }
                //get inode of file we are removing
                int inode = root_dir[i].inode;
                cache = inode_table[inode];
                //mark root directory entry as available
                root_dir[i].notInUse = true;
                //mark inode table entry as free
                inode_table[inode].notInUse = true;
                //write updates to disk 
                write_to_disk();
                //exit loop 
                break;
            }
        }
        //iterate entries 
        i++;
    }
    //handle case where file does not exist 
    if(cache.notInUse){
        printf("File not found!\n");
        return -1;
    }
    //else, if file exists, we remove it by deallocate any bitmap blocks
    int j = 0;
    int count = cache.link_cnt;
    while(j< count){
        if(j<12){
            //free direct pointer blocks 
            //set bitmap blocks to available 
            bmap[cache.pointer[j]] = BMAP_BLOCK_FREE;
        }else{
            //free indirect pointer block
            bmap[cache.indirect] = BMAP_BLOCK_FREE;

            int ind_ptrs[256];
            read_blocks(cache.indirect, 1, ind_ptrs);

            for(int k = j; k < cache.link_cnt; k++){
                     int blocks_to_allocate = k - 12;
                //mark bitmap block as free/available
                bmap[ind_ptrs[blocks_to_allocate]] = BMAP_BLOCK_FREE;
            }
            //exit loop 
            break;
        }
      j++;
    }
    //write updates to disk
    write_to_disk();
    return 0;
}

/************************************************************************************************/
//function to write_to_disk system cache
void write_to_disk(){
    //empty buffer of max file storage size
    char buffer[20000];
    bzero(buffer, sizeof(buffer));
    //write superblock to disk 
    memcpy(buffer, &superblock, sizeof(superblock));
    write_blocks(0, 1, buffer);
    //clear buffer
    bzero(buffer, sizeof(buffer));
     //write directory table to disk 
    memcpy(buffer, root_dir, sizeof(root_dir));
    write_blocks(inode_table[0].pointer[0], 12, buffer);
    //clear buffer
    bzero(buffer, sizeof(buffer));
    //write inode table to disk 
    memcpy(buffer, inode_table, sizeof(inode_table));
    write_blocks(1, superblock.inode_table_length, buffer);
    //clear buffer
    bzero(buffer, sizeof(buffer));
    //write bitmap to disk
    memcpy(buffer, bmap, sizeof(bmap));
    int bmap_blocks = sizeof(bmap)/BLOCK_SIZE + 1;
    write_blocks(NUM_BLOCKS - bmap_blocks, bmap_blocks, buffer);
}

/************************************************************************************************/
//this method provides the needed blocks from the bitmap and marks them as used
int bitmap_blocks_needed_for_write(int fileID, int num_blocks) {
    //keeps track of how many new bitmap blocks are allovcated
    int marked_blocks = 0;
    //get inode
    int inode = fdt[fileID].inode;
    //cache 
    inode_t cache = inode_table[inode];
    int count = cache.link_cnt + num_blocks;
    //where we start our loop
    int i = cache.link_cnt;

   /* allocate a pointer, a direct for the first 12 and indirect for the rest
    mark block as used in the bitmap  */
    while (i < count) {
        int block = 0;
        while (block < NUM_BLOCKS) {
            //check if block in bitmap is free
            if (bmap[block] == BMAP_BLOCK_FREE) {
                //if it is, mark the block in the bitmap as used
                bmap[block] = BMAP_BLOCK_IN_USE;
                //direct pointer case
                if (i < 12) {
                    //set inode cache pointer to allocated block
                    cache.pointer[i] = block;
                    //increase counter for used bitmap blocks
                    marked_blocks++;
                //indirect pointer case
                } else {
                    // check to see if indirect pointer has already been allocated
                    int indirect = -1;
                    if(cache.link_cnt + marked_blocks == 12){
                        for (int indirect_block_index = 0; indirect_block_index < NUM_BLOCKS; indirect_block_index++) {
                            //if the bitmap block is available
                            if (bmap[indirect_block_index] == BMAP_BLOCK_FREE) {
                                //mark it as used
                                bmap[indirect_block_index] = BMAP_BLOCK_IN_USE;
                                //indirect pointer set to block 
                                cache.indirect = indirect_block_index;
                                //indirect pointers array
                                int ind_ptrs[256];
                                //write it to disk 
                                write_blocks(cache.indirect, 1, ind_ptrs);
                                //allocate indirect pointer
                                indirect = 1;
                                break;
                            }
                        }
                    }else {
                        indirect = 1;
                    }
                    //if no indirect pointer we break 
                    if (indirect == -1){
                        //ran out of indirect pointers
                        break;
                    }
                    //add block to the array of ind_ptrs
                    int ind_ptrs[256];
                    read_blocks(cache.indirect, 1, ind_ptrs);
                    ind_ptrs[i-12] = block;
                    marked_blocks++;
                    write_blocks(cache.indirect, 1, ind_ptrs);
                }
                break;
            }
            //increment block
            block++;
        }
        //increment link counter
        i++;
    }
    //update link count
    cache.link_cnt = cache.link_cnt + marked_blocks;
    //write_to_disk function cache
    inode_table[fdt[fileID].inode] = cache;
    //write updates to disk
    write_to_disk();
    //return the number of newly allocate bitmap blocks
    return marked_blocks;
}   
