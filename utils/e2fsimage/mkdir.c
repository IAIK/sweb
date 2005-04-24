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
 * $Id: mkdir.c,v 1.1 2005/04/24 18:33:07 woswasi Exp $ 
 *
 */                           

#include "e2fsimage.h"
#include <errno.h>
#include <string.h>

/*
 * this function creates a new directory inode (returned in newdir)
 * and the link in the upper directory (e2c->curr_e2dir)
 * the name is the basename of the current path (e2c->curr_path)
 */
int e2mkdir(e2i_ctx_t *e2c, ext2_ino_t *newdir) {

	int ret;
	struct stat s;
	const char *dname;
	ext2_ino_t nd;
	
	ret = lstat(e2c->curr_path, &s);
	ERRNO_ERR(ret,"Could not 'stat': ", e2c->curr_path);
	
	/* sanity check */
	if (!S_ISDIR(s.st_mode)) {
		fprintf(stderr, "File '%s' is not a directory\n", e2c->curr_path);
		return -1;
	}
	
	/* edit the directoryname and create it */
	dname = basename(e2c->curr_path);
	
	ret = ext2fs_mkdir(e2c->fs, e2c->curr_e2dir, 0, dname);
	if (ret == EXT2_ET_DIR_NO_SPACE) {
		/* resize the directory */
		if (ext2fs_expand_dir(e2c->fs, e2c->curr_e2dir) == 0)
			ret = ext2fs_mkdir(e2c->fs, e2c->curr_e2dir, 0, dname);
	}
	E2_ERR(ret, "Could not create dir: ", dname);

	/* say what we do and increase the counter */
	if (e2c->verbose)
		printf ("Creating directory %s\n", dname);

	e2c->cnt->dir++;
	
	/* lookup the inode of the new directory if requested */
	ret = ext2fs_lookup(e2c->fs, e2c->curr_e2dir, dname, strlen(dname), 0, &nd);
	E2_ERR(ret, "Could not Ext2-lookup: ", dname);
	
	modinode(e2c, dname, nd);
	
	if (newdir) {
		*newdir = nd;
	}
	return 0;
}
