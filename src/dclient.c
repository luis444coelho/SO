#include "../include/utils.h"
#include "../include/executar.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


void send_comando(Comando *cmd) {
    snprintf(cmd->response_pipe, sizeof(cmd->response_pipe), "/tmp/resp_pipe_%d", getpid());

    // Criar pipe de resposta exclusivo para este cliente
    mkfifo(cmd->response_pipe, 0666);

    int fd = open(PIPE_NAME, O_WRONLY);
    if (fd == -1) {
        perror("Erro ao abrir o pipe");
        exit(EXIT_FAILURE);
    }
    write(fd, cmd, sizeof(Comando));
    close(fd);
}

void read_response(char *pipe_name) {
    int fd = open(pipe_name, O_RDONLY);
    if (fd == -1) {
        perror("Erro ao abrir pipe de resposta");
        exit(EXIT_FAILURE);
    }
    char buffer[512];
    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Resposta: %s\n", buffer);
    }
    close(fd);
    unlink(pipe_name);
}

int main(int argc, char *argv[]) {
    Comando cmd = parse_comando(argc, argv);

    if (cmd.tipo == CMD_INVALID) {
        fprintf(stderr, "Comando inválido.\n");
        return 1;
    }

    send_comando(&cmd);
    read_response(cmd.response_pipe);
    return 0;
}
