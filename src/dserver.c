#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define PIPE_NAME "doc_pipe"
#define RESPONSE_PIPE "response_pipe"


int main() {
    mkfifo(PIPE_NAME, 0666);
    mkfifo(RESPONSE_PIPE, 0666);

    inicializar_documentos(); //TODO
    carregar_documentos(); //TODO

    int pipe_fd;
    char buffer[512];

    while (1) {
        pipe_fd = open(PIPE_NAME, O_RDONLY);
        if (pipe_fd == -1) {
            perror("Erro ao abrir o pipe");
            continue;
        }

        ssize_t bytes_read = read(pipe_fd, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            processar_comando(buffer);//TODO
        }

        close(pipe_fd);
    }

    liberar_documentos();//TODO
    return 0;
}





