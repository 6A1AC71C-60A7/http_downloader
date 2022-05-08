#pragma once

#include <stdbool.h>
#include <openssl/ssl.h>

#ifndef SOCK_AF
# define SOCK_AF AF_INET
#endif

typedef struct	sock_cli
{
	SSL			*ssl;
	int			connection;
}				sock_cli_t;

void	ssl_init();
void	ssl_cleanup();

int client_connect(sock_cli_t *client, const char *hostname,
	const char *service, bool use_ssl);
int client_connect_ssl();
int client_disconnect();
int client_destroy();