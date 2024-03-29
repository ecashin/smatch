/*
 * sparse/smatch_files.c
 *
 * Copyright (C) 2009 Dan Carpenter.
 *
 *  Licensed under the Open Software License version 1.1
 *
 */

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "parse.h"
#include "smatch.h"

int open_data_file(const char *filename)
{
	int fd;
	char buf[256];

	fd = open(filename, O_RDONLY);
	if (fd >= 0)
		goto exit;
	if (!data_dir)
		goto exit;
	snprintf(buf, 256, "%s/%s", data_dir, filename);
	fd = open(buf, O_RDONLY);
exit:
	return fd;
}

struct token *get_tokens_file(const char *filename)
{
	int fd;
	struct token *token;

	if (option_no_data)
		return NULL;
	fd = open_data_file(filename);
	if (fd < 0)
		return NULL;
	token = tokenize(filename, fd, NULL, NULL);
	close(fd);
	return token;
}
