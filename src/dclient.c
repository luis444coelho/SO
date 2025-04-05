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

int processa_comando(int argc, char *argv[], char *command, size_t size) {
    if (argc == 3) {
        // Executar script com dataset
        snprintf(command, size, "%s %s", argv[1], argv[2]);
        return 0;
    } else if (argc == 6 && strcmp(argv[1], "-a") == 0) {
        // Adicionar manualmente com ADD|title|authors|year|path
        snprintf(command, size, "ADD|%s|%s|%s|%s", argv[2], argv[3], argv[4], argv[5]);
        return 0;
    } else {
        // Comando inválido
        fprintf(stderr, "Uso: dclient <script> <dataset>\n       dclient -a \"title\" \"authors\" \"year\" \"path\"\n");
        return 1;
    }
}

int main(int argc, char *argv[]) {
    char command[512];
    if (processa_comando(argc, argv, command, sizeof(command)) != 0) {
        return 1;
    }

    send_command(command);
    read_response();

    return 0;
}
