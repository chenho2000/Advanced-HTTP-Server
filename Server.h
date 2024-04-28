#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef MAX_BACKLOG
#define MAX_BACKLOG 1000
#endif

#ifndef MAX_LEN
#define MAX_LEN 65535
#endif

#ifndef BUF_SIZE
#define BUF_SIZE MAX_LEN + 1
#endif


#ifndef MAX_CHILD
#define MAX_CHILD 100
#endif

#ifndef MAX_ARG
#define MAX_ARG 3
#endif

struct response
{
    int go;
    char *res;
};

typedef struct response Response;

void string2hexString(char* input, char* output);

Response get_res(char *path, int code);

void basic_header(Response r, char *path);
