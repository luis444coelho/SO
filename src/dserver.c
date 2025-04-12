#include "../include/utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>


char base_folder[256]; 
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

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_folder, doc->path);

        if (access(full_path, F_OK) != 0) {
            send_response("Erro: o ficheiro especificado não existe na pasta base.");
            return;
        }

        doc->id = proximo_id++;
        escrever_metadados(doc);

        char resposta[64];
        snprintf(resposta, sizeof(resposta), "ID atribuído: %d", doc->id);
        send_response(resposta);

    } else if (cmd->tipo == CMD_SHUTDOWN) {
        encerrar_servidor();

    } else if(cmd -> tipo == CMD_CONSULT){
        int id_procurado = cmd->id;
        int fd = open(METADATA_FILE, O_RDONLY);
    
        if (fd == -1) {
            send_response("Erro: ficheiro de metadados não encontrado.");
            return;
        }
    
        char buffer[512];
        char linha[512];
        int idx = 0;
        ssize_t bytes;
        int encontrado = 0;
        Documentos doc_encontrado;
    
        while ((bytes = read(fd, buffer, sizeof(buffer))) > 0) {
            for (ssize_t i = 0; i < bytes; ++i) {
                char c = buffer[i];
                if (c != '\n' && idx < sizeof(linha) - 1) {
                    linha[idx++] = c;
                } else {
                    linha[idx] = '\0';
                    idx = 0;
    
                    int id, year;
                    char title[200], authors[200], path[256];
    
                    if (sscanf(linha, "%d,%199[^,],%199[^,],%d,%255[^\n]",
                               &id, title, authors, &year, path) == 5) {
                        if (id == id_procurado) {
                            doc_encontrado.id = id;
                            strncpy(doc_encontrado.title, title, sizeof(doc_encontrado.title));
                            strncpy(doc_encontrado.authors, authors, sizeof(doc_encontrado.authors));
                            doc_encontrado.year = year;
                            strncpy(doc_encontrado.path, path, sizeof(doc_encontrado.path));
                            encontrado = 1;
                            break;
                        }
                    }
                }
            }
    
            if (encontrado) break;
        }
    
        close(fd);
    
        if (encontrado) {
            char resposta[512];
            snprintf(resposta, sizeof(resposta),
                     "Title: %s\nAuthors: %s\nYear: %d\nPath: %s",
                     doc_encontrado.title, doc_encontrado.authors,
                     doc_encontrado.year, doc_encontrado.path);
            send_response(resposta);
        } else {
            send_response("Erro: documento com o ID especificado não encontrado.");
        }
    }
}

int main(int argc, char *argv[]) {
    mkfifo(PIPE_NAME, 0666);
    mkfifo(RESPONSE_PIPE, 0666);

    inicializar_proximo_id();

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <document_folder>\n", argv[0]);
        return 1;
    }
    
    strncpy(base_folder, argv[1], sizeof(base_folder));
    
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
