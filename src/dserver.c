#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define PIPE_NAME "doc_pipe"
#define RESPONSE_PIPE "response_pipe"
#define METADATA_FILE "metadata.txt"

int proximo_id = 1;

typedef struct {
    int id;
    char title[200];
    char authors[200];
    int year;
    char path[64];
} Documentos;

void escrever_metadados_struct(Documentos *doc) {
    int output_fd = open(METADATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (output_fd == -1) {
        perror("Erro ao abrir metadata_file");
        return;
    }

    char saida[1024];
    int len = snprintf(saida, sizeof(saida), "%d,%s,%s,%d,%s\n",
                       doc->id, doc->title, doc->authors, doc->year, doc->path);
    write(output_fd, saida, len);
    close(output_fd);
}

void armazenar_metadados(int input_fd) {
    char buffer[512];
    char linha[512];
    ssize_t bytes_read;
    int idx = 0;

    Documentos doc = {0};

    while ((bytes_read = read(input_fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytes_read; ++i) {
            char c = buffer[i];
            if (c != '\n' && idx < sizeof(linha) - 1) {
                linha[idx++] = c;
            } else {
                linha[idx] = '\0';
                idx = 0;

                if (strncmp(linha, "Filename:", 9) == 0) {
                    sscanf(linha, "Filename: %127[^\n]", doc.path);
                } else if (strncmp(linha, "Title:", 6) == 0) {
                    sscanf(linha, "Title: %255[^\n]", doc.title);
                } else if (strncmp(linha, "Year:", 5) == 0) {
                    sscanf(linha, "Year: %d", &doc.year);
                } else if (strncmp(linha, "Authors:", 8) == 0) {
                    sscanf(linha, "Authors: %255[^\n]", doc.authors);
                    doc.id = proximo_id++;

                    escrever_metadados_struct(&doc);

                    memset(&doc, 0, sizeof(Documentos));
                }
            }
        }
    }
}

void processar_comando_script(const char *comando) {
    char script[256], dataset[256];

    if (sscanf(comando, "%s %s", script, dataset) == 2) {
        printf("Executando script: %s com dataset: %s\n", script, dataset);

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("Erro ao criar pipe");
            return;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("Erro ao criar processo filho");
            return;
        }

        if (pid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            execlp("bash", "bash", script, dataset, NULL);
            perror("Erro ao executar script");
            exit(EXIT_FAILURE);
        } else {
            close(pipefd[1]);
            armazenar_metadados(pipefd[0]);
            close(pipefd[0]);
            wait(NULL);
            printf("Script executado. Metadados armazenados.\n");
        }
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

void processar_comando(const char *comando) {
    if (strncmp(comando, "ADD|", 4) == 0) {
        Documentos doc;
        if (sscanf(comando, "ADD|%200[^|]|%200[^|]|%d|%64[^\n]", doc.title, doc.authors, &doc.year, doc.path) == 4) {
            doc.id = proximo_id++;
            escrever_metadados_struct(&doc);

            char resposta[64];
            snprintf(resposta, sizeof(resposta), "ID atribuído: %d", doc.id);
            send_response(resposta);
        } else {
            send_response("Erro: comando ADD mal formatado.");
        }
    } else {
        processar_comando_script(comando);
        send_response("Metadados armazenados com sucesso!\n");
    }
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

        ssize_t bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            processar_comando(buffer);
        }

        close(pipe_fd);
    }

    return 0;
}
