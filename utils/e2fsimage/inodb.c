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
 * $Id: inodb.c,v 1.1 2005/04/24 18:33:07 woswasi Exp $ 
 *
 */                           

#include "e2fsimage.h"
#include <string.h>

#define RESIZE 50

/* 
 * The inode database (inodb) is a lookup table to find inodes
 * from the source filesystem  and their corresponding inode number
 * in the target ext2 filesystem.
 * It is a simple resizeable unsorted array of an integer structure.
 */


/* 
 * allocate initial space for the maintainance structure 
 * and the first 50 inode hashes 
 */
inodb_t *inodb_init(void)
{
	inodb_t *db;
	db = (inodb_t *)malloc(sizeof(inodb_t));
	if (db == NULL) {
		fprintf(stderr, "malloc() failed\n");
		return 0;
	}
	db->size = RESIZE;
	db->cnt=0;
	db->ino_pairs = (struct ino_pair *)malloc(RESIZE * sizeof(struct ino_pair));
	if (db->ino_pairs == NULL) {
		fprintf(stderr, "malloc() failed\n");
		free(db);
		return 0;
	}
	return db;
}

/* 
 * adds an inode pair to the stack 
 * and resizes the stack if nessecary
 */
int inodb_add(inodb_t *db, ino_t ino1, ext2_ino_t ino2)
{
	if (db->cnt >= db->size) {
		struct ino_pair *ptr;
		ptr = realloc(db->ino_pairs, (db->size + RESIZE) * sizeof(struct ino_pair) );
		if (ptr == NULL) {
			fprintf(stderr, "realloc() failed\n");
			return -1;
		}
		db->size += RESIZE;
		db->ino_pairs = ptr;
	}
	db->ino_pairs[db->cnt].ino1 = ino1;
	db->ino_pairs[db->cnt].ino2 = ino2;
	db->cnt++;
	return 0;
}

/*
 * iterates over the inode stack and searches for the supplied source inode.
 * Returns the corresponding ext2inode or 0 if it was not found.
 */
ext2_ino_t inodb_search(inodb_t *db, ino_t ino1)
{
	int i;
	for (i=0; i<db->cnt; i++) {
		if (ino1 == db->ino_pairs[i].ino1) {
			return db->ino_pairs[i].ino2;
		}
	}
	return 0;
}

/* 
 * release all the occupied memory
 */
void inodb_free(inodb_t *db)
{
	if (db) {
		free(db->ino_pairs);
		free(db);
	}
}
