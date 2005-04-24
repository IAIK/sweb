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
 * $Id: sfile.c,v 1.1 2005/04/24 18:33:07 woswasi Exp $ 
 *
 */                           

#include "e2fsimage.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#define BUF_SIZE 4096

/*
 * create a block or character special device file
 * with the filename (fname) the mode (mode) and 
 * the source inode number (ino) or 0 if read from .DEVICES
 */

static int special_inode(e2i_ctx_t *e2c, const char *fname, struct stat *s)
{
	ext2_ino_t e2ino;
	struct ext2_inode inode;
	int ret, e2mod;
	
	/* create a new inode for this special device file */
	ret = ext2fs_new_inode(e2c->fs, e2c->curr_e2dir, s->st_mode, 0, &e2ino);
	E2_ERR(ret, "Could not create new inode for:", fname);
	
	/* populate the new inode */
	ext2fs_inode_alloc_stats(e2c->fs, e2ino, 1);
	
	if (s->st_ino) { 
		ret = inodb_add(e2c->ino_db, s->st_ino, e2ino);
		if (ret) return -1;
	}
	
	/* 
	 * initialize inode and set major and minor.
	 * major and minor do live in i_block[0]
	 * obvious, eh ?? 
	 */
	init_inode(e2c, &inode, s);
	if(s->st_rdev)
		inode.i_block[0] = s->st_rdev;

	/* select modetype from mode */
	e2mod = mode2filetype(s->st_mode);
	
	ret = ext2fs_write_inode(e2c->fs, e2ino, &inode);
	E2_ERR(ret, "Ext2 Inode Error", "");
	
	e2c->cnt->specf++;
	
	/* It is time to link the inode into the directory */
	return e2link(e2c, fname, e2ino, e2mod);
}

/*
 * copy the original special file to the fs
 * this is an unusual but nevertheless supported case,
 * because special files can only be created by r00t
 * using mknod()
 */
int e2mknod(e2i_ctx_t *e2c)
{
	int ret;
	struct stat s;
	
	ret = lstat(e2c->curr_path, &s);
    ERRNO_ERR(ret, "Could not stat: ", e2c->curr_path);
	
	if (!(S_ISSF(s.st_mode)) ) {
		fprintf(stderr, "File '%s' is not a block or charspecial device\n", e2c->curr_path);
		return -1;
	}
	if (e2c->verbose)
		printf("Copying special file: %s\n", e2c->curr_path);

	return special_inode(e2c, basename(e2c->curr_path), &s);
}
			
/*
 * read the .DEVICES file and create all mentioned devices
 */
int read_special_file(e2i_ctx_t *e2c)
{
	FILE *fp;
	char fname[80], *line_buf, type;
	int n, major, minor, mode, ln=0, uid, gid, pug_tmp;
	dev_t rdev;
	struct stat s;

	pug_tmp = e2c->preserve_uidgid;
	e2c->preserve_uidgid = 1;
	
	fp = fopen(e2c->curr_path, "r");
	ERRNO_ERR(!fp, "Failed to open: ", e2c->curr_path);
	fname[0] = '\0';
	
	/* prepare the stat struct */
	memset(&s, 0, sizeof(struct stat));
	s.st_atime = s.st_mtime = s.st_ctime = time(NULL);
	
	line_buf = (char *)malloc(256);
	/* iterate over the lines in the device file */
	while (fgets(line_buf, 256, fp) != 0) {
		ln++;  /* count the line numbers */
		/* check for too long lines */
		if (strlen(line_buf)>254) {
			char c = line_buf[254];
			fprintf(stderr, "Line too long %d\n",ln);
			/* eat up the rest of the line */
			while (c != '\n' && c >0) c = fgetc(fp);
			continue;
		}
		
		s.st_uid = e2c->default_uid;
		s.st_gid = e2c->default_gid;
		s.st_mode = 0600;
		
		n = sscanf(line_buf, "%79s %c %d %d %o %d %d",
			fname, &type, &major, &minor, &mode, &uid, &gid);
		
		/* check for comment lines */
		if (line_buf[0] == '\n' || fname[0] == '#' ) continue;

		/* how much parameters were given ? */
		switch (n) {
			case 7 : s.st_gid = gid; /* uid gid and mode were given */
			case 6 : s.st_uid = uid;
			case 5 : s.st_mode = mode; /* mode was given */
			case 4 : break;
			default:
				fprintf(stderr, "Bad entry in %s, line %d (%s)\n",
					e2c->curr_path, ln, fname);
				free(line_buf);
				fclose(fp);
				return -1;
		}
		
		rdev = (major << 8) + minor;
		switch (type) {
			case 'u' :
			case 'c' : s.st_mode |= S_IFCHR; break;
			case 'b' : s.st_mode |= S_IFBLK; break;
			case 'p' : 
			case 'f' : s.st_mode |= S_IFIFO; break;
			default:
				fprintf(stderr, "Bad mode (%c) in %s, line %d\n",
					type, e2c->curr_path, ln);
				free(line_buf);
				fclose(fp);
				return -1;
		}
		
		if (e2c->verbose)
			printf("Creating special file: %s (%c, Major %d, Minor: %d)\n",
					fname, type, major, minor);

		s.st_rdev = rdev;
		special_inode(e2c, fname, &s);
		fname[0] = '\0';
	}
	e2c->preserve_uidgid = pug_tmp;
	free(line_buf);
	fclose(fp);
	return 0;
}				
