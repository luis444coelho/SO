#include "../include/executar.h"
#include "../include/cache.h"


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



int processar(Comando *cmd, Cache *cache) {
    switch (cmd->tipo) {
        case CMD_ADD:
            processar_add(cmd);
            return 1; 
            
        case CMD_SHUTDOWN:
            return processar_shutdown(cmd); 
            
        case CMD_CONSULT:
            processar_consult(cmd, cache);
            return 1;
            
        case CMD_REMOVE:
            processar_remove(cmd);
            return 1;
            
        case CMD_LINES:
            processar_lines(cmd);
            return 1;
            
        case CMD_SEARCH:
            processar_search(cmd);
            return 1;
            
        case CMD_SEARCH_PARALLEL:
            processar_search_parallel(cmd);
            return 1;
            
        default:
            send_response_to("Erro: comando não reconhecido.", cmd->response_pipe);
            return 1;
    }
}


int main(int argc, char *argv[]) {

    int fd_comando = -1;
    mkfifo(PIPE_NAME, 0666);

    inicializar_proximo_id();

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <document_folder> <cache_size>\n", argv[0]);
        return 1;
    }

    strncpy(base_folder, argv[1], sizeof(base_folder));

    int cache_capacidade = atoi(argv[2]);
    Cache* cache = criar_cache(cache_capacidade);

    int continuar = 1;
    while (continuar) {
        fd_comando = open(PIPE_NAME, O_RDONLY);
        if (fd_comando == -1) {
            perror("Erro ao abrir pipe principal");
            return 1;
        }
    
        Comando cmd;
        ssize_t bytes;
        while ((bytes = read(fd_comando, &cmd, sizeof(Comando))) > 0) {
            if (bytes == sizeof(Comando)) {

                
                if (cmd.tipo == CMD_CONSULT || cmd.tipo == CMD_SEARCH || cmd.tipo == CMD_LINES) {
                    pid_t pid = fork();
                    if (pid == 0) {
                        processar(&cmd);
                        _exit(0);  
                    } else if (pid < 0) {
                        perror("Erro ao criar fork");
                    }
                    
                } else {
                    
                    processar(&cmd);
                }
            }
        }
    
        close(fd_comando); // Chegou EOF, cliente fechou pipe de escrita

    }
    

    return 0;
}
