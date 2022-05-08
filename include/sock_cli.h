#pragma once

#include <stdbool.h>
#include <openssl/ssl.h>

#ifndef SOCK_AF
# define SOCK_AF AF_INET
#endif

typedef struct	sock_cli
{
	SSL			*ssl;
	int			conn;
}				sock_cli_t;

void	ssl_init();
void	ssl_cleanup();

int		client_connect(sock_cli_t *client, const char *hostname,
	const char *service, bool use_ssl);

ssize_t	client_send(const sock_cli_t *client, const char *data, size_t size);
ssize_t	client_recv(const sock_cli_t *client, char *dest, size_t size);

int		client_disconnect(sock_cli_t *client);

int		client_destroy(sock_cli_t *client);