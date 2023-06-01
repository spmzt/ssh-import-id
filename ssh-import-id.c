/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2022, 2023, Seyed Pouria Mousavizadeh Tehrani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *	  may be used to endorse or promote products derived from this software
 *	  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
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
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <curl/curl.h>

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
  if(mem->memory == NULL) {
	/* out of memory */
	printf("not enough memory (realloc returned NULL)\n");
	return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

void
usage()
{
		fprintf(stdout, "Parameters: [-u User-Agent] [Provider:]<Username>\n");
		exit(EXIT_FAILURE);
}

char*
get_url_of_ssh_keys(char *username, char *provider)
{
	if (!strcmp(provider, "gh")) {
		if (strlen(username) > 39) {
			fprintf(stderr, "username is too long for github.\n");
			exit(EXIT_FAILURE);
		}
		char *github_url = (char*)malloc(19 + 39 + 5 + 1);
		if (github_url == NULL) {
			fprintf(stderr, "malloc() failed\n");
			exit(EXIT_FAILURE);
		}
		strcpy(github_url, "https://github.com/");
		strncat(github_url, username, 39); /* 39 is the maximum username characters in github */
		strncat(github_url, ".keys", sizeof(github_url) - strlen(github_url) - 1);
		return github_url;
	} else if (!strcmp(provider, "lp")) {
		if (strlen(username) > 32) {
			fprintf(stderr, "username is too long for launchpad.\n");
			exit(EXIT_FAILURE);
		}
		char *launchpad_url = (char*)malloc(23 + 32 + 9 + 1);
		if (launchpad_url == NULL) {
			fprintf(stderr, "malloc() failed\n");
			exit(EXIT_FAILURE);
		}
		strcpy(launchpad_url, "https://launchpad.net/~");
		strncat(launchpad_url, username, 32); /* 32 is the maximum username characters in launchpad */
		strncat(launchpad_url, "/+sshkeys", sizeof(launchpad_url) - strlen(launchpad_url) - 1);
		return launchpad_url;
	} else {
		fprintf(stderr, "Providers other than gh (github) and lp (launchpad) is not implemented. Selected provider is %s.\n", provider);
		exit(EXIT_FAILURE);
	}
}

int
main(int argc, char *argv[])
{
	int c;
	int errflag = 0;
	char* useragent = (char*)malloc(18);
	if (useragent == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	strcpy(useragent, "libcurl-agent/1.0");
	extern char* optarg;
    extern int optind, optopt;
	while ((c = getopt(argc, argv, "u:")) != -1) {
		switch(c) 
		{
			case 'u':
				useragent = realloc(useragent, sizeof(optarg));
				if (useragent == NULL) {
					fprintf(stderr, "malloc() failed\n");
					exit(EXIT_FAILURE);
				}
				strcpy(useragent, optarg);
				break;
				case ':':
					fprintf(stderr, "-u require an argument.");
					errflag++;
					break;
        	case '?':
               	fprintf(stderr, "Unrecognised option: -%c\n", optopt);
        	    errflag++;
				break;
			default:
        	    errflag++;
		}
	}

	if (errflag) {
		free(useragent);
		usage();
	}

	if ((optind == argc) || (optind + 1 < argc)) {
		free(useragent);
		usage();
	}

	/* provider detection */
	char* provider = (char*)malloc(strlen(argv[optind]) - 1);
	if (provider == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	if (strchr(argv[optind], ':') != NULL) {
		if ((provider = strsep(&argv[optind], ":")) == NULL) {
			fprintf(stderr, "Provider can not be empty.\n");
			exit(EXIT_FAILURE);
		} else if (strlen(provider) == 0) {
			fprintf(stderr, "Provider can not be empty.\n");
			exit(EXIT_FAILURE);
		}
	} else {
		/* set the default provider to the launchpad to be backward compatible */
		provider = "lp";
	}

	/* get home directory */
	char *home;
	home = getenv("HOME");
	if (home == NULL) {
		fprintf(stderr, "HOME env is not set.\n");
		exit(EXIT_FAILURE);
	}

	/* get or create ssh directory */
	char *ssh_directory = strdup(home);
	strncat(ssh_directory, "/.ssh", sizeof(ssh_directory) - strlen(ssh_directory) - 1);
	if (mkdir(ssh_directory, 0700) == -1 && EEXIST != errno) {
		fprintf(stderr, "Create %s directory failed: %s\n", ssh_directory, strerror(errno));
		exit(EXIT_FAILURE);
	}

	char *authorized_keys = (char*)malloc(strlen(ssh_directory) + 16 + 1);
	if (authorized_keys == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	strcpy(authorized_keys, ssh_directory);
	free(ssh_directory);
	strncat(authorized_keys, "/authorized_keys", sizeof(authorized_keys) - strlen(authorized_keys) - 1);

	/* get url of ssh keys */
	char* url = get_url_of_ssh_keys(argv[optind], provider);

	/* curl structure to fetch key */
	CURL *curl_handle;
	CURLcode res;

	struct MemoryStruct chunk;

	chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	curl_global_init(CURL_GLOBAL_ALL);

	curl_handle = curl_easy_init();

	if(curl_handle) {


		/* set url for curl */
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);

		/* send all data to this function  */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

		/* request failure on HTTP response */
		curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1L);

		/* open the file */
		int fd;
		if ((fd = open(authorized_keys, O_WRONLY | O_CREAT | O_APPEND, 0600 )) == -1) {
			 fprintf(stderr, "Can't open the %s: %s\n", authorized_keys, strerror(errno));
			 exit(EXIT_FAILURE);
		}

		/* write the page body to this file handle */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
	
		/* some servers do not like requests that are made without a user-agent field, so we provide one */
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, useragent);

		/* get it! */
		if ((res = curl_easy_perform(curl_handle)) != CURLE_OK) {
			fprintf(stderr, "curl failed: %s\n",
					curl_easy_strerror(res));
			close(fd);
			exit(EXIT_FAILURE);
		} else if(res == CURLE_HTTP_RETURNED_ERROR) {
			/* an HTTP response error problem */
			fprintf(stderr, "Fail: %s\n",
					curl_easy_strerror(res));
			close(fd);
			exit(EXIT_FAILURE);
  		}
		free(useragent);

		if (strcmp(chunk.memory, "Not Found") == 0) {
			fprintf(stderr, "Account Not Found.\n");
			exit(EXIT_FAILURE);
		}

		if (write(fd, chunk.memory, (int)chunk.size) != (int)chunk.size) {
			fprintf(stderr, "Write on %s failed: %s\n", authorized_keys, strerror(errno));
			exit(EXIT_FAILURE);
		}

		/* close the header file */
		close(fd);

		free(authorized_keys);
		free(chunk.memory);

		/* always cleanup */
		curl_easy_cleanup(curl_handle);
	}
	curl_global_cleanup();
	return 0;
}
