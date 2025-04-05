

typedef struct {
    int id;                
    char title[200];       
    char authors[200];     
    int year [4];              
    char path[64];         
} Documentos;


typedef enum {
    CMD_ADD,
    CMD_CONSULT,
    CMD_REMOVE,
    CMD_LINES,
    CMD_SEARCH,
    CMD_SEARCH_PARALLEL,
    CMD_SHUTDOWN,
    CMD_INVALID
} TipoComando;

typedef struct {
    TipoComando tipo;

    // Comuns a vários comandos
    int id;
    char keyword[64];

    // Para o comando -a (adicionar)
    char title[200];
    char authors[200];
    int year;
    char path[64];

    // Para pesquisa paralela
    int num_processos;
} Comando;


Comando parse_comando(int argc, char *argv[]) {
    Comando cmd;
    memset(&cmd, 0, sizeof(Comando));

    if (argc < 2) {
        cmd.tipo = CMD_INVALID;
        return cmd;
    }

    if (strcmp(argv[1], "-a") == 0 && argc == 6) {
        cmd.tipo = CMD_ADD;
        strncpy(cmd.title, argv[2], sizeof(cmd.title));
        strncpy(cmd.authors, argv[3], sizeof(cmd.authors));
        cmd.year = atoi(argv[4]);
        strncpy(cmd.path, argv[5], sizeof(cmd.path));
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
    } else if (strcmp(argv[1], "-f") == 0 && argc == 2) {
        cmd.tipo = CMD_SHUTDOWN;
    } else {
        cmd.tipo = CMD_INVALID;
    }

    return cmd;
}


