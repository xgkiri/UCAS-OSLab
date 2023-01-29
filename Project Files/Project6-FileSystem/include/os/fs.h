#ifndef __INCLUDE_OS_FS_H__
#define __INCLUDE_OS_FS_H__

#include <type.h>

/* macros of file system */
#define SUPERBLOCK_MAGIC 0x09925014
#define NUM_FDESCS 16

#define FS_START_POS (1lu << 20) // 512MB
#define SECTOR_SIZE 512
#define BLOCK_SIZE 4096
#define BLOCK2SECTOR 8 // 一个block：4KB 一个sector：0.5KB
// sector_map
#define TOTAL_SECTOR_NUM 2097152 // 1GB
#define SECTOR_MAP_NUM 64
// inode
#define INODE_NUM 4096 // 512Byte，刚好一个sector
#define INODE_MAP_NUM 1

#define FILE_NAME_LEN_MAX 32
#define DIR_ENTRY_NUM (BLOCK_SIZE / sizeof(dentry_t))

typedef enum {
    DATA,
    DIR,
} file_type_t;

typedef enum {
    RD_ONLY,
    WR_ONLY,
    RD_WR,
} mode_t;

/* data structures of file system */
typedef struct superblock_t{
    // TODO [P6-task1]: Implement the data structure of superblock
    uint64_t magic_number;

    int fs_size; //文件系统大小，单位为sector
    int start_pos; //起始sector

    // 单位都为sector
    int sector_map_num;
    int sector_map_offset;

    int inode_map_num;
    int inode_map_offset;

    int inode_num;
    int inode_offset;

    int data_num;
    int data_offset;

    // 单位为Byte
    int dir_entry_size;
    int inode_size;

    int root_inode_number;

} superblock_t;

typedef struct dentry_t{
    // TODO [P6-task1]: Implement the data structure of directory entry
    char file_name[FILE_NAME_LEN_MAX];
    int inode_number;//第几个sector
    file_type_t file_type;
} dentry_t;

typedef struct dir_t{
    char dir_name[FILE_NAME_LEN_MAX];
    int inode_number;
    int start_sector_number;
    dentry_t dir_entry[DIR_ENTRY_NUM];
} dir_t;

typedef struct inode_t{ 
    // TODO [P6-task1]: Implement the data structure of inode
    file_type_t file_type;
    int file_size; // sector
    int nlinks;
    // TODO: add other info

    int direct_blocks[10]; // sector number of first sector in each block
    int indirect_blocks[4];  // sector number of first sector in indirect block
    int double_indirect_blocks[2];
    int triple_indirect_blocks;
    // add triple?
} inode_t;

typedef struct fdesc_t{
    // TODO [P6-task2]: Implement the data structure of file descriptor
    int inode_number;
    mode_t mode;   
    //byte
    int read_offset; 
    int write_offset;
} fdesc_t;

/* modes of do_fopen */
#define O_RDONLY 1  /* read only open */
#define O_WRONLY 2  /* write only open */
#define O_RDWR   3  /* read/write open */

/* whence of do_lseek */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* type of pointer */
#define READ_P 0
#define WRITE_P 1

/* fs function declarations */
extern int do_mkfs(void);
extern int do_statfs(void);
extern int do_cd(char *path);
extern int do_mkdir(char *path);
extern int do_rmdir(char *path);
extern int do_ls(char *path, int option);
extern int do_touch(char *path);
extern int do_cat(char *path);
extern int do_fopen(char *path, int mode);
extern int do_fread(int fd, char *buff, int length);
extern int do_fwrite(int fd, char *buff, int length);
extern int do_fclose(int fd);
extern int do_ln(char *src_path, char *dst_path);
extern int do_rm(char *path);
extern int do_lseek(int fd, int offset, int whence, int rw_type);

extern void init_fs();
#endif