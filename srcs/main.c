#include <openssl/evp.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <http.h>

#ifndef DL_AF
# define DL_AF AF_INET
#endif

typedef struct	client
{
	SSL	*ssl;
	int	socket;
}				client_t;

int	resolve_hostname(struct sockaddr *addr, const char *hostname)
{
	int	status;

	status = inet_pton(DL_AF, hostname, addr);
	return (status);
}

int	connect_hostname(const char *hostname, const char *port)
{
	int				fd;
	struct addrinfo	hints = (struct addrinfo)
	{
		.ai_family = DL_AF,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_IP,
	};
	struct addrinfo	*info;
	struct addrinfo	*curr;

	fd = getaddrinfo(hostname, port, &hints, &info);
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

	inet_ntop(DL_AF, addr, hostname, sizeof(*addr));
	write(STDOUT_FILENO, hostname, strlen(hostname));
}

void	init_ssl()
{
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
}

void	cleanup_ssl()
{
	EVP_cleanup();
}

int	main(int ac, const char **av)
{
	(void)	ac;
	int	status;
	int	client;
	int	dest_fd;
	
	client = connect_hostname("i.imgur.com", "80");
	status = client == -1;

	if (status == 0)
	{
		dest_fd = open(av[1] ? av[1] : "goat.png",
			O_WRONLY | O_CREAT | O_TRUNC, 00600);

		status = dest_fd == -1;
		if (status == 0)
		{
			status = download(dest_fd, client, "/wPPYWyV.png");

			close(dest_fd);
		}
	}

	if (status != 0)
		perror(av[0]);
	return (status);
}
