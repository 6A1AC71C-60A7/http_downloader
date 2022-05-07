#pragma once

int	get(int client, const char *path);
int	download(int dest_fd, int client, const char *path);