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
 * $Id: util.c,v 1.1 2005/04/24 18:33:07 woswasi Exp $ 
 *
 */                           

#include "e2fsimage.h"
#include <string.h>

const char *basename(const char *path)
{
	const char *bn;
	/* extract the filename from the path */
	bn = strrchr(path,'/');
	if (!bn){
		bn = path;
	}
	else {
		bn++;
	}
	/* now ptr points to the basename of 'path' */
	return bn;
}

void init_inode(e2i_ctx_t *e2c, struct ext2_inode *i, struct stat *s)
{
	/* do the root squash */
	if (! e2c->preserve_uidgid) {
		s->st_uid = e2c->default_uid;
		s->st_gid = e2c->default_gid;
	}

	memset(i, 0, sizeof(struct ext2_inode));

	i->i_links_count = 1;
	i->i_mode = s->st_mode;
	i->i_size = s->st_size;
	i->i_uid = s->st_uid;
	i->i_gid = s->st_gid;
	i->i_atime = s->st_atime;
	i->i_ctime = s->st_ctime;
	i->i_mtime = s->st_mtime;
									  
}
			
int e2link(e2i_ctx_t *e2c, const char *fname, ext2_ino_t e2ino, int mode)
{
	int ret;

	modinode(e2c, fname, e2ino);
	
	ret = ext2fs_link(e2c->fs, e2c->curr_e2dir, fname, e2ino, mode);
	if (ret == EXT2_ET_DIR_NO_SPACE) {
		/* resize the directory */
		if (ext2fs_expand_dir(e2c->fs, e2c->curr_e2dir) == 0) {
			ret = ext2fs_link(e2c->fs, e2c->curr_e2dir, fname, e2ino, mode);
		}
	}
	E2_ERR(ret, "Ext2 Link Error", fname);
	return 0;
}	

__u16 mode2filetype(mode_t m)
{
	__u16 e2ft;
	/* select modetype from mode */
	switch (m & S_IFMT) {
		case S_IFDIR : e2ft = EXT2_FT_DIR; break;
		case S_IFREG : e2ft = EXT2_FT_REG_FILE; break;
		case S_IFLNK : e2ft = EXT2_FT_SYMLINK; break;
		case S_IFSOCK: e2ft = EXT2_FT_SOCK; break;
		case S_IFCHR : e2ft = EXT2_FT_CHRDEV; break;
		case S_IFBLK : e2ft = EXT2_FT_BLKDEV; break;
		case S_IFIFO : e2ft = EXT2_FT_FIFO; break;
		default:       e2ft = EXT2_FT_UNKNOWN;
	}
	return e2ft;
}	

