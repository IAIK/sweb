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
 * $Id: passwd.c,v 1.1 2005/04/24 18:33:07 woswasi Exp $ 
 *
 */                           

#include "e2fsimage.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
			
/*
 * read the etc/passwd file
 */

int read_passwd(e2i_ctx_t *e2c)
{
	FILE *fp;
	char *line_buf, *p1, *p2;
	int n=0, ln=0, uid, gid, len;

	/* try to open the file or return */
	fp = fopen(e2c->pw_file, "r");
	if(!fp){
		return 0;
	}
	
	if (e2c->verbose)
		printf("Reading username information from %s\n", e2c->pw_file);

	line_buf = (char *)malloc(256 * sizeof(char));
	if (line_buf == NULL) {
		fprintf(stderr, "malloc() failed\n");
		return -1;
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
		
		do {
			uid = gid =-1;
			p1  = strchr(line_buf, ':'); /* the : between name and password */
			len = p1 - line_buf;
			if (len > 79 || len < 1) break;
			*p1 = '\0'; /* terminate name */
			 p1 = strchr(p1+1,':') + 1; /* points to  UID */
			 if (!p1) break;
			 p2 = strchr(p1,':');
			 if (!p2) break;
			*p2++ = '\0'; /* delete : and point to GID */
			uid = atoi(p1);
			 p1 = strchr(p2, ':');
			 if (!p1) break;
			*p1 = '\0';
			gid = atoi(p2);
			n=1;	
		} while(0);
		
		if (n != 1) {
			fprintf(stderr, "Bad entry in %s, line %d : %s, %d, %d\n",
				e2c->pw_file, ln, line_buf, uid, gid);
			free(line_buf);
			return -1;
		}
		uiddb_add(e2c->passwd, line_buf, uid, gid);		
	}
	free(line_buf);
	return 0;
}				

