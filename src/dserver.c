#include "../include/executar.h"


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


void processar(Comando *cmd) {
    switch (cmd->tipo) {
        case CMD_ADD:
            processar_add(cmd);
            break;

        case CMD_SHUTDOWN:
            processar_shutdown(cmd);
            break;

        case CMD_CONSULT:
            processar_consult(cmd);
            break;

        default:
            send_response_to("Erro: comando não reconhecido.",cmd -> response_pipe);
            break;
    }
}


int main(int argc, char *argv[]) {
    // Cria o pipe principal se não existir
    
    int fd_comando = -1;
    mkfifo(PIPE_NAME, 0666);

    inicializar_proximo_id();

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <document_folder>\n", argv[0]);
        return 1;
    }

    strncpy(base_folder, argv[1], sizeof(base_folder));

    while (1) {
        fd_comando = open(PIPE_NAME, O_RDONLY);
        if (fd_comando == -1) {
            perror("Erro ao abrir pipe principal");
            return 1;
        }

        Comando cmd;
        ssize_t bytes;
        while ((bytes = read(fd_comando, &cmd, sizeof(Comando))) > 0) {
            if (bytes == sizeof(Comando)) {
                processar(&cmd);
            }
        }

        close(fd_comando); // Chegou EOF, cliente fechou pipe de escrita
    }

    return 0;
}
