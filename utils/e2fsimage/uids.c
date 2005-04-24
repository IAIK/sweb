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
 * $Id: uids.c,v 1.1 2005/04/24 18:33:07 woswasi Exp $ 
 *
 */                           

#include "e2fsimage.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
			
/*
 * read the .UIDGID file
 */

int read_uids(e2i_ctx_t *e2c, uiddb_t *db)
{
	FILE *fp;
	char *fname, *line_buf, *ename, *uname;
	int n, ln=0, uid, gid, len;

	line_buf = (char *)malloc(672 * sizeof(char));
	fname = line_buf + 256;
	ename = fname + 256;
	uname = ename + 80;

	/* prepare the filename PATH/.UIDGID */
	len = strlen(e2c->curr_path);
	strncpy(fname, e2c->curr_path, 256);
	if (fname[len-1] != '/') {
		fname[len++] = '/';
	}
	strncpy((fname + len), e2c->uid_file, 256-len);
	
	/* try to open the file or return */
	fp = fopen(fname, "r");
	if(!fp){
		free(line_buf);
		return 0;
	}
	
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
		
		uid = gid = 0;
		n = sscanf(line_buf, " %79s %d %d", ename, &uid, &gid);
		
		/* check for comment lines */
		if (ename[0] == '\n' || line_buf[0] == '#' ) continue;

		/* how much parameters were given ? */
		if (n < 2 || n > 3) {
			n = sscanf(line_buf, " %79s %79s ", ename, uname);
			if (n !=2) {
				fprintf(stderr, "Bad entry in %s, line %d (%s)\n",
					e2c->curr_path, ln, fname);
				free(line_buf);
				return -1;
			}
			/* fetch uid and gid from passwd */
			if (!uiddb_search(e2c->passwd, uname, &uid, &gid)) {
				fprintf(stderr, 
						"Username %s from %s line %d not found in '%s'\n",
						uname, fname, ln, e2c->pw_file);
				free(line_buf);
				return -1;
			}
		}
		uiddb_add(db, ename, uid, gid);		
	}
	free(line_buf);
	return 0;
}				

int modinode(e2i_ctx_t *e2c, const char *fname, ext2_ino_t e2ino)
{
	struct ext2_inode inode;
	int uid, gid, ret;
	
	/* read the inode, alter uid, gid  and write it back */
	ret = ext2fs_read_inode(e2c->fs, e2ino, &inode);
	E2_ERR(ret, "Ext2 read Inode Error", "");

	/* do the root squash */
	if (! e2c->preserve_uidgid) {
		inode.i_uid = e2c->default_uid;
		inode.i_gid = e2c->default_gid;
	}

	/* if the filename is mentioned in .UIDGID we must change the owner */
	if (uiddb_search(e2c->uid_db, fname, &uid, &gid)){
		if (e2c->verbose)
			printf("Changing UID and GID for %s (%d:%d)\n", fname, uid, gid);
		inode.i_uid = uid;
		inode.i_gid = gid;
	}		
		
	ret = ext2fs_write_inode(e2c->fs, e2ino, &inode);
	E2_ERR(ret, "Ext2 write Inode Error", "");
	
	return 0;
}
