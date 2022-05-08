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


void	ssl_init()
{
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
}

void	ssl_cleanup()
{
	EVP_cleanup();
}

int		resolve_hostname(struct sockaddr *addr, const char *hostname)
{
	int	status;

	status = inet_pton(SOCK_AF, hostname, addr);
	return (status);
}

int		connect_hostname(const char *hostname, const char *service)
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
	}
	else
	{
		if (client->ssl != NULL)
		{
			SSL_free(client->ssl);
			client->ssl = NULL;
		}
	}

	if (client->conn > 0)
		close(client->conn);

	client->conn = connect_hostname(hostname, service);

	status = client->conn == -1;

	if (status == 0 && client->ssl != NULL)
	{
		SSL_set_fd(client->ssl, client->conn);

		status = SSL_connect(client->ssl) != 1;
	}

	return (status);
}

int		client_disconnect(sock_cli_t *client)
{
	if (client->conn != 0)
	{
		if (client->ssl != NULL)
		{
			SSL_shutdown(client->ssl);
			SSL_free(client->ssl);
			client->ssl = NULL;
		}
		client->conn = close(client->conn);
	}
	return (client->conn);
}

int		client_destroy(sock_cli_t *client)
{
	int	status;

	status = client_disconnect(client);

	if (client->ssl)
	{
		SSL_CTX_free(ssl_ctx);
		ssl_ctx = NULL;
	}
	return (status);
}

ssize_t	client_send(const sock_cli_t *client, const char *data, size_t size)
{
	ssize_t	length;

	if (client->ssl == NULL)
		length = send(client->conn, data, size, 0);
	else
		length = SSL_write(client->ssl, data, size);

	return (length);
}

ssize_t	client_recv(const sock_cli_t *client, char *dest, size_t size)
{
	ssize_t	length;

	if (client->ssl == NULL)
		length = recv(client->conn, dest, size, 0);
	else
		length = SSL_read(client->ssl, dest, size);

	return (length);
}