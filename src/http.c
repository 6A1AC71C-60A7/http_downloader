#include <openssl/ssl.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <http.h>


static const char	*headers = "Accept: */*\r\n";

static const char	*find_term(const char *message)
{
	while (*message != '\0' && message[0] != '\r' && message[1] != '\n')
		message++;
	
	if (*message == '\0')
		message = NULL;
	else
		message += 2;

	return (message);
}

static size_t		header_size(const char *message)
{
	size_t		size;
	size_t		field_size;
	const char	*next;

	size = 0;

	do
	{
		next = find_term(message);

		if (next != NULL)
		{
			field_size = next - message;
			dprintf(2, "Skipping header %.*s\n", (unsigned)field_size, message);
			size += field_size;
		}
		else
			field_size = 0;

		message = next;
	}
	while (next != NULL && field_size != 0);

	return (size);
}

int					http_connect(sock_cli_t *client, const char *url)
{
	char	hostname[256];
	char	protocol[256];
	char	port[32];
	ssize_t	nconv;
	int		status;
	bool	use_ssl;

	nconv = sscanf(url, "%256[^:]://%256[^:]:%32[^/]", protocol, hostname, port);

	status = nconv < 2;

	use_ssl = strcmp(HTTP_PROTO_SSL, protocol) == 0;

	if (nconv == 2)
		strcpy(port, protocol);

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

int					http_get(const sock_cli_t *client, const char *path)
{
	char	request[256];
	ssize_t	length;
	ssize_t	send_length;
	int		status;

	length = snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\n%s\r\n",
		path, headers);
	status = length == -1;

	if (status == 0)
	{
		dprintf(2, "Sending %s\n", request);
		if (client->ssl == NULL)
		{
			do
			{
				send_length = send(client->connection, request, length, 0);
				length -= send_length;
			}
			while (length > 0 && send_length > 0);
		}
		else
		{
			dprintf(2, "Using ssl...\n");
			do
			{
				send_length = SSL_write(client->ssl, request, length);
				length -= send_length;
			}
			while (length > 0 && send_length > 0);

			if (send_length == -1)
				SSL_get_error(client->ssl, send_length);
			dprintf(2, "Sent length %zd\n", send_length);
		}

		status = send_length == -1;
	}

	return (status);
}

int					http_download(int dest_fd, const sock_cli_t *client,
	const char *path)
{
	char		response[1024];
	const char	*it;
	ssize_t		length;
	size_t		body_offset;
	int			status;

	status = http_get(client, path);
	if (status == 0)
	{
		if (client->ssl == NULL)
			length = recv(client->connection, response, sizeof(response), 0);
		else
			length = SSL_read(client->ssl, response, sizeof(response));

		dprintf(2, "Downloading %zd\n", length);
		status = length == -1;

		if (status == 0)
		{
			body_offset = header_size(response);
			it = response + body_offset;
			length -= body_offset;

			if (client->ssl == NULL)
			{
				do
				{
					write(dest_fd, it, length);
					length = recv(client->connection, response,
						sizeof(response), 0);
					it = response;
				}
				while (length > 0);
			}
			else
			{
				do
				{
					write(dest_fd, it, length);
					length = SSL_write(client->ssl, response, sizeof(response));
					it = response;
				}
				while (length > 0);
			}

			status = length != 0;
		}
	}
	return (status);
}