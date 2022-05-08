#include <openssl/ssl.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sock_cli.h>

static SSL_CTX *ssl_ctx = NULL;

int	resolve_hostname(struct sockaddr *addr, const char *hostname)
{
	int	status;

	status = inet_pton(SOCK_AF, hostname, addr);
	return (status);
}

int	connect_hostname(const char *hostname, const char *service)
{
	int				fd;
	struct addrinfo	hints = (struct addrinfo)
	{
		.ai_family		= SOCK_AF,
		.ai_socktype	= SOCK_STREAM,
		.ai_protocol	= IPPROTO_IP,
	};
	struct addrinfo	*info;
	struct addrinfo	*curr;

	fd = getaddrinfo(hostname, service, &hints, &info);
	if (fd == 0)
	{
		for (curr = info; curr != NULL; curr = curr->ai_next)
		{
			fd = socket(curr->ai_family, curr->ai_socktype,
				curr->ai_protocol);
			if (fd != -1 && connect(fd, curr->ai_addr, curr->ai_addrlen) == 0)
				break;
			else
				close(fd);
		}

		freeaddrinfo(info);

		if (curr == NULL)
		{
			fd = -1;
			if (errno == 0)
				errno = EADDRNOTAVAIL;
		}
	}
	return fd;
}

void	print_hostname(const struct sockaddr *addr)
{
	char	hostname[256];

	inet_ntop(SOCK_AF, addr, hostname, sizeof(*addr));
	write(STDOUT_FILENO, hostname, strlen(hostname));
}

int		client_connect(sock_cli_t *client, const char *hostname,
	const char *service, bool use_ssl)
{
	int	status;

	if (use_ssl)
	{
		if (ssl_ctx == NULL)
		{
			ssl_ctx = SSL_CTX_new(SSLv23_client_method());
			SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2);
		}

		if (client->ssl == NULL)
			client->ssl = SSL_new(ssl_ctx);
		else
			SSL_clear(client->ssl);

		dprintf(2, "init ssl: %p, %p\n", ssl_ctx, client->ssl);
	}
	else
	{
		if (client->ssl != NULL)
		{
			SSL_free(client->ssl);
			client->ssl = NULL;
		}
	}

	if (client->connection > 0)
		close(client->connection);

	client->connection = connect_hostname(hostname, service);

	status = client->connection == -1;

	if (status == 0 && client->ssl != NULL)
	{
		dprintf(2, "Binding ssl to fd %d...\n", client->connection);
		SSL_set_fd(client->ssl, client->connection);

		status = SSL_connect(client->ssl) != 1;
	}

	dprintf(2, "Connected! %d\n", client->connection);

	return (status);
}