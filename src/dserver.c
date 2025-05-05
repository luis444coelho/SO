#include "../include/executar.h"

int politica_cache = 1; // 1 = LRU, 2 = FIFO (padrão: LRU)

void inicializar_proximo_id() {
    int fd = open(METADATA_FILE, O_RDONLY);
    if (fd == -1) {
        proximo_id = 1;
        return;
    }

    Documentos doc;
    int max_id = 0;

    while (read(fd, &doc, sizeof(Documentos)) == sizeof(Documentos)) {
        if (doc.id > max_id) {
            max_id = doc.id;
        }
    }

    close(fd);
    proximo_id = max_id + 1;
}

Documentos processar(Comando *cmd, Cache *cache) {
    Documentos doc_vazio = {0};

    switch (cmd->tipo) {
        case CMD_ADD:
            processar_add(cmd);
            return doc_vazio;

        case CMD_SHUTDOWN:
            processar_shutdown(cmd);
            return doc_vazio;

        case CMD_CONSULT: {
            Documentos doc = processar_consult(cmd, cache);
            return doc;
        }

        case CMD_REMOVE:
            processar_remove(cmd);
            return doc_vazio;

        case CMD_LINES:
            processar_lines(cmd);
            return doc_vazio;

        case CMD_SEARCH:
            processar_search(cmd);
            return doc_vazio;

        case CMD_SEARCH_PARALLEL:
            processar_search_parallel(cmd);
            return doc_vazio;

        default:
            send_response_to("Erro: comando não reconhecido.", cmd->response_pipe);
            return doc_vazio;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Uso: %s <document_folder> <cache_size> [politica]\n", argv[0]);
        return 1;
    }

    mkfifo(PIPE_NAME, 0666);
    inicializar_proximo_id();

    strncpy(base_folder, argv[1], sizeof(base_folder) - 1);
    base_folder[sizeof(base_folder) - 1] = '\0';

    int cache_capacidade = atoi(argv[2]);
    if (cache_capacidade <= 0) {
        fprintf(stderr, "Erro: a capacidade da cache deve ser maior que zero.\n");
        return 1;
    }

    if (argc == 4) {
        politica_cache = atoi(argv[3]);
        if (politica_cache != 1 && politica_cache != 2 && politica_cache != 3) {
            fprintf(stderr, "Erro: política inválida. Use 1 (LRU) ou 2 (FIFO) ou 3 (MRU).\n");
            return 1;
        }
    }

    Cache* cache = criar_cache(cache_capacidade);

    int fd_comando = open(PIPE_NAME, O_RDONLY);
    if (fd_comando == -1) {
        perror("Erro ao abrir pipe principal");
        return 1;
    }

    int continuar = 1;
    while (continuar) {
        Comando cmd;
        ssize_t bytes = read(fd_comando, &cmd, sizeof(Comando));

        if (bytes == 0) {
            close(fd_comando);
            fd_comando = open(PIPE_NAME, O_RDONLY); 
            if (fd_comando == -1) {
                return 1;
            }
            continue;
        }

        if (bytes == -1) {
            perror("Erro ao ler do pipe principal");
            continue;
        }

        if (bytes != sizeof(Comando)) {
            fprintf(stderr, "Comando incompleto recebido\n");
            continue;
        }

        if (cmd.tipo == CMD_CONSULT || cmd.tipo == CMD_SEARCH || cmd.tipo == CMD_LINES) {
            int pipe_fd[2];
            if (pipe(pipe_fd) == -1) {
                perror("Erro ao criar pipe anônimo");
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) {
                close(pipe_fd[0]);
                Documentos doc = processar(&cmd, cache);
                write(pipe_fd[1], &doc, sizeof(Documentos));
                close(pipe_fd[1]);
                _exit(0);
            } else if (pid > 0) {
                close(pipe_fd[1]);

                Documentos doc_recebido;
                ssize_t lido = read(pipe_fd[0], &doc_recebido, sizeof(Documentos));
                if (lido == sizeof(Documentos)) {
                    if (cmd.tipo == CMD_CONSULT) {
                        adicionar_na_cache(cache, doc_recebido, politica_cache);
                    }
                } else {
                    perror("Erro ao ler do pipe anônimo");
                }

                close(pipe_fd[0]);
                waitpid(pid, NULL, 0);
            } else {
                perror("Erro ao criar fork");
                close(pipe_fd[0]);
                close(pipe_fd[1]);
            }
        } else {
            Documentos doc = processar(&cmd, cache);
            (void)doc;
        }

    }

    close(fd_comando);
    return 0;
}
