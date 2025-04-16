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

    write(fd, doc, sizeof(Documentos));
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
        send_response_to("Erro: ficheiro de metadados não encontrado.", cmd->response_pipe);
        return;
    }

    Documentos doc;
    int encontrado = 0;

    while (read(fd, &doc, sizeof(Documentos)) == sizeof(Documentos)) {
        if (doc.id == id_procurado) {
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
