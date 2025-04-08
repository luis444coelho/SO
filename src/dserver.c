#include "../include/utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

int proximo_id = 1;
int fd_comando = -1;

void inicializar_proximo_id() {
    int fd = open(METADATA_FILE, O_RDONLY);
    if (fd == -1) {
        proximo_id = 1;
        return;
    }

    char buffer[512];
    ssize_t bytes;
    int max_id = 0;
    char linha[512];
    int idx = 0;

    while ((bytes = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytes; ++i) {
            char c = buffer[i];
            if (c != '\n' && idx < sizeof(linha) - 1) {
                linha[idx++] = c;
            } else {
                linha[idx] = '\0';
                idx = 0;

                int id;
                if (sscanf(linha, "%d,", &id) == 1) {
                    if (id > max_id) {
                        max_id = id;
                    }
                }
            }
        }
    }

    close(fd);
    proximo_id = max_id + 1;
}

void escrever_metadados(Documentos *doc) {
    int fd = open(METADATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1) {
        perror("Erro ao abrir ficheiro de metadados");
        return;
    }

    char linha[512];
    int len = snprintf(linha, sizeof(linha), "%d,%s,%s,%d,%s\n",
                       doc->id, doc->title, doc->authors, doc->year, doc->path);
    write(fd, linha, len);
    close(fd);
}

void send_response(const char *msg) {
    int fd = open(RESPONSE_PIPE, O_WRONLY);
    if (fd == -1) {
        perror("Erro ao abrir resposta");
        return;
    }
    write(fd, msg, strlen(msg) + 1);
    close(fd);
}

void encerrar_servidor() {
    send_response("Servidor a encerrar...");
    close(fd_comando);
    unlink(PIPE_NAME);
    unlink(RESPONSE_PIPE);
    exit(0);
}

void processar(Comando *cmd) {
    if (cmd->tipo == CMD_ADD) {
        Documentos *doc = &cmd->doc;
        
        if (access(doc->path, F_OK) != 0) {
            send_response("Erro: o ficheiro especificado não existe.");
            return;
        }
        
        doc->id = proximo_id++;
        escrever_metadados(doc);

        char resposta[64];
        snprintf(resposta, sizeof(resposta), "ID atribuído: %d", doc->id);
        send_response(resposta);

    } else if (cmd->tipo == CMD_SHUTDOWN) {
        encerrar_servidor();

    } else {
        send_response("Comando ainda não implementado.");
    }
}

int main() {
    mkfifo(PIPE_NAME, 0666);
    mkfifo(RESPONSE_PIPE, 0666);

    inicializar_proximo_id();

    fd_comando = open(PIPE_NAME, O_RDONLY);
    if (fd_comando == -1) {
        perror("Erro ao abrir pipe principal");
        return 1;
    }

    while (1) {
        Comando cmd;
        ssize_t bytes = read(fd_comando, &cmd, sizeof(Comando));
        if (bytes == sizeof(Comando)) {
            processar(&cmd);
        }
    }

    return 0;
}
