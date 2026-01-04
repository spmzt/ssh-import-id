/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2022-2026 Seyed Pouria Mousavizadeh Tehrani <info@spmzt.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <curl/curl.h>

/* 39 is the maximum username characters in github */
#define GH_MAX_USERNAME		39
/* 32 is the maximum username characters in launchpad */
#define LP_MAX_USERNAME		32
#define MAXURLSIZE		128
#define MAX_USERAGENT_LEN	200

struct MemoryStruct {
	char *memory;
	size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		/* out of memory */
		printf("not enough memory (realloc returned NULL)\n");
		return (0);
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return (realsize);
}

void
usage()
{

	fprintf(stdout, "Parameters: [-t] [-u User-Agent] [Provider:]<Username>\n");
	return;
}

/* Free returned value after use */
char *
get_url_of_ssh_keys(char *username, char *provider)
{
	char *url;

	url = (char *)malloc(MAXURLSIZE);
	if (url == NULL)
		exit(EXIT_FAILURE);

	if (!strcmp(provider, "gh")) {
		if (strlen(username) > GH_MAX_USERNAME)
			errx(EINVAL, "username is too long for github.\n");

		snprintf(url, MAXURLSIZE, "https://github.com/%s.keys", username);
	} else if (!strcmp(provider, "lp")) {
		if (strlen(username) > LP_MAX_USERNAME)
			errx(EINVAL, "username is too long for launchpad.\n");

		snprintf(url, MAXURLSIZE, "https://launchpad.net/~%s/+sshkeys", username);
	} else
		errx(EINVAL, "Providers other than gh (github) and lp (launchpad) is not implemented. Selected provider is %s.\n", provider);

	return (url);
}

int
main(int argc, char *argv[])
{
	int c, errflag = 0;
	int authorized_keys_mode, fd = -1, truncate = 0;
	char authorized_keys[PATH_MAX];
	char *useragent, *provider, *home, *url;
	extern char *optarg;
	extern int optind, optopt;
	size_t optlen;
	struct MemoryStruct chunk;

	/* set default value */
	useragent = strdup("libcurl-agent/1.0");

	while ((c = getopt(argc, argv, "tu:h")) != -1) {
		switch (c) {
		case 't':
			truncate++;
			break;

		case 'u':
			optlen = strlen(optarg) + 1;
			if (strlen(optarg) > MAX_USERAGENT_LEN)
				errx(EINVAL, "User-Agent is too big. Keep it under 200 characters.\n");

			useragent = realloc(useragent, optlen);
			if (useragent == NULL)
				exit(EXIT_FAILURE);

			memcpy(useragent, optarg, optlen);
			break;

		case ':':
			fprintf(stderr, "-u require an argument.");
			errflag++;
			break;

		case 'h':
			errflag++;
			break;

		default:
			fprintf(stderr, "Unrecognised option: -%c\n", optopt);
			errflag++;
		}
	}

	if (errflag != 0 || optind == argc || optind + 1 < argc)
		goto usage;

	/* provider detection */
	provider = (char *)malloc(strlen(argv[optind]) - 1);
	if (provider == NULL)
		goto error;
	if (strchr(argv[optind], ':') != NULL) {
		if (((provider = strsep(&argv[optind], ":")) == NULL) ||
		    (strlen(provider) == 0)) {
			fprintf(stdout, "Provider can not be empty.\n");
			goto usage;
		}
	} else
		/* set the default provider to the launchpad to be backward compatible */
		provider = "lp";

	/* get home directory */
	home = getenv("HOME");
	if ((home == NULL) || (strlen(home) > PATH_MAX)) {
		fprintf(stderr, "HOME env is not valid.\n");
		goto usage;
	}

	/* get or create ssh directory */
	snprintf(authorized_keys, sizeof(authorized_keys), "%s/.ssh", home);
	if (mkdir(authorized_keys, 0700) == -1 && EEXIST != errno) {
		fprintf(stderr, "Create %s directory failed: %s\n",
		    authorized_keys, strerror(errno));
		exit(EXIT_FAILURE);
	}

	strlcat(authorized_keys, "/authorized_keys", sizeof(authorized_keys));

	/* curl structure to fetch key */
	CURL *curl_handle;
	CURLcode res;
	chunk.memory = malloc(1);	/* will be grown as needed by the realloc above */
	chunk.size = 0;
	url = get_url_of_ssh_keys(argv[optind], provider);

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1L);

		/* overwrite or append the keys */
		authorized_keys_mode = O_APPEND;
		if (truncate > 0)
			authorized_keys_mode = O_TRUNC;

		fd = open(authorized_keys,
		    O_WRONLY | O_CREAT | authorized_keys_mode, 0600);
		if (fd < 0) {
			fprintf(stderr, "Can't open the %s: %s\n",
			    authorized_keys, strerror(errno));
			goto error;
		}

		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
		/*
		 * some servers do not like requests that are made
		 * without a user-agent field, so we provide one'
		 */
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, useragent);
		if (((res = curl_easy_perform(curl_handle)) != CURLE_OK) ||
		   (res == CURLE_HTTP_RETURNED_ERROR)) {
			fprintf(stderr, "curl failed: %s\n",
			    curl_easy_strerror(res));
			goto error;
		}

		if (strcmp(chunk.memory, "Not Found") == 0) {
			fprintf(stderr, "Account Not Found.\n");
			goto error;
		}

		if (write(fd, chunk.memory, (int)chunk.size) != (int)chunk.size) {
			fprintf(stderr, "Write on %s failed: %s\n",
			    authorized_keys, strerror(errno));
			goto error;
		} else
			printf("Done.\n");

		close(fd);
		free(useragent);
		free(chunk.memory);
		curl_easy_cleanup(curl_handle);
	}
	curl_global_cleanup();
	return (0);

usage:
	usage();

error:
	if (useragent != NULL)
		free(useragent);
	if (fd != -1)
		close(fd);

	exit(EXIT_FAILURE);
}
