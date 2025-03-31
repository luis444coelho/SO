#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#define PIPE_NAME "doc_pipe"
#define RESPONSE_PIPE "response_pipe"

void send_command(const char *command) {
    int pipe_fd = open(PIPE_NAME, O_WRONLY);
    if (pipe_fd == -1) {
        perror("Erro ao abrir o pipe");
        exit(EXIT_FAILURE);
    }
    write(pipe_fd, command, strlen(command) + 1);
    close(pipe_fd);
}

void read_response() {
    int resp_fd = open(RESPONSE_PIPE, O_RDONLY);
    if (resp_fd == -1) {
        perror("Erro ao abrir o pipe de resposta");
        return;
    }
    
    char buffer[512];
    ssize_t bytes_read = read(resp_fd, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Resposta do servidor: %s\n", buffer);
    }
    close(resp_fd);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: dclient <script> <dataset>\n");
        return 1;
    }

    char command[512];
    snprintf(command, sizeof(command), "%s %s", argv[1], argv[2]);

    send_command(command);
    read_response();

    return 0;
}


