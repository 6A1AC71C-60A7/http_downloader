#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
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
			size += field_size;
		}
		else
			field_size = 0;

		message = next;
	}
	while (next != NULL && field_size != 0);

	return (size);
}

int					get(int client, const char *path)
{
	char	request[256];
	ssize_t	length;
	ssize_t	send_length;
	int		status;

	length = snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\n%s\r\n", path, headers);
	status = length == -1;
	if (status == 0)
	{
		do
		{
			send_length = send(client, request, length, 0);
			length -= send_length;
		}
		while (length > 0 && send_length > 0);

		status = send_length == -1;
	}

	return (status);
}

int					download(int dest_fd, int client, const char *path)
{
	char		response[1024];
	const char	*it;
	ssize_t		length;
	size_t		body_offset;
	int			status;

	status = get(client, path);
	if (status == 0)
	{
		length = recv(client, response, sizeof(response), 0);
		body_offset = header_size(response);

		it = response + body_offset;
		length -= body_offset;

		do
		{
			write(dest_fd, it, length);
			length = recv(client, response, sizeof(response), 0);
			it = response;
		}
		while (length > 0);

		status = length != 0;
	}
	return (status);
}