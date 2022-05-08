#pragma once

#include <libft/hmap_s.h>

#include <sock_cli.h>

#ifndef HTTP_PROTO
# define HTTP_PROTO       "http"
#endif

#ifndef HTTP_PROTO_SSL
# define HTTP_PROTO_SSL   "https"
#endif

#define HTTP_LENGTH_UNKNOWN -1

typedef struct  http_cli
{
	sock_cli_t	socket;
	t_hmap_s	*headers;
}               http_cli_t;

int http_connect(http_cli_t *client, const char *url);
int	http_destroy(http_cli_t *client);

int http_get(const http_cli_t *client, const char *path);

int	http_download(int dest_fd, const http_cli_t *client, const char *path);