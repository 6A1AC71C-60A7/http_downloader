#include <openssl/evp.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <sock_cli.h>
#include <http.h>

void	init_ssl()
{
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
}

void	cleanup_ssl()
{
	EVP_cleanup();
}

int	http_connect(sock_cli_t *client, const char *url)
{
	char	hostname[256];
	char	protocol[256];
	char	port[32];
	ssize_t	nconv;
	int		status;
	bool	use_ssl;

	nconv = sscanf(url, "%256[^:]://%256[^:]:%32[^/]", protocol, hostname, port);

	status = nconv < 2;

	use_ssl = strcmp("https", protocol) == 0;

	if (nconv == 2)
	{
		if (use_ssl)
			strcpy(port, "443");
		else
			strcpy(port, "80");
	}

	if (status == 0)
	{
		dprintf(2, "Connecting to %s://%s:%s...\n", protocol, hostname, port);
		status = client_connect(client, hostname, port, use_ssl);
	}
	else
	{
		dprintf(2, "Invalid url: %s, found %zi fields!\n", url, nconv);
	}
	return (status);
}

int	main(int ac, const char **av)
{
	(void)	ac;
	sock_cli_t	client;
	int			dest_fd;
	int			status;

	init_ssl();

	bzero(&client, sizeof(client));
	status = http_connect(&client, "https://i.imgur.com");
	if (status == 0)
	{
		dest_fd = open(av[1] ? av[1] : "goat.png",
			O_WRONLY | O_CREAT | O_TRUNC, 00600);

		status = dest_fd == -1;
		if (status == 0)
		{
			status = download(dest_fd, &client, "/wPPYWyV.png");

			close(dest_fd);
		}
	}

	if (status != 0)
		perror(av[0]);

	cleanup_ssl();
	return (status);
}
