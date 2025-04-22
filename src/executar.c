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
    doc->ativo = 1;

    int fd = open(METADATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1) {
        perror("Erro ao abrir ficheiro de metadados");
        return;
    }

    write(fd, doc, sizeof(Documentos));
    close(fd);
}


void processar_add(Comando *cmd) {
    Documentos *doc = &cmd->doc;

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", base_folder, doc->path);

    if (access(full_path, F_OK) != 0) {
        send_response_to("Erro: o ficheiro especificado não existe na pasta base.", cmd->response_pipe);
        return;
    }

    int fd = open(METADATA_FILE, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        send_response_to("Erro ao abrir ficheiro de metadados.", cmd->response_pipe);
        return;
    }

    Documentos temp;
    int encontrado_slot = 0;
    int menor_id_disponivel = -1;

    while (read(fd, &temp, sizeof(Documentos)) == sizeof(Documentos)) {
        if (temp.ativo == 0 && (menor_id_disponivel == -1 || temp.id < menor_id_disponivel)) {
            menor_id_disponivel = temp.id;
        }
    }

    if (menor_id_disponivel != -1) {
        doc->id = menor_id_disponivel;
        doc->ativo = 1;

        lseek(fd, 0, SEEK_SET);

        while (read(fd, &temp, sizeof(Documentos)) == sizeof(Documentos)) {
            if (temp.id == menor_id_disponivel) {
                lseek(fd, -sizeof(Documentos), SEEK_CUR);
                write(fd, doc, sizeof(Documentos));
                encontrado_slot = 1;
                break;
            }
        }
    } else {

        doc->id = proximo_id++;
        doc->ativo = 1;

        lseek(fd, 0, SEEK_END);
        write(fd, doc, sizeof(Documentos));
    }

    close(fd);

    char resposta[64];
    snprintf(resposta, sizeof(resposta), "ID atribuído: %d", doc->id);
    send_response_to(resposta, cmd->response_pipe);
}

void processar_consult(Comando *cmd) {
    int id_procurado = cmd->id;
    int fd = open(METADATA_FILE, O_RDONLY);

    if (fd == -1) {
        send_response_to("Erro: ficheiro de metadados não encontrado.", cmd->response_pipe);
        return;
    }

    Documentos doc;
    int encontrado = 0;

    while (read(fd, &doc, sizeof(Documentos)) == sizeof(Documentos)) {
        if (doc.id == id_procurado && doc.ativo) {
            encontrado = 1;
            break;
        }
    }

    close(fd);

    if (encontrado) {
        char resposta[512];
        snprintf(resposta, sizeof(resposta),
                 "Title: %s\nAuthors: %s\nYear: %d\nPath: %s",
                 doc.title, doc.authors, doc.year, doc.path);
        send_response_to(resposta, cmd->response_pipe);
    } else {
        send_response_to("Erro: documento com o ID especificado não encontrado.", cmd->response_pipe);
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


void processar_remove(Comando *cmd) {
    int id_remover = cmd->id;
    int fd = open(METADATA_FILE, O_RDWR);
    if (fd == -1) {
        send_response_to("Erro: ficheiro de metadados não encontrado.", cmd->response_pipe);
        return;
    }

    Documentos doc;
    int encontrado = 0;

    while (read(fd, &doc, sizeof(Documentos)) == sizeof(Documentos)) {
        if (doc.id == id_remover && doc.ativo) {
            encontrado = 1;
            doc.ativo = 0;


            lseek(fd, -sizeof(Documentos), SEEK_CUR);
            write(fd, &doc, sizeof(Documentos));
            break;
        }
    }

    close(fd);

    if (encontrado) {
        char resposta[64];
        snprintf(resposta, sizeof(resposta), "Entrada %d marcada como removida.", id_remover);
        send_response_to(resposta, cmd->response_pipe);
    } else {
        send_response_to("Erro: documento com o ID especificado não encontrado ou já removido.", cmd->response_pipe);
    }
}

void processar_lines(Comando *cmd) {
    int id = cmd->id;
    char *keyword = cmd->keyword;

    int fd_meta = open(METADATA_FILE, O_RDONLY);
    if (fd_meta == -1) {
        send_response_to("Erro: arquivo de metadados não encontrado.", cmd->response_pipe);
        return;
    }

    Documentos doc;
    int encontrado = 0;

    while (read(fd_meta, &doc, sizeof(Documentos)) == sizeof(Documentos)) {
        if (doc.id == id && doc.ativo) {
            encontrado = 1;
            break;
        }
    }

    close(fd_meta);

    if (!encontrado) {
        send_response_to("Erro: documento com o ID especificado não encontrado.", cmd->response_pipe);
        return;
    }

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", base_folder, doc.path);

    int fd = open(full_path, O_RDONLY);
    if (fd == -1) {
        send_response_to("Erro: não foi possível abrir o arquivo do documento.", cmd->response_pipe);
        return;
    }

    char buf[1024];
    ssize_t bytes_read;
    size_t line_len = 0;
    char line[1024];
    int count = 0;

    while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (line_len < sizeof(line) - 1) {
                line[line_len++] = buf[i];
            }

            if (buf[i] == '\n') {
                line[line_len] = '\0';

                if (strstr(line, keyword) != NULL) {
                    count++;
                }

                line_len = 0; 
            }
        }
    }

    if (line_len > 0) {
        line[line_len] = '\0';
        if (strstr(line, keyword) != NULL) {
            count++;
        }
    }

    close(fd);

    char resposta[64];
    snprintf(resposta, sizeof(resposta), "%d", count);
    send_response_to(resposta, cmd->response_pipe);
}


