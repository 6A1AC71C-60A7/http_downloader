#include "libft/hmap/hmap_s.h"
#include "libft/hmap/hmap_s_pair.h"
#include "sock_cli.h"
#include <openssl/ssl.h>
#include <stddef.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <http.h>


#define HTTP_HEADER_COUNT 16

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

int					http_connect(http_cli_t *client, const char *url)
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
		//dprintf(2, "Connecting to %s://%s:%s...\n", protocol, hostname, port);
		status = client_connect(&client->socket, hostname, port, use_ssl);
		
		if (client->headers == NULL)
		{
			client->headers = hmap_s_new(16);
			status = client->headers == NULL;
			if (status == 0)
			{
				hmap_s_set(client->headers, "Host", strdup(hostname));
			}
			else
				client_disconnect(&client->socket);
		}
	}
	else
		dprintf(2, "Invalid url: %s, found %zi fields!\n", url, nconv);
	return (status);
}

int		http_header_add(t_hmap_s *headers, const char *header, size_t length)
{
	char	key[256];
	char	value[256];
	int		nconv;
	int		status;

	nconv = sscanf(header, "%256[^:]: %256[^\r]\r\n", key, value);
	status = nconv != 2;
	if (status == 0)
		status = hmap_s_set(headers, key, strndup(value, length)) == NULL;
	return (status);
}

const char	*http_header_get(t_hmap_s *headers, const char *key,
	const char *default_value)
{
	const char			*value;
	t_hmap_s_pair		*header;
	
	header = hmap_s_get(headers, key);
	if (header != NULL)
		value = header->value;
	else
		value = default_value;
	return (value);
}

int					http_get(const http_cli_t *client, const char *path)
{
	char	request[256];
	ssize_t	length;
	ssize_t	send_length;
	int		status;

	length = snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s\r\n%s\r\n",
		path, http_header_get(client->headers, "Host", "localhost"), headers);
	status = length == -1;

	if (status == 0)
	{
		do
		{
			send_length = client_send(&client->socket, request, length);
			length -= send_length;
		}
		while (length > 0 && send_length > 0);

		status = send_length == -1;
	}

	return (status);
}

size_t	http_headers(t_hmap_s *headers, const char *message, size_t size)
{
	const char	*next;
	size_t		body_offset;
	size_t		header_size;

	body_offset = 0;

	do
	{
		next = find_term(message);

		if (next != NULL)
		{
			header_size = next - message - 2;
			if (header_size != 0)
				http_header_add(headers, message, header_size);
			body_offset += header_size + 2;
		}
		else
			header_size = 0;

		message = next;
	}
	while (next != NULL && header_size != 0 && body_offset < size);

	return (body_offset);
}

ssize_t				http_content_length(t_hmap_s *headers)
{
	ssize_t			length;
	const char		*content_length;
	char			*end;

	content_length = http_header_get(headers, "Content-Length", "-1");

	length = (ssize_t)strtoll(content_length, &end, 10);

	if (length == 0)
		length = HTTP_LENGTH_UNKNOWN;

	return (length);
}

int					http_download(int dest_fd, const http_cli_t *client,
	const char *path)
{
	char		response[1024];
	const char	*it;
	t_hmap_s	*headers;
	ssize_t		content_length;
	size_t		body_offset;
	ssize_t		length;
	int			status;

	status = http_get(client, path);
	if (status == 0)
	{
		length = client_recv(&client->socket, response, sizeof(response));

		status = length == -1;

		if (status == 0)
		{
			headers = hmap_s_new(HTTP_HEADER_COUNT);

			status = headers == NULL;
			if (status == 0)
			{
				body_offset = http_headers(headers, response, length);

				it = response + body_offset;
				length -= body_offset;

				content_length = http_content_length(headers);

				do
				{
					write(dest_fd, it, length);

					length = client_recv(&client->socket, response, sizeof(response));
					content_length -= length;
				
					it = response;
				}
				while (length > 0 && (size_t)content_length > 0);

				status = length == -1;
				hmap_s_clr(&headers, free);
			}
		}
	}
	return (status);
}

int	http_destroy(http_cli_t *client)
{
	int status;
	
	status = client_destroy(&client->socket);
	hmap_s_clr(&client->headers, free);

	return (status);
}