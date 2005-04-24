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
 * $Id: e2fsimage.h,v 1.1 2005/04/24 18:33:07 woswasi Exp $ 
 *
 */                           

#include <e2p/e2p.h>
#include <ext2fs/ext2fs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef E2FSIMAGE_H
#define E2FSIMAGE_H

#define E2_ERR(ret,x,y) if (ret) { fprintf(stderr,"%s(%d): %s%s - ext2 error: %s\n", \
					__FILE__, __LINE__, x, y, error_message(ret)); return ret; }
#define ERRNO_ERR(ret,x,y)  if (ret) { fprintf(stderr,"%s(%d): %s%s - Error: %s\n", \
	                    __FILE__, __LINE__, x, y, strerror(errno)); return ret; }

#define S_ISSF(x) (S_ISCHR(x) || S_ISBLK(x) || S_ISFIFO(x))

/* inode DB */
struct ino_pair {
	ino_t ino1;
	ext2_ino_t ino2;
};

typedef struct {
	int size; 
	int cnt; 
	struct ino_pair *ino_pairs;
} inodb_t ;

/* uid db */
struct uidentry {
	int uid;
	int gid;
	int namelen;
};

typedef struct {
	struct uidentry *first;
	int size;
	int maxsize;
} uiddb_t;

/* global filesystem information */

struct cnt_t {
	int dir, regf, specf, hardln, softln;
};

typedef struct {
    ext2_filsys fs;
    ext2_ino_t curr_e2dir;
    const char *curr_path;

    inodb_t *ino_db;
	uiddb_t *uid_db;
	uiddb_t *passwd;
    int default_uid;
    int default_gid;
    int verbose;
    int preserve_uidgid;
    const char *dev_file;
    const char *uid_file;
    const char *pw_file;
	struct cnt_t *cnt;
} e2i_ctx_t;

int mke2fs(const char *fname, int size);
int init_fs(ext2_filsys *fs, char *fsname, int size);

int e2cp(e2i_ctx_t *e2c);
int e2symlink(e2i_ctx_t *e2c);
int e2mkdir(e2i_ctx_t *e2c, ext2_ino_t *newdir);
int e2cpdir(e2i_ctx_t *e2c, ext2_ino_t newdir);
int e2mknod(e2i_ctx_t *e2c);

int e2filetype_select(e2i_ctx_t *e2c);
int read_special_file(e2i_ctx_t *e2c);

/* functions from util.c */
const char *basename(const char *path);
void init_inode(e2i_ctx_t *e2c, struct ext2_inode *i, struct stat *s);
int e2link(e2i_ctx_t *e2c, const char *fname, ext2_ino_t e2ino, int mode);
__u16 mode2filetype(mode_t m);

/* inode database functions */
inodb_t *inodb_init(void);
int inodb_add(inodb_t *db, ino_t ino1, ext2_ino_t ino2);
ext2_ino_t inodb_search(inodb_t *db, ino_t ino1);
void inodb_free(inodb_t *db);

/* uid database functions */
int uiddb_init(uiddb_t *db);
int uiddb_add(uiddb_t *db, const char* name, int uid, int gid);
int uiddb_search(uiddb_t *db, const char *name, int *uid, int *gid);
void uiddb_free(uiddb_t *db);

int read_uids(e2i_ctx_t *e2c, uiddb_t *db);
int modinode(e2i_ctx_t *e2c, const char *fname, ext2_ino_t e2ino);

int read_passwd(e2i_ctx_t *e2c);
#endif
