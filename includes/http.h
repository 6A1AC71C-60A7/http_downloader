#pragma once

#include <sock_cli.h>

#ifndef HTTP_PROTO
# define HTTP_PROTO       "http"
#endif

#ifndef HTTP_PROTO_SSL
# define HTTP_PROTO_SSL   "https"
#endif

int http_connect(sock_cli_t *client, const char *url);

int http_get(const sock_cli_t *client, const char *path);

int	http_download(int dest_fd, const sock_cli_t *client, const char *path);