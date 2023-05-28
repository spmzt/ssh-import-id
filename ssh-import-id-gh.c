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
        fprintf(stdout, "Parameters: <Github Username>\n");
        exit(EXIT_FAILURE);
}

char*
get_url_of_ssh_keys(char *username, char *provider)
{
    if (!strcmp(provider, "github")) {
        char *github_url = (char*)malloc(18 + 39 + 5 + 1);
        if (github_url == NULL) {
            fprintf(stderr, "malloc() failed\n");
            exit(EXIT_FAILURE);
        }
        strcpy(github_url, "https://github.com/");
        strncat(github_url, username, 39); /* 39 is the maximum username characters in github */
        strncat(github_url, ".keys", 5);
        return github_url;
    } else {
        fprintf(stderr, "Providers other than github is not implemented. Selected provider is %s\n", provider);
        exit(EXIT_FAILURE);
    }
}

int
main(int argc,char *argv[])
{
    if (argc != 2) {
        usage();
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
    strncat(ssh_directory, "/.ssh", 5);
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
    strncat(authorized_keys, "/authorized_keys", 16);

    /* get url of ssh keys */
    char* url = get_url_of_ssh_keys(argv[1], "github");

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

        /* open the file */
        int fd;
        if ((fd = open(authorized_keys, O_WRONLY | O_CREAT | O_APPEND, 0600 )) == -1) {
             fprintf(stderr, "Can't open the %s: %s\n", authorized_keys, strerror(errno));
             exit(EXIT_FAILURE);
        }

        /* write the page body to this file handle */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    
        /* some servers do not like requests that are made without a user-agent field, so we provide one */
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        /* get it! */
        if ((res = curl_easy_perform(curl_handle)) != CURLE_OK) {
            fprintf(stderr, "curl failed: %s\n",
                    curl_easy_strerror(res));
            close(fd);
            exit(EXIT_FAILURE);
        }

	if (strcmp(chunk.memory, "Not Found") == 0) {
		fprintf(stderr, "Github Account Not Found.\n");
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
