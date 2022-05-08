#pragma once

#include <sock_cli.h>

int	get(const sock_cli_t *client, const char *path);
int	download(int dest_fd, const sock_cli_t *client, const char *path);