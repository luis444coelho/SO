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