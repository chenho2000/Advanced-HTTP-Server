#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>  /* inet_ntoa */
#include <netdb.h>      /* gethostname */
#include <netinet/in.h> /* struct sockaddr_in */
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "Server.h"

// function to convert ascii char[] to hex-string (char[])
// Reference: https://www.includehelp.com/c/convert-ascii-string-to-hexadecimal-string-in-c.aspx
void string2hexString(char *input, char *output)
{
    int loop;
    int i;

    i = 0;
    loop = 0;

    while (input[loop] != '\0')
    {
        sprintf((char *)(output + i), "%02X", input[loop]);
        loop += 1;
        i += 2;
    }
    // insert NULL at the end of the output string
    output[i++] = '\0';
}


/* Supported response codes are 200 304 206 400 412 416*/
Response get_res(char *path, int code)
{

    Response r;
    char *res = malloc(sizeof(char) * BUF_SIZE);
    r.res = res;
    r.go = 0;

    if (code == 200) {strcpy(res, "HTTP/1.1 200 OK\r\n");}
    else if (code == 206) {strcpy(res, "HTTP/1.1 206 Partial Content\r\n");}
    else if (code == 304) {strcpy(res, "HTTP/1.1 304 Not Modified\r\n");}
    else if (code == 400) {strcpy(res, "HTTP/1.1 400 Bad Request\r\n");}
    else if (code == 412) {strcpy(res, "HTTP/1.1 412 Precondition Failed\r\n");}
    else if (code == 416) {strcpy(res, "HTTP/1.1 416 Range Not Satisfiable\r\n");}
    else {strcpy(res, "Response code error");}
    
    struct stat sb;
    if (stat(path, &sb) == -1)
    {
        r.go = 2;
        return r;
    }
    basic_header(r, path);
    strcat(r.res, "\r\n");
    return r;
}



void basic_header(Response r, char *path)
{
    struct stat sb;
    if (stat(path, &sb) == -1)
    {
        r.go = 2;
        return;
    }

    char date[100] = "Date: ";
    time_t currtime;
    time(&currtime);
    struct tm *time = gmtime(&currtime);
    char curr_t[50] = {0};
    strftime(curr_t, 50, "%a, %d %b %G %R GMT\r\n", time);
    strcat(date, curr_t);
    strcat(r.res, date);

    char last_modify[50] = "Last-Modified: ";
    char modify_t[50] = {0};
    time = gmtime(&sb.st_mtime);
    strftime(modify_t, 50, "%a, %d %b %G %R GMT\r\n", time);
    strcat(last_modify, modify_t);
    strcat(r.res, last_modify);

    char accept_range[] = "Accept-Ranges: bytes \r\n";
    strcat(r.res, accept_range);

    char content_type[50] = "Content-Type: ";
    if (strstr(path, ".html") != NULL)
    {
        strcat(content_type, "text/html \r\n");
    }
    else if (strstr(path, ".css") != NULL)
    {
        strcat(content_type, "text/css \r\n");
    }
    else if (strstr(path, ".js") != NULL)
    {
        strcat(content_type, "application/javascript \r\n");
    }
    else if (strstr(path, ".txt") != NULL)
    {
        strcat(content_type, "text/txt \r\n");
    }
    else if (strstr(path, ".jpg") != NULL)
    {
        strcat(content_type, "image/jpeg \r\n");
    }
    else if (strstr(path, ".jpeg") != NULL)
    {
        strcat(content_type, "image/jpeg \r\n");
    }
    else if (strstr(path, ".png") != NULL)
    {
        strcat(content_type, "image/png \r\n");
    }
    else
    {
        r.go = 2;
        return;
    }

    strcat(r.res, content_type);
    return;
}