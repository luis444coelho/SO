#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>


#define PIPE_NAME "doc_pipe"
#define RESPONSE_PIPE "response_pipe"
#define METADATA_FILE "metadata.txt"

void armazenar_metadados(const char *output_file, const char *metadata_file) {
    int input_fd = open(output_file, O_RDONLY);
    if (input_fd == -1) {
        perror("Erro ao abrir arquivo de saída do script");
        return;
    }

    int output_fd = open(metadata_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (output_fd == -1) {
        perror("Erro ao abrir metadata.txt");
        close(input_fd);
        return;
    }

    char buffer[512];
    ssize_t bytes_read;
    
    while ((bytes_read = read(input_fd, buffer, sizeof(buffer))) > 0) {
        write(output_fd, buffer, bytes_read);
    }

    close(input_fd);
    close(output_fd);
}

void processar_comando(const char *comando) {
    char script[256], dataset[256];

    if (sscanf(comando, "%s %s", script, dataset) == 2) {
        printf("Executando script: %s com dataset: %s\n", script, dataset);

        char full_command[512];
        snprintf(full_command, sizeof(full_command), "bash %s %s > temp_output.txt", script, dataset);

        int status = system(full_command);
        if (status == -1) {
            perror("Erro ao executar o script");
            return;
        }

        printf("Script executado. Salvando metadados...\n");
        armazenar_metadados("temp_output.txt", METADATA_FILE);
    }
}

void send_response(const char *message) {
    int resp_fd = open(RESPONSE_PIPE, O_WRONLY);
    if (resp_fd == -1) {
        perror("Erro ao abrir pipe de resposta");
        return;
    }
    write(resp_fd, message, strlen(message) + 1);
    close(resp_fd);
}

int main() {
    mkfifo(PIPE_NAME, 0666);
    mkfifo(RESPONSE_PIPE, 0666);

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
            processar_comando(buffer);
            send_response("Metadados armazenados com sucesso!\n");
        }

        close(pipe_fd);
    }

    return 0;
}







