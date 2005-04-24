/* vi: set sw=4 ts=4: */
/*
 * Copyright (C) 2001 Christian Hohnstaedt.
 *
 *  All rights reserved.
 *
 *
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  - Neither the name of the author nor the names of its contributors may be 
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *
 * http://www.hohnstaedt.de/e2fsimage
 * email: christian@hohnstaedt.de
 *
 * $Id: initfs.c,v 1.1 2005/04/24 18:33:07 woswasi Exp $ 
 *
 */                           

#include "e2fsimage.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define STRIDE_LENGTH 8

static errcode_t zero_blocks(ext2_filsys fs, blk_t blk, int num,
                             blk_t *ret_blk, int *ret_count)
{
        int             j, count, next_update, next_update_incr;
        static char     *buf;
        errcode_t       retval;

        /* If fs is null, clean up the static buffer and return */
        if (!fs) {
                if (buf) {
                        free(buf);
                        buf = 0;
                }
                return 0;
        }
        /* Allocate the zeroizing buffer if necessary */
        if (!buf) {
                buf = malloc(fs->blocksize * STRIDE_LENGTH);
                if (!buf) {
						fprintf(stderr, "malloc() failed\n");
                        return -1; 
                }
                memset(buf, 0, fs->blocksize * STRIDE_LENGTH);
        }
        /* OK, do the write loop */
        next_update = 0;
        next_update_incr = num / 100;
        if (next_update_incr < 1)
                next_update_incr = 1;
        for (j=0; j < num; j += STRIDE_LENGTH, blk += STRIDE_LENGTH) {
                if (num-j > STRIDE_LENGTH)
                        count = STRIDE_LENGTH;
                else
                        count = num - j;
                retval = io_channel_write_blk(fs->io, blk, count, buf);
                if (retval) {
                        if (ret_count)
                                *ret_count = count;
                        if (ret_blk)
                                *ret_blk = blk;
                        return retval;
                }
        }
        return 0;
}


static void write_inode_tables(ext2_filsys fs)
{
        errcode_t       retval;
        blk_t           blk;
        int             i, num;

        for (i = 0; i < fs->group_desc_count; i++) {

                blk = fs->group_desc[i].bg_inode_table;
                num = fs->inode_blocks_per_group;

                retval = zero_blocks(fs, blk, num, &blk, &num);
                if (retval) {
                        fprintf(stderr, "Could not write %d blocks "
                                "in inode table starting at %d: %s\n",
                                num, blk, error_message(retval));
                }
        }
        zero_blocks(0, 0, 0, 0, 0);
}

static void create_bad_block_inode(ext2_filsys fs, badblocks_list bb_list)
{
        errcode_t       retval;

        ext2fs_mark_inode_bitmap(fs->inode_map, EXT2_BAD_INO);
        fs->group_desc[0].bg_free_inodes_count--;
        fs->super->s_free_inodes_count--;
        retval = ext2fs_update_bb_inode(fs, bb_list);
        if (retval) {
			//error
        }

}

static int create_root_dir(ext2_filsys fs)
{
	errcode_t retval;
	struct ext2_inode inode;

	retval = ext2fs_mkdir(fs, EXT2_ROOT_INO, EXT2_ROOT_INO, 0);
	if (retval) {
		fprintf(stderr, "Error while creating root dir");
		return 1;
	}
	retval = ext2fs_mkdir(fs, EXT2_ROOT_INO, 0, "lost+found");
	if (retval) {
		fprintf(stderr, "Error while creating lnf dir");
		return 1;
	}
#if 0
	retval = ext2fs_read_inode(fs, EXT2_ROOT_INO, &inode);
	if (retval) {
		fprintf(stderr, "Error while reading root inode");
		return 1;
	}
	
	inode.i_uid = default_uid;
	inode.i_gid = default_gid;
	
	retval = ext2fs_write_inode(fs, EXT2_ROOT_INO, &inode);
	if (retval) {
		fprintf(stderr, "Error while setting root inode ownership");
		return 1;
	}
#endif
	
	return 0;
}


int init_fs(ext2_filsys *fs, char *fsname, int size)
{
	struct ext2_super_block super;
	int ret, i;
	struct stat s;
	char *buf;
	FILE *fp;
	badblocks_list  bb_list = 0;

	memset(&super, 0, sizeof(struct ext2_super_block) );

	super.s_rev_level = 1; 
	super.s_feature_incompat |= EXT2_FEATURE_INCOMPAT_FILETYPE;
	super.s_feature_ro_compat |= EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER;
	super.s_blocks_count = size; /* size is in kilobyte */
	super.s_creator_os = EXT2_OS_LINUX;
	super.s_log_block_size = 0;
	super.s_log_frag_size = 0;
	super.s_inodes_count = size >> 3;
	super.s_free_inodes_count = 119;
	
	ret = stat(fsname, &s);
	
	if (!ret) {  /* is the existing file a regular one ? */
		if (!S_ISREG(s.st_mode)) {
			fprintf(stderr, "Operation on non regular file '%s' cowardly refused\n",
				   	fsname);
			return -1;
		}
	}
	fp = fopen(fsname, "wb+");
	if (!fp) {
		perror("Error opening file");
		return 1;
	}
	
	buf = malloc(1024);
	if (!buf) {
		fclose(fp);
		return -1;
	}
	memset(buf, 0, 1024 );
	
	for (i=0; i<size; i++) {
		fwrite(buf, 1024, 1, fp);
	}
	free(buf);
	fclose(fp);

	ret = ext2fs_initialize(fsname, 0, &super, unix_io_manager, fs);

	if (ret) {
		fprintf(stderr, "Error while setting up superblock (%d) on file '%s'\n",
			ret, fsname);
		return 2;
	}
	(*fs)->super->s_max_mnt_count = 0;
	(*fs)->super->s_checkinterval = 0;
	(*fs)->blocksize = 1024;
	(*fs)->fragsize = 1024;

	ret = ext2fs_allocate_tables(*fs);
	if (ret) {
		fprintf(stderr, "Error allocating the inode tables on file '%s'\n", fsname);
		return 2;
	}
	printf("group_desc_count :%d, inode_blocks_per_group: %d\n",
		   	(*fs)->group_desc_count, (*fs)->inode_blocks_per_group);
	
	for (i = 0; i < (*fs)->group_desc_count; i++) {
		printf("bg_inode_table-%d: %d\n",i,(*fs)->group_desc[i].bg_inode_table);
	}								   		   
	//(*fs)->super->s_free_inodes_count = 119;
	//write_inode_tables(*fs);
	create_root_dir(*fs);
	create_bad_block_inode(*fs, bb_list);
}
