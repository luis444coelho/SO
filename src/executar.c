#include "../include/executar.h"

char base_folder[256];
int proximo_id = 1;

void send_response_to(const char *msg, const char *pipe_name) {
    int fd = open(pipe_name, O_WRONLY);
    if (fd == -1) {
        perror("Erro ao abrir pipe de resposta do cliente");
        return;
    }
    write(fd, msg, strlen(msg) + 1);
    close(fd);
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


void processar_add(Comando *cmd) {
    Documentos *doc = &cmd->doc;
 
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", base_folder, doc->path);

    if (access(full_path, F_OK) != 0) {
        send_response_to("Erro: o ficheiro especificado não existe na pasta base.",cmd -> response_pipe);
        return;
    }

    doc->id = proximo_id++;
    escrever_metadados(doc);

    char resposta[64];
    snprintf(resposta, sizeof(resposta), "ID atribuído: %d", doc->id);
    send_response_to(resposta,cmd -> response_pipe);
}

void processar_consult(Comando *cmd) {
    int id_procurado = cmd->id;
    int fd = open(METADATA_FILE, O_RDONLY);

    if (fd == -1) {
        send_response_to("Erro: ficheiro de metadados não encontrado.",cmd -> response_pipe);
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
        send_response_to(resposta,cmd -> response_pipe);
    } else {
        send_response_to("Erro: documento com o ID especificado não encontrado.",cmd -> response_pipe);
    }
}

void processar_shutdown (Comando *cmd) {
    int fd_comando = -1;
    send_response_to("Servidor a encerrar...",cmd -> response_pipe);
    close(fd_comando);
    unlink(PIPE_NAME);
    unlink(RESPONSE_PIPE);
    exit(0);
}
