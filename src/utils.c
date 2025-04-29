#include "../include/utils.h"

Comando parse_comando(int argc, char *argv[]) {
    Comando cmd;
    memset(&cmd, 0, sizeof(Comando));

    if (argc < 2) {
        cmd.tipo = CMD_INVALID;
        return cmd;
    }

    if (strcmp(argv[1], "-a") == 0 && argc == 6) {
        cmd.tipo = CMD_ADD;
        strncpy(cmd.doc.title, argv[2], sizeof(cmd.doc.title));
        strncpy(cmd.doc.authors, argv[3], sizeof(cmd.doc.authors));
        cmd.doc.year = atoi(argv[4]);
        strncpy(cmd.doc.path, argv[5], sizeof(cmd.doc.path));
    } else if (strcmp(argv[1], "-c") == 0 && argc == 3) {
        cmd.tipo = CMD_CONSULT;
        cmd.id = atoi(argv[2]);
    } else if (strcmp(argv[1], "-d") == 0 && argc == 3) {
        cmd.tipo = CMD_REMOVE;
        cmd.id = atoi(argv[2]);
    } else if (strcmp(argv[1], "-l") == 0 && argc == 4) {
        cmd.tipo = CMD_LINES;
        cmd.id = atoi(argv[2]);
        strncpy(cmd.keyword, argv[3], sizeof(cmd.keyword));
    } else if (strcmp(argv[1], "-s") == 0 && (argc == 3 || argc == 4)) {
        cmd.tipo = (argc == 4) ? CMD_SEARCH_PARALLEL : CMD_SEARCH;
        strncpy(cmd.keyword, argv[2], sizeof(cmd.keyword));
        if (argc == 4) cmd.num_processos = atoi(argv[3]);
    } else if (strcmp(argv[1], "-f") == 0) {
        cmd.tipo = CMD_SHUTDOWN;
    } else {
        cmd.tipo = CMD_INVALID;
    }

    return cmd;
}

void send_response_to(const char *msg, const char *pipe_name) {
    if (strlen(pipe_name) > 255) {
        fprintf(stderr, "Erro: nome do pipe demasiado longo.\n");
        return;
    }

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