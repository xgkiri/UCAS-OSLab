#include <os/string.h>
#include <os/fs.h>
#include <os/kernel.h>
#include <os/mm.h>
#include <printk.h>

static superblock_t superblock; // 在完成init_fs后，superblock中总是有有效数据
static fdesc_t fdesc_array[NUM_FDESCS];

// 当前目录
dir_t current_dir; 
dentry_t dir_entry_buf[DIR_ENTRY_NUM];

// 大小为一个sector的buf
uint8_t sector_buf[SECTOR_SIZE]; 

// 间接块buf
uint32_t indirect_block_buf[BLOCK_SIZE / sizeof(uint32_t)];

// 读写文件块buf
uint8_t block_buf[BLOCK_SIZE];

int do_mkfs(void)
{
    // TODO [P6-task1]: Implement do_mkfs
    /*
    printk("do mkfs\n");
    return;
    */

    inode_t inode_root;

    bios_sdread(kva2pa(sector_buf), 1, FS_START_POS);
    memcpy((&superblock), sector_buf, sizeof(superblock_t));
    if(superblock.magic_number == SUPERBLOCK_MAGIC){
        do_statfs();
        return 0;
    }
    
    superblock.magic_number = SUPERBLOCK_MAGIC;
    superblock.fs_size = TOTAL_SECTOR_NUM;
    superblock.start_pos = FS_START_POS;
    superblock.sector_map_num = SECTOR_MAP_NUM;
    superblock.sector_map_offset = 1;
    superblock.inode_map_num = INODE_MAP_NUM;
    superblock.inode_map_offset = 1 + SECTOR_MAP_NUM;
    superblock.inode_num = INODE_NUM;
    superblock.inode_offset = 1 + SECTOR_MAP_NUM + INODE_MAP_NUM;
    superblock.data_num = TOTAL_SECTOR_NUM - 1 - SECTOR_MAP_NUM - INODE_MAP_NUM - INODE_NUM;
    superblock.data_offset = 1 + SECTOR_MAP_NUM + INODE_MAP_NUM + INODE_NUM;
    superblock.dir_entry_size = sizeof(dentry_t);
    superblock.inode_size = SECTOR_SIZE;//一个inode占据一个sector，后续可以考虑压缩
    superblock.root_inode_number = alloc_one_inode();
    // 此处写入磁盘后，superblock不再改变
    bzero(sector_buf, SECTOR_SIZE);
    memcpy(sector_buf, &superblock, sizeof(superblock_t));
    bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS);

    //TODO: print info

    // 初始化sector_map
    // 初始化时使用了4162个sector
    // 4096 + 66
    bzero(sector_buf, SECTOR_SIZE);
    memset(sector_buf, 0xff, SECTOR_SIZE);
    bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + superblock.sector_map_offset);

    bzero(sector_buf, SECTOR_SIZE);
    //memset(sector_buf, 0xff, 8);
    //memset(sector_buf + 8, 0xc0, 1);
    // padding: 8 sector align
    memset(sector_buf, 0xff, (1 + SECTOR_MAP_NUM + INODE_MAP_NUM) / 8 + 1);
    bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + superblock.sector_map_offset + 1);

    // 初始化root目录的inode
    inode_root.file_type = DIR;
    // root目录占据第一个block
    inode_root.file_size = 0; 
    alloc_sector(&inode_root, 1 * BLOCK2SECTOR);
    inode_root.nlinks = 0;
    for(int i = 0; i < 4; i++){
        inode_root.indirect_blocks[i] = -1;
    }
    //printk("root dir 1st sector = %d\n", inode_root.direct_blocks[0]);
    store_inode(&inode_root, sector_buf, superblock.root_inode_number);
    /*
    bzero(sector_buf, SECTOR_SIZE);
    memcpy(sector_buf, &inode_root, sizeof(inode_t));
    bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + superblock.inode_offset + superblock.root_inode_number);
    */

    // 为root添加 . 目录项
    strcpy(dir_entry_buf[0].file_name, ".");
    dir_entry_buf[0].inode_number = superblock.root_inode_number;
    dir_entry_buf[0].file_type = DIR;
    bzero(sector_buf, SECTOR_SIZE);
    memcpy(sector_buf, &dir_entry_buf, sizeof(dir_entry_buf[0]));
    bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + inode_root.direct_blocks[0]);

    do_statfs();
    return 0;  // do_mkfs succeeds
}

int do_statfs(void)
{
    // TODO [P6-task1]: Implement do_statfs
    /*
    printk("do status\n");
    return;
    */

    int sector_used = get_used_sector_num();
    int inode_used = get_used_inode_num();

    printk("magic  number : 0x%x\n", superblock.magic_number);
    printk("start  pos : number %d block\n", FS_START_POS);
    printk("sector map offset : %d, sector occupied : %d\n", superblock.sector_map_offset, superblock.sector_map_num);
    printk("inode  map offset : %d, sector occupied : %d\n", superblock.inode_map_offset, superblock.inode_map_num);
    printk("inode offset : %d, occupied sector : %d\n", superblock.inode_offset, superblock.inode_num);
    printk("data  offset : %d, occupied sector : %d\n", superblock.data_offset, superblock.data_num);
    printk("sector used : %d/%d\n", sector_used, superblock.fs_size);
    printk("inode  used : %d/%d\n", inode_used, superblock.inode_num);
    printk("one inode occupied size : %d Byte\n", superblock.inode_size);
    printk("one dir entry      size : %d Byte\n", superblock.dir_entry_size);
    printk("inode number of root dir : %d\n", superblock.root_inode_number);

    return 0;  // do_statfs succeeds
}

int do_cd(char *path)
{
    // TODO [P6-task1]: Implement do_cd
    /*
    printk("do mkdir\n");
    printk("path = %s\n", path);
    return;
    */
    char single_path[FILE_NAME_LEN_MAX];
    int success_flag = -1;
    for(int i = 0; ; i++){
        if(get_single_path(single_path, path, i) == -1){
            break;
        }
        else{
            success_flag = do_cd_once(single_path);
            if(success_flag == -1){
                break;
            }
        }
    }
    return success_flag;
}

int do_mkdir(char *path)
{
    // TODO [P6-task1]: Implement do_mkdir
    /*
    printk("do mkdir\n");
    printk("path = %s\n", path);
    return;
    */

    inode_t new_inode;
    dentry_t new_dentry;
    if(find_inode_number_once(path, current_dir.dir_entry) != -1){
        return -1; // failed
    }
    // 为新目录分配inode，并初始化inode
    strcpy(new_dentry.file_name, path);
    new_dentry.inode_number = alloc_one_inode();
    new_dentry.file_type = DIR;

    new_inode.file_type = DIR;
    new_inode.file_size = 0;
    new_inode.nlinks = 1; // 接下来会在父目录中创建新目录的目录项
    for(int i = 0; i < 4; i++){
        new_inode.indirect_blocks[i] = -1;
    }
    alloc_sector(&new_inode, 1 * BLOCK2SECTOR);
    store_inode(&new_inode, sector_buf, new_dentry.inode_number);
    /*
    bzero(sector_buf, SECTOR_SIZE);
    memcpy(sector_buf, &new_inode, sizeof(inode_t));
    bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + superblock.inode_offset + new_dentry.inode_number);
    */

    // 为新目录添加基础目录项
    strcpy(dir_entry_buf[0].file_name, ".");
    dir_entry_buf[0].inode_number = new_dentry.inode_number;
    dir_entry_buf[0].file_type = DIR;
    strcpy(dir_entry_buf[1].file_name, "..");
    dir_entry_buf[1].inode_number = current_dir.inode_number;
    dir_entry_buf[1].file_type = DIR;
    bzero(sector_buf, SECTOR_SIZE);
    memcpy(sector_buf, &dir_entry_buf, sizeof(dir_entry_buf[0]) + sizeof(dir_entry_buf[1]));
    bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + new_inode.direct_blocks[0]);

    // 在父目录（当前目录）中创建新目录的目录项
    add_new_dentry(&new_dentry, &current_dir);

    return 0;  // do_mkdir succeeds
}

int do_rmdir(char *path)
{
    // TODO [P6-task1]: Implement do_rmdir
    inode_t inode_buf;
    int inode_number = find_inode_number_once(path, current_dir.dir_entry);
    if(inode_number == -1){
        return -1; // failed
    }

    load_inode(&inode_buf, sector_buf, inode_number);
    free_one_file_sector(&inode_buf);
    free_one_inode(inode_number);
    del_dentry(path, &current_dir);
    return 0;  // do_rmdir succeeds
}

int do_ls(char *path, int option)
{
    // TODO [P6-task1]: Implement do_ls
    // Note: argument 'option' serves for 'ls -l' in A-core
    char single_path[FILE_NAME_LEN_MAX];
    int success_flag = -1;
    int file_count = 0;
    inode_t inode_buf;

    for(int i = 0; ; i++){
        if(get_single_path(single_path, path, i) == -1){
            break;
        }
        else{
            if(i == 0){
                success_flag = set_dir_buf_once(single_path, current_dir.dir_entry);
            }
            else{
                success_flag = set_dir_buf_once(single_path, dir_entry_buf);
            }
            if(success_flag == -1){
                return -1;
            }
        }
    }

    if(option == 0){
        for(int i = 0; i < DIR_ENTRY_NUM; i++){
            if(strcmp(dir_entry_buf[i].file_name, "") != 0){
                if(strcmp(dir_entry_buf[i].file_name, ".") != 0 && strcmp(dir_entry_buf[i].file_name, "..") != 0){
                    printk("%s\t\t\t", dir_entry_buf[i].file_name);
                    file_count++;
                    if(file_count != 0 && file_count % 4 == 0){
                        printk("\n");
                    }
                }
            }
        }
        printk("\n");
    }

    if(option == 1){
        for(int i = 0; i < DIR_ENTRY_NUM; i++){
            if(strcmp(dir_entry_buf[i].file_name, "") != 0){
                if(strcmp(dir_entry_buf[i].file_name, ".") != 0 && strcmp(dir_entry_buf[i].file_name, "..") != 0){
                    printk("name : %s  ", dir_entry_buf[i].file_name);
                    if(dir_entry_buf[i].file_type == DIR){
                        printk("file type : direction\n");
                    }
                    else if(dir_entry_buf[i].file_type == DATA){
                        printk("file type : data\n");
                    }
                    bios_sdread(kva2pa(sector_buf), 1, FS_START_POS + superblock.inode_offset + dir_entry_buf[i].inode_number);
                    memcpy(&inode_buf, sector_buf, sizeof(inode_t));    
                    printk("inode number : %d  ", dir_entry_buf[i].inode_number);
                    printk("hard link number : %d  ", inode_buf.nlinks);
                    printk("file size : %d sector\n\n", inode_buf.file_size);
                }
            }
        }
    }

    return 0;
}

int do_touch(char *path)
{
    // TODO [P6-task2]: Implement do_touch
    inode_t new_inode;
    dentry_t new_dentry;

    if(find_inode_number_once(path, current_dir.dir_entry) != -1){
        return -1; // failed
    }

    // 为新文件分配inode，并初始化inode
    strcpy(new_dentry.file_name, path);
    new_dentry.inode_number = alloc_one_inode();
    new_dentry.file_type = DATA;

    new_inode.file_type = DATA;
    new_inode.file_size = 0; 
    new_inode.nlinks = 1; // 接下来会在父目录中创建新目录的目录项
    for(int i = 0; i < 4; i++){
        new_inode.indirect_blocks[i] = -1;
    }
    alloc_sector(&new_inode, 1 * BLOCK2SECTOR); // 初始化大小为1个block
    store_inode(&new_inode, sector_buf, new_dentry.inode_number);
    /*
    bzero(sector_buf, SECTOR_SIZE);
    memcpy(sector_buf, &new_inode, sizeof(inode_t));
    bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + superblock.inode_offset + new_dentry.inode_number);
    */

    // 在父目录（当前目录）中创建新文件的目录项
    add_new_dentry(&new_dentry, &current_dir);
    return 0;  // do_touch succeeds
}

int do_cat(char *path)
{
    // TODO [P6-task2]: Implement do_cat
    inode_t inode_buf;
    int direct_block_num;
    int indirect_block_num;
    int indirect_block_pointer_num;
    int byte_count = 0;
    int inode_number = find_inode_number_once(path, current_dir.dir_entry);
    int total_block;
    if(inode_number == -1){
        return -1; // failed
    }
    // 读入文件的inode
    load_inode(&inode_buf, sector_buf, inode_number);

    total_block = (inode_buf.file_size / BLOCK2SECTOR) + (inode_buf.file_size % BLOCK2SECTOR != 0);
    if(total_block >= 10){
        direct_block_num = 10;
        indirect_block_num = (total_block - 10) % (BLOCK_SIZE / sizeof(uint32_t));
    }
    else{
        direct_block_num = total_block;
        indirect_block_num = -1;
    }
    indirect_block_pointer_num = (total_block - 10) / (BLOCK_SIZE / sizeof(uint32_t)) + (indirect_block_num != 0);

    // 每次cat一个block的数据
    for(int i = 0; i < direct_block_num; i++){
        bios_sdread(kva2pa(block_buf), 8, FS_START_POS + inode_buf.direct_blocks[i]);
        for(int j = 0; j < BLOCK_SIZE; j++){
            if(block_buf[j] != '\0'){
                printk("%c", block_buf[j]);
                byte_count++;
                if(byte_count % 32 == 0 && byte_count != 0){
                    //printk("\n");
                }
            }
        }
    }
    if(indirect_block_num != -1){
        for(int j = 0; j < indirect_block_pointer_num; j++){
            if(j != indirect_block_pointer_num - 1){
                indirect_block_num = BLOCK_SIZE / sizeof(uint32_t);
            }
            else{
                indirect_block_num = (total_block - 10) % (BLOCK_SIZE / sizeof(uint32_t));
            }
            bios_sdread(kva2pa(indirect_block_buf), 8, FS_START_POS + inode_buf.indirect_blocks[j]);
            for(int i = 0; i < indirect_block_num; i++){
                bios_sdread(kva2pa(block_buf), 8, FS_START_POS + indirect_block_buf[i]);
                for(int k = 0; k < BLOCK_SIZE; k++){
                    if(block_buf[k] != '\0'){
                        printk("%c", block_buf[k]);
                        byte_count++;
                        if(byte_count % 32 == 0 && byte_count != 0){
                            //printk("\n");
                        }
                    }
                }
            }
        }

    }
    printk("\n");

    return 0;  // do_cat succeeds
}

int do_fopen(char *path, int mode)
{
    // TODO [P6-task2]: Implement do_fopen
    int fdesc_idx = -2;
    int inode_number = find_inode_number_once(path, current_dir.dir_entry);
    if(inode_number == -1){
        return -1; // failed
    }

    for(int i = 0; i < NUM_FDESCS; i++){
        if(fdesc_array[i].inode_number == -1){
            fdesc_idx = i;
            break;
        }
    }
    if(fdesc_idx == -2){
        return -2; // failed
    }

    fdesc_array[fdesc_idx].inode_number = inode_number;
    fdesc_array[fdesc_idx].read_offset = 0;
    fdesc_array[fdesc_idx].write_offset = 0;
    if(mode == O_RDONLY){
        fdesc_array[fdesc_idx].mode = RD_ONLY;
    }
    else if(mode == O_WRONLY){
        fdesc_array[fdesc_idx].mode = WR_ONLY;
    }
    else if(mode == O_RDWR){
        fdesc_array[fdesc_idx].mode = RD_WR;
    }
    
    return fdesc_idx;  // return the id of file descriptor
}

int do_fread(int fd, char *buff, int length)
{
    // TODO [P6-task2]: Implement do_fread
    inode_t inode_buf;
    fdesc_t fdesc = fdesc_array[fd]; 
    int data_pos;
    int direct_block_idx;
    int indirect_block_idx;
    int read_length = 0;
    int read_length_once;
    int rem = fdesc.read_offset % BLOCK_SIZE;
    int total_block = (fdesc.read_offset / BLOCK_SIZE) + (rem != 0);
    int indirect_block_pointer_idx;
    // 权限判断
    if(fdesc.mode == RD_ONLY){
        return -1;
    }
    // 读入文件的inode
    load_inode(&inode_buf, sector_buf, fdesc.inode_number);
    if(length + fdesc.read_offset > inode_buf.file_size * SECTOR_SIZE){
        return -2; //大小超出
    }
    
    // 计算offset位置
    if(total_block >= 10){
        direct_block_idx = -1;
        indirect_block_idx = (total_block - 10) % (BLOCK_SIZE / sizeof(uint32_t));
        if(indirect_block_idx > 0){
            indirect_block_idx--;
        }
    }
    else{
        direct_block_idx = total_block;
        if(direct_block_idx > 0){
            direct_block_idx--;
        }
        indirect_block_idx = 0;
    }
    indirect_block_pointer_idx = (total_block - 10) / (BLOCK_SIZE / sizeof(uint32_t));

    // 从文件中读出数据
    while(read_length != length){
        if(direct_block_idx != -1 && direct_block_idx <= 9){
            data_pos = FS_START_POS + inode_buf.direct_blocks[direct_block_idx];
            direct_block_idx++;
        }
        else{
            bios_sdread(kva2pa(indirect_block_buf), 8, FS_START_POS + 
                                inode_buf.indirect_blocks[indirect_block_pointer_idx]);
            data_pos = FS_START_POS + indirect_block_buf[indirect_block_idx];
            if(indirect_block_idx < (BLOCK_SIZE / sizeof(uint32_t)) - 1){
                indirect_block_idx++;
            }
            else{
                indirect_block_pointer_idx++;
                indirect_block_idx = 0;
            }
        }
        bios_sdread(kva2pa(block_buf), 8, data_pos);

        if(rem + length > BLOCK_SIZE){
            read_length_once = BLOCK_SIZE - rem;
        }
        else{
            read_length_once = length;
        }
        memcpy(buff + read_length, block_buf + rem, read_length_once);
        rem = 0;
        read_length += read_length_once;
    }

    fdesc_array[fd].read_offset += read_length;

    return read_length;  // return the length of trully read data
}

int do_fwrite(int fd, char *buff, int length)
{
    // TODO [P6-task2]: Implement do_fwrite
    inode_t inode_buf;
    fdesc_t fdesc = fdesc_array[fd]; 
    int data_pos;
    int size_to_alloc;
    int direct_block_idx;
    int indirect_block_idx;
    int written_length = 0;
    int written_length_once;
    int rem = fdesc.write_offset % BLOCK_SIZE;
    int total_block = (fdesc.write_offset / BLOCK_SIZE) + (rem != 0);
    int indirect_block_pointer_idx;
    // 权限判断
    if(fdesc.mode == RD_ONLY){
        return -1;
    }
    // 读入文件的inode
    load_inode(&inode_buf, sector_buf, fdesc.inode_number);

    // 首先判断要写的数据是否超过了文件大小，如果超过了文件大小，则先为文件分配空间
    if(length + fdesc.write_offset > inode_buf.file_size * SECTOR_SIZE){
        if((length + fdesc.write_offset) % SECTOR_SIZE != 0){
            size_to_alloc = (length + fdesc.write_offset) / SECTOR_SIZE - inode_buf.file_size + 1;
        }
        else{
            size_to_alloc = (length + fdesc.write_offset) / SECTOR_SIZE - inode_buf.file_size;
        }
        alloc_sector(&inode_buf, size_to_alloc);
    }

    // 接下来计算offset所在的block
    if(total_block >= 10){
        direct_block_idx = -1; // 此处设置为-1表示不在直接索引中
        indirect_block_idx = (total_block - 10) % (BLOCK_SIZE / sizeof(uint32_t));
        if(indirect_block_idx > 0){
            indirect_block_idx--;
        }
    }
    else{
        direct_block_idx = total_block;
        if(direct_block_idx > 0){
            direct_block_idx--;
        }
        indirect_block_idx = 0;
    }
    indirect_block_pointer_idx = (total_block - 10) / (BLOCK_SIZE / sizeof(uint32_t));

    // 向文件中写入数据
    while(written_length != length){
        if(direct_block_idx != -1 && direct_block_idx <= 9){
            data_pos = FS_START_POS + inode_buf.direct_blocks[direct_block_idx];
            direct_block_idx++;
        }
        else{
            bios_sdread(kva2pa(indirect_block_buf), 8, FS_START_POS + 
                            inode_buf.indirect_blocks[indirect_block_pointer_idx]);
            data_pos = FS_START_POS + indirect_block_buf[indirect_block_idx];
            if(indirect_block_idx < (BLOCK_SIZE / sizeof(uint32_t)) - 1){
                indirect_block_idx++;
            }
            else{
                indirect_block_pointer_idx++;
                indirect_block_idx = 0;
            }
            indirect_block_idx++;
        }
        bios_sdread(kva2pa(block_buf), 8, data_pos);

        if(rem + length > BLOCK_SIZE){
            written_length_once = BLOCK_SIZE - rem;
        }
        else{
            written_length_once = length;
        }
        memcpy(block_buf + rem, buff + written_length, written_length_once);
        rem = 0;
        written_length += written_length_once;
        bios_sdwrite(kva2pa(block_buf), 8, data_pos);
    }

    fdesc_array[fd].write_offset += written_length;
    store_inode(&inode_buf, sector_buf, fdesc.inode_number);
    return written_length;  // return the length of trully written data
}

int do_fclose(int fd)
{
    // TODO [P6-task2]: Implement do_fclose
    fdesc_array[fd].inode_number = -1;

    return 0;  // do_fclose succeeds
}

int do_ln(char *src_path, char *dst_path)
{
    // TODO [P6-task2]: Implement do_ln
    char src_name[FILE_NAME_LEN_MAX];
    dentry_t new_dentry;
    inode_t inode_buf_src;
    inode_t inode_buf_dst;
    int inode_number_src = find_inode_number(src_path);
    int inode_number_dst = find_inode_number(dst_path);
    // 此时dir_buf中储存的是dst_dir
    load_inode(&inode_buf_src, sector_buf, inode_number_src);
    load_inode(&inode_buf_dst, sector_buf, inode_number_dst);
    if(inode_buf_dst.file_type != DIR || inode_buf_src.file_type != DATA){
        return -1; // file type error
    }
    get_file_name(src_path, src_name);
    if(find_inode_number_once(src_name, dir_entry_buf) != -1){
        return -2; // link already exist
    }
    // 在dst目录中添加该项
    for(int i = 0; i < DIR_ENTRY_NUM; i++){
        if(strcmp(dir_entry_buf[i].file_name, "") == 0){
            strcpy(dir_entry_buf[i].file_name, src_name);
            dir_entry_buf[i].inode_number = inode_number_src;
            dir_entry_buf[i].file_type = DATA;
            break;
        }
    }
    // 更新src的hard link number
    inode_buf_src.nlinks++;
    store_inode(&inode_buf_src, sector_buf, inode_number_src);
    // 写回dst目录
    bios_sdwrite(kva2pa(dir_entry_buf), 8, FS_START_POS + inode_buf_dst.direct_blocks[0]);

    return 0;  // do_ln succeeds 
}

int do_rm(char *path)
{
    // TODO [P6-task2]: Implement do_rm
    inode_t inode_buf;
    int inode_number = find_inode_number_once(path, current_dir.dir_entry);
    if(inode_number == -1){
        return -1; // failed
    }

    load_inode(&inode_buf, sector_buf, inode_number);
    inode_buf.nlinks--;
    if(inode_buf.nlinks == 0){
        // 释放资源
        free_one_file_sector(&inode_buf);
        free_one_inode(inode_number);
    }
    else{
        store_inode(&inode_buf, sector_buf, inode_number);
    }
    del_dentry(path, &current_dir);
    return 0;  // do_rm succeeds 
}

int do_lseek(int fd, int offset, int whence, int rw_type)
{
    // TODO [P6-task2]: Implement do_lseek
    inode_t inode_buf;
    load_inode(&inode_buf, sector_buf, fdesc_array[fd].inode_number);

    if(rw_type == READ_P){
        if(whence == SEEK_SET){
            fdesc_array[fd].read_offset = offset;
        }
        else if(whence == SEEK_CUR){
            fdesc_array[fd].read_offset += offset;
        }
        else if(whence == SEEK_END){
            fdesc_array[fd].read_offset = inode_buf.file_size * SECTOR_SIZE + offset;
        }
        return fdesc_array[fd].read_offset;
    }
    else if(rw_type == WRITE_P){
        if(whence == SEEK_SET){
            fdesc_array[fd].write_offset = offset;
        }
        else if(whence == SEEK_CUR){
            fdesc_array[fd].write_offset += offset;
        }
        else if(whence == SEEK_END){
            fdesc_array[fd].write_offset = inode_buf.file_size * SECTOR_SIZE + offset;
        }
        return fdesc_array[fd].write_offset;
    }
    // return the resulting offset location from the beginning of the file
}

int num2vec(int num){
    switch(num){
        case 1:
            return 0x80;
        case 2:
            return 0xc0;
        case 3:
            return 0xe0;
        case 4:
            return 0xf0;
        case 5:
            return 0xf8;
        case 6:
            return 0xfc;
        case 7:
            return 0xfe;
        case 8:
            return 0xff;
    }
}

void init_fs(){
    inode_t inode_buf;

    //sector_buf = kmalloc(SECTOR_SIZE);
    //block_buf = kmalloc(BLOCK_SIZE);
    do_mkfs();
    init_fdesc();

    // 设置当前目录为root
    bios_sdread(kva2pa(sector_buf), 1, FS_START_POS + superblock.inode_offset + superblock.root_inode_number);
    memcpy(&inode_buf, sector_buf, sizeof(inode_t));
    strcpy(current_dir.dir_name, "root");
    current_dir.inode_number = superblock.root_inode_number;
    current_dir.start_sector_number = inode_buf.direct_blocks[0];
    bios_sdread(kva2pa(current_dir.dir_entry), 8, FS_START_POS + inode_buf.direct_blocks[0]);
}

void init_fdesc(){
    for(int i = 0; i < NUM_FDESCS; i++){
        fdesc_array[i].inode_number = -1;
    }
}

/* NOTE: 假设：
*        1. sector map 8 sector对齐
*/
int get_used_sector_num(){
    int sector_used = 0;
    uint8_t *sector_map_pointer;
    for(int i = 0; i < SECTOR_MAP_NUM; i++){
        bios_sdread(kva2pa(sector_buf), 1, FS_START_POS + superblock.sector_map_offset + i);
        sector_map_pointer = sector_buf;
        for(int j = 0; j < SECTOR_SIZE; j++){
            if(*sector_map_pointer == 0xff){
                sector_used += 8;
            }
            sector_map_pointer++;
        }
    }

    return sector_used;
}

int get_used_inode_num(){
    int inode_used = 0;
    uint8_t *inode_map_pointer = sector_buf;
    uint8_t byte_buf;
    // inode_map仅有一个sector
    bios_sdread(kva2pa(sector_buf), 1, FS_START_POS + superblock.inode_map_offset);
    for(int i = 0; i < SECTOR_SIZE; i++){
        if(*inode_map_pointer == 0xff){
            inode_used += 8;
        }
        else{
            byte_buf = *inode_map_pointer;
            for(int j = 0; j < 8; j++){
                if((byte_buf & num2vec(1)) != 0){
                    inode_used++;
                }
                byte_buf = byte_buf << 1;
            }
        }
        inode_map_pointer++;
    }
    return inode_used;
}

// 使用name查找目录中的单级路径，返回查找到的inode，未查找到返回-1
int find_inode_number_once(char *name, dentry_t *dir_entry){
    for(int i = 0; i < DIR_ENTRY_NUM; i++){
        if(strcmp(name, dir_entry[i].file_name) == 0){
            return dir_entry[i].inode_number;
        }
    }
    return -1;
}

// 使用name查找相对于当前目录的多级路径，返回查找到的inode，未查找到返回-1
// 注意此函数同时会设置dir_buf
int find_inode_number(char *path){
    char single_path[FILE_NAME_LEN_MAX];
    int inode_number;
    for(int i = 0; ; i++){
        if(get_single_path(single_path, path, i) == -1){
            break;
        }
        else{
            if(i == 0){
                inode_number = set_dir_buf_once(single_path, current_dir.dir_entry);
            }
            else{
                inode_number = set_dir_buf_once(single_path, dir_entry_buf);
            }
            if(inode_number == -1){
                return -1;
            }
        }
    }
    return inode_number;
}

// 获取多级路径最后的文件名
void get_file_name(char *path, char *name_buf){
    for(int i = 0; ; i++){
        if(get_single_path(name_buf, path, i) == -1){
            return;
        }
    }
}

void load_inode(void* buf_i, void *buf_s, int inode_number){
    bios_sdread(kva2pa(buf_s), 1, FS_START_POS + superblock.inode_offset + inode_number);
    memcpy(buf_i, sector_buf, sizeof(inode_t));
}

void store_inode(void* buf_i, void *buf_s, int inode_number){
    memcpy(buf_s, buf_i, sizeof(inode_t));
    bios_sdwrite(kva2pa(buf_s), 1, FS_START_POS + superblock.inode_offset + inode_number);
}

// 分配空闲inode，设置inode map
// 返回分配的inode号
int alloc_one_inode(){
    int inode_number = 0;
    int inode_rem = 0;
    uint8_t *inode_map_pointer = sector_buf;
    uint8_t byte_buf;
    // inode_map仅有一个sector
    bios_sdread(kva2pa(sector_buf), 1, FS_START_POS + superblock.inode_map_offset);
    while(1){
        if(*inode_map_pointer == 0xff){
            inode_map_pointer++;
            inode_number += 8;
        }
        else{
            // 该byte不满，存在空位可以分配
            byte_buf = *inode_map_pointer;
            for(int j = 0; j < 8; j++){
                if((byte_buf & num2vec(1)) != 0){
                    inode_rem++;
                    inode_number++;
                    byte_buf = byte_buf << 1;
                }
                else{
                    break;
                }
            }
            break;
        }
    }
    *inode_map_pointer = (*inode_map_pointer | (1 << (7 - inode_rem)));

    bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + superblock.inode_map_offset);
    return inode_number;
}

int free_one_inode(int inode_number){
    int found_flag = 0;
    int inode_rem = 0;
    int inode_count = 0;
    int mask = 0;
    uint8_t *inode_map_pointer = sector_buf;
    uint8_t byte_buf;
    bios_sdread(kva2pa(sector_buf), 1, FS_START_POS + superblock.inode_map_offset);
    // 找到该inode numebr对应的位
    inode_number++; // 因为inode是从0开始计数
    while(1){
        // 只能一位一位找
        byte_buf = *inode_map_pointer;
        for(int j = 0; j < 8; j++){
            if((byte_buf & num2vec(1)) != 0){
                inode_rem++;
                inode_count++;
                if(inode_count == inode_number){
                    found_flag = 1;
                    break;
                }
            }
            byte_buf = byte_buf << 1;
        }
        if(found_flag == 1){
            break;
        }
        else{
            inode_map_pointer++;
        }
    }
    for(int i = 0; i < 8; i++){
        if(i != 8 - inode_rem){
            mask = (mask | (1 << i));
        }
    }
    *inode_map_pointer = (*inode_map_pointer & mask);

    bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + superblock.inode_map_offset);
    return 0;
}

/* NOTE: 假设：
*        1. sector map 8 sector对齐
*        2. 最多只使用一级间接索引
*   
*        注意size的单位全部为sector
*        维护inode内的file_size项
*/
int alloc_sector(inode_t *inode_pointer, int size_to_alloc){
    int sector_map_idx = 0;
    int sector_number = 0;
    int already_have_size = inode_pointer->file_size;
    int alloced_size = 0;
    int direct_block_idx;
    int indirect_block_idx;
    int total_block = (already_have_size / BLOCK2SECTOR) + (already_have_size % BLOCK2SECTOR != 0);
    int indirect_block_pointer_idx = 0;
    if(total_block >= 10){
        direct_block_idx = -1;
        indirect_block_idx = (total_block - 10) % (BLOCK_SIZE / sizeof(uint32_t));
        indirect_block_pointer_idx = (total_block - 10) / (BLOCK_SIZE / sizeof(uint32_t));
    }
    else{
        direct_block_idx = total_block;
        indirect_block_idx = 0;
    }
   
    uint8_t *sector_map_pointer = sector_buf;
    bios_sdread(kva2pa(sector_buf), 1, FS_START_POS + superblock.sector_map_offset + sector_map_idx);
    while(alloced_size < size_to_alloc){
        if(*sector_map_pointer == 0xff){
            // do nothing here
        }
        else{
            // 分配出空闲块
            if(direct_block_idx <= 9 && direct_block_idx != -1){
                // 使用直接索引
                inode_pointer->direct_blocks[direct_block_idx] = sector_number;
                direct_block_idx++;
                alloced_size += 8;
            }
            else{
                // 需要使用一级间接块索引
                // 如果当前文件没有间接块，则首先将当前的空闲块用作一级间接块分配给该文件
                // 然后继续循环，寻找下一个空闲块用作分配
                if(inode_pointer->indirect_blocks[indirect_block_pointer_idx] == -1){
                    inode_pointer->indirect_blocks[indirect_block_pointer_idx] = sector_number;
                }
                else{
                    //bzero(block_buf, BLOCK_SIZE);
                    //bios_sdwrite(kva2pa(block_buf), 8, FS_START_POS + sector_number);
                    bios_sdread(kva2pa(indirect_block_buf), 8, FS_START_POS + 
                                inode_pointer->indirect_blocks[indirect_block_pointer_idx]);
                    indirect_block_buf[indirect_block_idx] = sector_number;
                    bios_sdwrite(kva2pa(indirect_block_buf), 8, FS_START_POS + 
                                inode_pointer->indirect_blocks[indirect_block_pointer_idx]);
                    if(indirect_block_idx < (BLOCK_SIZE / sizeof(uint32_t)) - 1){
                        indirect_block_idx++;
                    }
                    else{
                        indirect_block_pointer_idx++;
                        indirect_block_idx = 0;
                    }
                    alloced_size += 8;
                }
            }
            *sector_map_pointer = 0xff;            
        }
        sector_number += 8;
        if((int)(sector_map_pointer - sector_buf) == SECTOR_SIZE - 1){
            // 当前sector map已经全部用完
            // 写回旧块，读入新块
            bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + superblock.sector_map_offset + sector_map_idx);
            sector_map_idx++;
            bios_sdread(kva2pa(sector_buf), 1, FS_START_POS + superblock.sector_map_offset + sector_map_idx);
            sector_map_pointer = sector_buf;
        }
        else{
            // 当前sector map尚未全部用完
            sector_map_pointer++;
        }
    }
    // 写回最后用到的sector map
    bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + superblock.sector_map_offset + sector_map_idx);
    inode_pointer->file_size += size_to_alloc;
    return 0; // success
}

/* NOTE: 假设：
*        1. sector map 8 sector对齐
*/
int free_one_file_sector(inode_t *inode_pointer){
    int sector_map_idx;
    int sector_map_offset;
    int direct_block_count;
    int indirect_block_count;
    int total_block = (inode_pointer->file_size / BLOCK2SECTOR) + (inode_pointer->file_size % BLOCK2SECTOR != 0);
    int indirect_block_pointer_count;
    if(total_block > 10){
        direct_block_count = 10;
        indirect_block_count = (total_block - 10) % (BLOCK_SIZE / sizeof(uint32_t));
    }
    else{
        direct_block_count = total_block;
        indirect_block_count = -1; // 此处需要设置为-1，以表示未使用间接块
    }
    indirect_block_pointer_count = (total_block - 10) / (BLOCK_SIZE / sizeof(uint32_t)) + (indirect_block_count != 0);

    // 释放直接索引
    for(int i = 0; i < direct_block_count; i++){
        sector_map_idx = inode_pointer->direct_blocks[i] / (SECTOR_SIZE * 8);
        sector_map_offset = inode_pointer->direct_blocks[i] % (SECTOR_SIZE * 8);
        bios_sdread(kva2pa(sector_buf), 1, FS_START_POS + superblock.sector_map_offset + sector_map_idx);
        bzero(sector_buf + (sector_map_offset / 8), 1);
        bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + superblock.sector_map_offset + sector_map_idx);
    }
    // 释放间接索引
    if(indirect_block_count != -1){
        for(int j = 0; j < indirect_block_pointer_count; j++){
            bios_sdread(kva2pa(indirect_block_buf), 8, FS_START_POS + inode_pointer->indirect_blocks[j]);
            if(j == indirect_block_pointer_count - 1){
                indirect_block_count = (total_block - 10) % (BLOCK_SIZE / sizeof(uint32_t));
            }
            else{
                indirect_block_count = BLOCK_SIZE / sizeof(uint32_t);
            }
            for(int i = 0; i < indirect_block_count; i++){
                sector_map_idx = indirect_block_buf[i] / (SECTOR_SIZE * 8);
                sector_map_offset = indirect_block_buf[i] % (SECTOR_SIZE * 8);
                bios_sdread(kva2pa(sector_buf), 1, FS_START_POS + superblock.sector_map_offset + sector_map_idx);
                bzero(sector_buf + (sector_map_offset / 8), 1);
                bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + superblock.sector_map_offset + sector_map_idx);
            }
            // 释放索引块
            sector_map_idx = inode_pointer->indirect_blocks[j] / (SECTOR_SIZE * 8);
            sector_map_offset = inode_pointer->indirect_blocks[j] % (SECTOR_SIZE * 8);
            bios_sdread(kva2pa(sector_buf), 1, FS_START_POS + superblock.sector_map_offset + sector_map_idx);
            bzero(sector_buf + (sector_map_offset / 8), 1);
            bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + superblock.sector_map_offset + sector_map_idx);
        }
    }

    return 0;
}

int add_new_dentry(dentry_t *new_dentry, dir_t *father_dir){
    inode_t inode_buf;
    // 在父目录中添加该项
    for(int i = 0; i < DIR_ENTRY_NUM; i++){
        if(strcmp(father_dir->dir_entry[i].file_name, "") == 0){
            strcpy(father_dir->dir_entry[i].file_name, new_dentry->file_name);
            father_dir->dir_entry[i].inode_number = new_dentry->inode_number;
            father_dir->dir_entry[i].file_type = new_dentry->file_type;
            break;
        }
    }
    // 更新父目录hard link number
    if(new_dentry->file_type == DIR){
        bios_sdread(kva2pa(sector_buf), 1, FS_START_POS + superblock.inode_offset + father_dir->inode_number);
        memcpy(&inode_buf, sector_buf, sizeof(inode_t));
        inode_buf.nlinks++;
        memcpy(sector_buf, &inode_buf, sizeof(inode_t));
        bios_sdwrite(kva2pa(sector_buf), 1, FS_START_POS + superblock.inode_offset + father_dir->inode_number);
    }

    bios_sdwrite(kva2pa(father_dir->dir_entry), 8, FS_START_POS + father_dir->start_sector_number);
}

int del_dentry(char *path, dir_t *father_dir){
    inode_t inode_buf;
    int is_dir;
    // 在父目录中删除该项
    for(int i = 0; i < DIR_ENTRY_NUM; i++){
        if(strcmp(father_dir->dir_entry[i].file_name, path) == 0){
            if(father_dir->dir_entry[i].file_type == DIR){
                is_dir = 1;
            }
            else{
                is_dir = 0;
            }
            memset(&(father_dir->dir_entry[i]), 0, sizeof(father_dir->dir_entry[i]));
            break;
        }
    }
    // 更新父目录hard link number
    if(is_dir){
        load_inode(&inode_buf, sector_buf, father_dir->inode_number);
        inode_buf.nlinks--;
        store_inode(&inode_buf, sector_buf, father_dir->inode_number);
    }

    bios_sdwrite(kva2pa(father_dir->dir_entry), 8, FS_START_POS + father_dir->start_sector_number);
}

int do_cd_once(char *single_path){
    inode_t inode_buf;
    int inode_number = find_inode_number_once(single_path, current_dir.dir_entry);
    if(inode_number == -1){
        return -1;
    }
    // 更新当前目录为目标目录
    strcpy(current_dir.dir_name, single_path);
    current_dir.inode_number = inode_number;
    bios_sdread(kva2pa(sector_buf), 1, FS_START_POS + superblock.inode_offset + inode_number);
    memcpy(&inode_buf, sector_buf, sizeof(inode_t));
    current_dir.start_sector_number = inode_buf.direct_blocks[0];
    bios_sdread(kva2pa(current_dir.dir_entry), 8, FS_START_POS + current_dir.start_sector_number);

    return 0;  // do_cd succeeds
}

// 从path中取出第level级的single path
// level出错时返回-1，其余情况返回0
// 目录出错不在此处判断
int get_single_path(char *single_path, char *path, int level){
    char *p_s = single_path;
    char *p_p = path;
    for(int i = 0; i < level; i++, p_p++){
        while(*p_p != '\0' && *p_p != '/'){
            p_p++;
        }
        if(*p_p == '\0'){
            return -1;
        }
    }
    while(*p_p != '\0' && *p_p != '/'){
        *p_s = *p_p;
        p_s++;
        p_p++;
    }
    *p_s = '\0';
    return 0;
}

// 返回设置目录的inode number
int set_dir_buf_once(char *single_path, dentry_t *dir_to_search){
    inode_t inode_buf;
    int inode_number = find_inode_number_once(single_path, dir_to_search);
    if(inode_number == -1){
        return -1;
    }
    // 更新dir_buf为目标目录
    load_inode(&inode_buf, sector_buf, inode_number);
    if(inode_buf.file_type == DIR){
        bios_sdread(kva2pa(dir_entry_buf), 8, FS_START_POS + inode_buf.direct_blocks[0]);
    }

    return inode_number;  // do_cd succeeds
}