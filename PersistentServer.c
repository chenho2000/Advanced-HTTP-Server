#define _XOPEN_SOURCE
#define _GNU_SOURCE
#define __USE_XOPEN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>	/* inet_ntoa */
#include <netdb.h>		/* gethostname */
#include <netinet/in.h> /* struct sockaddr_in */
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "Server.h"

int main(int argc, char *argv[])
{
	// We want 2 arguments (port number, path of root directory)
	if (argc != 3)
	{
		fprintf(stderr, "Input Error");
		exit(1);
	}
	// Get the port number
	char *ptr;
	int SERVER_PORT = strtol(argv[1], &ptr, 10);

	// Check if the port number is valid
	if (SERVER_PORT > 65535 || SERVER_PORT == 0)
	{
		fprintf(stderr, "Invalid port number");
		exit(1);
	}

	// Get the path of root directory
	char path[BUF_SIZE];

	// Check its length is valid
	if (strlen(argv[0]) >= BUF_SIZE - 1)
	{
		fprintf(stderr, "Input Error");
	}
	else
	{
		strncpy(path, argv[2], strlen(argv[2]) + 1);
	}

	// We store the original path length for reset propers
	int path_length = strlen(path);

	// Start the server (code is similar to CSC209 A4)
	struct sockaddr_in addr;

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("server socket");
		exit(1);
	}

	memset(&addr, '\0', sizeof(addr));
	addr.sin_family = AF_INET;

	// Port will be the port number in argument
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		perror("server: bind");
		exit(1);
	}

	if (listen(sockfd, MAX_BACKLOG) < 0)
	{
		perror("server: listen");
	}

	// Before accept connection, do init

	// Current socket we are talking to
	int curr_socket;

	// Socket buffer
	char buffer[BUF_SIZE];

	// Will return 404 if met any errors
	char *not_found_response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<html><head><title>404 Not Found</title></head><body><center><h1>404 Not Found</h1></center><hr></body></html>\r\n\r\n";

	while (1)
	{

		struct sockaddr_in curr;
		socklen_t size;

		// accept connection
		curr_socket = accept(sockfd, (struct sockaddr *)&curr, &size);
		if (curr_socket < 0)
		{
			exit(1);
		}
		// To make the server concurrency, fork once accept the connection, so server can serve multiple clients
		// Parent will accept the connections, childs will handle the requests
		pid_t child = fork();

		if (child == 0)
		{
			// Do not need parent process anymore
			close(sockfd);

			time_t start_time, end_time;
			time(&start_time);

			while (1)
			{
				Response res;
				// reset buffer at the beginning
				bzero(buffer, sizeof(buffer));

				// Recieve message from clients
				recv(curr_socket, buffer, BUF_SIZE, MSG_DONTWAIT);

				// printf("FROM BROWSER! %s \n\n", buffer);

				if (strlen(buffer) == 0)
				{
					if (time(&end_time) - start_time >= 0.5)
					{
						break;
					}
				}

				char *rest1;
				// Seperate message with \r\n
				char *get_req = strtok_r(buffer, "\r\n", &rest1);

				// if we didn't get any request we will wait for client
				if (get_req == NULL)
				{
					continue;
				}

				// check if it's a GET request (GET is the only request we can handle)
				char *token;
				token = strtok(get_req, " ");
				if (strcmp(token, "GET") != 0)
				{
					send(curr_socket, not_found_response, strlen(not_found_response), 0);
					continue;
				}

				// get the file name and add it to the path provided earlier
				token = strtok(NULL, " ");
				strcat(path, token);

				/*signal here represents the conditional header
				1. If-Match
				2. If-None-Match
				3. If-Modified-Since
				4. If-Unmodified-Since
				5. If-Range with date input
				6. If-Range with etag input*/
				char *rest, *leftover, *head, *date, *user_etag, *container, *bytes, *range;
				char ranges[MAX_ARG][BUF_SIZE / 3]; // at max 10 ranges can be defined
				int signal = 0;
				int range_sig = 0; // 0 for no range header specified, 1 for range header detected
				int counter = 0;   // keep record of how many ranges are there

				while (1)
				{
					leftover = strtok_r(NULL, "\r\n", &rest1);
					if (leftover == NULL)
					{
						// printf("NO IF HEADER\r\n");
						break;
					}
					head = strtok_r(leftover, " ", &rest);
					// printf("---------%s---------\r\n", head);
					if (strcmp(head, "If-Modified-Since:") == 0)
					{ // encounter If-Modified-Since: Wed, 21 Oct 2023 07:28:00 GMT
						date = strtok_r(NULL, "\r\n", &rest);
						// printf("YES IF HEADER: %s\r\n", date);
						signal = 3;
						break;
					}

					if (strcmp(head, "If-Unmodified-Since:") == 0)
					{ // encounter If-Unmodified-Since: Wed, 21 Oct 2023 07:28:00 GMT
						date = strtok_r(NULL, "\r\n", &rest);
						signal = 4;
						break;
					}

					if (strcmp(head, "If-Match:") == 0)
					{
						user_etag = strtok_r(NULL, "\r\n", &rest);
						signal = 1;
						break;
					}

					if (strcmp(head, "If-None-Match:") == 0)
					{
						user_etag = strtok_r(NULL, "\r\n", &rest);
						signal = 2;
						break;
					}

					if (strcmp(head, "If-Range:") == 0)
					{
						container = strtok_r(NULL, "\r\n", &rest);
						if (strlen(container) != 0 && (container[0] == '"' || container[2] == '"'))
						{ // etag input detected
							user_etag = container;
							signal = 6;
						}
						else
						{ // date input detected
							date = container;
							signal = 5;
						}
						continue;
					}

					if (strcmp(head, "Range:") == 0)
					{
						bytes = strtok_r(NULL, "=", &rest);
						while (1)
						{
							range = strtok_r(NULL, ", ", &rest);
							if (range == NULL)
							{
								break;
							}
							strcat(ranges[counter], range);
							counter++;
						}
						range_sig = 1;
						continue;
					}
				}

				// have if-range header but no range header
				if ((signal == 6 || signal == 5) && range_sig == 0)
				{
					res = get_res(path, 416);
					send(curr_socket, res.res, strlen(res.res), 0);
					free(res.res);
					continue;
				}

				time_t cache_time, modified_time;
				char modified_time_str[50] = {0};
				struct stat sb;
				if (signal == 3 || signal == 4 || signal == 5)
				{ // date is set
					// get conditional header last-modified-since time
					struct tm tm;
					strptime(date, "%a, %d %b %Y %X %Z", &tm);
					cache_time = timegm(&tm);
					printf("Cache_time: %ld\r\n", cache_time);
					if (cache_time == -1)
					{
						res = get_res(path, 400);
						send(curr_socket, res.res, strlen(res.res), 0);
						free(res.res);
						continue;
					}
				}

				// get file last modified time
				if (stat(path, &sb) == -1)
				{
					send(curr_socket, not_found_response, strlen(not_found_response), 0);
					break;
				}
				modified_time = sb.st_mtime;
				struct tm *last_mod = gmtime(&sb.st_mtime);
				strftime(modified_time_str, 50, "%a, %d %b %G %R GMT\r\n", last_mod);
				printf("Signal: %d\r\n", signal);
				printf("Modified_time: %ld\r\n", modified_time);
				printf("Modified_time_str: %s", modified_time_str);
				printf("Path: %s\r\n\r\n\r\n", path);

				/* Our etag format:
				For strong validation: "hexdump({file path} {last modified time (in GMT)})"
				For weak validation: W/"hexdump({file path})"
				Besides those above, etag also accept an asterisk that means any resource will match
				*/
				char etag_str[200] = {0};
				char etag[500] = {0};
				if ((signal == 1 || signal == 2 || signal == 6) // user_etag is set
					&& strlen(user_etag) >= 1 && user_etag[0] != '*')
				{ // etag == "*" means if-match header was bypassed
					printf("User etag: %s\r\n", user_etag);

					if (strlen(user_etag) >= 2 && user_etag[0] == 'W' && user_etag[1] == '/')
					{ // weak validation
						strcat(etag_str, path);
						strcat(etag, "W/\"");
						string2hexString(etag_str, &etag[3]);
						strcat(etag, "\"");
					}
					else
					{ // strong validation
						strcat(etag_str, path);
						strcat(etag_str, " ");
						strcat(etag_str, modified_time_str);
						strcat(etag, "\"");
						string2hexString(etag_str, &etag[1]);
						strcat(etag, "\"");
					}
					printf("etag: %s\r\n", etag);
					printf("etag_str: %s\r\n", etag_str);
					if (signal == 1 && strcmp(user_etag, etag) != 0)
					{ // couldnt match, 412 response
						// send(curr_socket, precondition_failed_response, strlen(precondition_failed_response), 0);
						// break;
						res = get_res(path, 412);
						send(curr_socket, res.res, strlen(res.res), 0);
						free(res.res);
						continue;
					}

					if (signal == 2 && strcmp(user_etag, etag) == 0)
					{ // match, 304 response
						// send(curr_socket, not_modified_response, strlen(not_modified_response), 0);
						// break;
						res = get_res(path, 304);
						send(curr_socket, res.res, strlen(res.res), 0);
						free(res.res);
						continue;
					}
				}

				/* Here We deal with range request header*/
				int err_sig = 0;
				if ((range_sig == 1 && signal == 0) || (signal == 5 && modified_time <= cache_time) || (signal == 6 && strcmp(user_etag, etag) == 0))
				{ // ranges are set
					char *start, *end;
					int starting, ending;
					FILE *range_file = fopen(path, "rb");
					int ct = 0;
					while (ct < counter)
					{
						// printf("Index: %d\r\nLength: %d\r\nCounter: %d\r\n", len-1-counter, len, counter);
						start = strtok(ranges[ct], "-");
						end = strtok(NULL, "-");
						starting = -1;
						ending = -1;
						if (start == NULL && end == NULL)
						{ // format error
							err_sig = 1;
							break;
						}

						if (start != NULL)
						{
							starting = strtol(start, NULL, 10);
						}
						if (end != NULL)
						{
							ending = strtol(end, NULL, 10);
						}
						if ((starting == -1 && ending == -1) || ending < starting)
						{
							err_sig = 1;
							break;
						}

						if (starting != -1 && ending != -1)
						{
							bzero(ranges[ct], sizeof(ranges[ct]));
							fseek(range_file, starting, SEEK_SET);
							fread(ranges[ct], sizeof(char), ending - starting + 1, range_file);
							printf("Partial File:%s\r\n", ranges[ct]);
						}
						ct++;
					}
					fclose(range_file);
				}

				// range not in the correct format: bytes={range}, {range}}
				if (range_sig == 1 && (strcmp(bytes, "bytes") != 0 || counter == 0))
				{
					res = get_res(path, 416);
					send(curr_socket, res.res, strlen(res.res), 0);
					free(res.res);
					continue;
				}
				if (err_sig == 1)
				{
					res = get_res(path, 416);
					send(curr_socket, res.res, strlen(res.res), 0);
					free(res.res);
					printf("Format error for range request\n");
					continue;
				}

				/* If-Modified-since was not modified*/
				if (signal == 3 && modified_time < cache_time)
				{
					res = get_res(path, 304);
					send(curr_socket, res.res, strlen(res.res), 0);
					free(res.res);
					continue;
				}

				/* If-Unmodified-Since was modified*/
				if (signal == 4 && modified_time > cache_time)
				{
					res = get_res(path, 412);
					send(curr_socket, res.res, strlen(res.res), 0);
					free(res.res);
					continue;
				}

				/* Until this point, only those response 206 200 are left,
				they are special cuz they need a body */
				if ((signal == 5 && modified_time <= cache_time) || (signal == 6 && strcmp(user_etag, etag) == 0) || (signal == 0 && range_sig == 1))
				{
					res = get_res(path, 206);
				}
				else
				{
					res = get_res(path, 200);
				}

				// if error (no such file, broken file)
				if (res.go == 2)
				{
					free(res.res);
					send(curr_socket, not_found_response, strlen(not_found_response), 0);
					continue;
				}
				// read file
				FILE *fp = fopen(path, "rb");
				if (fp == NULL)
				{
					send(curr_socket, not_found_response, strlen(not_found_response), 0);
					continue;
				}
				fseek(fp, 0, SEEK_SET);
				char send_buffer[BUF_SIZE] = {0};

				// send reponse header to client
				send(curr_socket, res.res, strlen(res.res), 0);

				if ((signal == 5 && modified_time <= cache_time) || (signal == 6 && strcmp(user_etag, etag) == 0) || (signal == 0 && range_sig == 1))
				{
					int ct1 = 0;
					while (ct1 < counter)
					{
						send(curr_socket, ranges[ct1], strlen(ranges[ct1]), 0);
						bzero(ranges[ct1], sizeof(ranges[ct1]));
						ct1++;
					}
				}
				else
				{
					// send a complete file to client
					size_t readed = 0;
					while ((readed = fread(send_buffer, 1, sizeof(send_buffer) - 1, fp)) > 0)
					{
						write(curr_socket, send_buffer, readed);
						bzero(send_buffer, sizeof(send_buffer));
					}
				}

				// clean up
				path[path_length + 1] = '\0';
				free(res.res);
				bzero(buffer, sizeof(buffer));
			}
		}
		close(curr_socket);
	}

	return 0;
}