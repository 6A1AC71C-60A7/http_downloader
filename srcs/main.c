#include <unistd.h>
#include <fcntl.h>
#include <string.h>


#include <downloader.h>
#include <sock_cli.h>
#include <http.h>

int	main(int ac, const char **av)
{
	(void)	ac;
	sock_cli_t	client;
	int			dest_fd;
	int			status;

	ssl_init();

	bzero(&client, sizeof(client));
	status = http_connect(&client, DL_BASEURL);
	if (status == 0)
	{
		dest_fd = open(av[1] ? av[1] : DL_FILEPATH, DL_FILEOPTS, DL_FILEMODE);

		status = dest_fd == -1;
		if (status == 0)
		{
			status = http_download(dest_fd, &client, DL_URLPATH);

			close(dest_fd);
		}
	}

	if (status != 0)
		perror(av[0]);

	ssl_cleanup();
	return (status);
}
