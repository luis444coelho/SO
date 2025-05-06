#include "../include/executar.h"
#include "../include/cache.h"


char base_folder[256];
int proximo_id = 1;

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

Documentos processar_consult(Comando *cmd, Cache *cache) {
    int id_procurado = cmd->id;

    Documentos *doc_cache = procurar_na_cache(cache, id_procurado);
    
    if (doc_cache) {

        char resposta[512];
        snprintf(resposta, sizeof(resposta),
                 "Title: %s\nAuthors: %s\nYear: %d\nPath: %s",
                 doc_cache->title, doc_cache->authors, doc_cache->year, doc_cache->path);
        send_response_to(resposta, cmd->response_pipe);
        return *doc_cache;
    }

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

    return doc;
}


int processar_shutdown(Comando *cmd) {
    send_response_to("Servidor a encerrar...", cmd->response_pipe);
    unlink(PIPE_NAME);
    return 0; 
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

    int p1[2], p2[2];
    pipe(p1);
    pipe(p2);

    pid_t pid1 = fork();
    if (pid1 == 0) {
        dup2(p1[1], STDOUT_FILENO);
        close(p1[0]); close(p1[1]);
        execlp("grep", "grep", keyword, full_path, NULL);
        _exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        dup2(p1[0], STDIN_FILENO);
        dup2(p2[1], STDOUT_FILENO);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]);
        execlp("wc", "wc", "-l", NULL);
        _exit(1);
    }

    close(p1[0]); close(p1[1]);
    close(p2[1]);

    char buffer[64];
    ssize_t n = read(p2[0], buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        send_response_to(buffer, cmd->response_pipe);
    } else {
        send_response_to("0", cmd->response_pipe);
    }

    close(p2[0]);
    wait(NULL);
    wait(NULL);
}


void processar_search(Comando *cmd) {
    char *keyword = cmd->keyword;
    int fd_meta = open(METADATA_FILE, O_RDONLY);
    if (fd_meta == -1) {
        send_response_to("Erro: ficheiro de metadados não encontrado.", cmd->response_pipe);
        return;
    }

    Documentos doc;
    int *ids_encontrados = NULL;
    int count = 0;

    while (read(fd_meta, &doc, sizeof(Documentos)) == sizeof(Documentos)) {
        if (!doc.ativo) continue;

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_folder, doc.path);

        pid_t pid = fork();
        if (pid == 0) {
            int devnull = open("/dev/null", O_WRONLY);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
            execlp("grep", "grep", "-q", "-w", keyword, full_path, NULL);
            _exit(1);
        }

        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            ids_encontrados = realloc(ids_encontrados, (count + 1) * sizeof(int));
            ids_encontrados[count++] = doc.id;
        }
    }

    close(fd_meta);

    if (count == 0) {
        send_response_to("Nenhum documento encontrado com essa palavra-chave.", cmd->response_pipe);
        return;
    }

    // Ordenar os IDs encontrados
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (ids_encontrados[j] > ids_encontrados[j + 1]) {
                int temp = ids_encontrados[j];
                ids_encontrados[j] = ids_encontrados[j + 1];
                ids_encontrados[j + 1] = temp;
            }
        }
    }

    // Construir resposta com buffer dinâmico
    size_t resposta_size = 64;
    char *resposta = malloc(resposta_size);
    strcpy(resposta, "[");

    for (int i = 0; i < count; i++) {
        char temp[32];
        snprintf(temp, sizeof(temp), "%s%d", i > 0 ? ", " : "", ids_encontrados[i]);

        if (strlen(resposta) + strlen(temp) + 2 > resposta_size) {
            resposta_size *= 2;
            resposta = realloc(resposta, resposta_size);
        }

        strcat(resposta, temp);
    }
    strcat(resposta, "]");

    send_response_to(resposta, cmd->response_pipe);

    free(resposta);
    free(ids_encontrados);
}




void processar_search_parallel(Comando *cmd) {
    char *keyword = cmd->keyword;
    int num_processos = cmd->num_processos;

    int fd_meta = open(METADATA_FILE, O_RDONLY);
    if (fd_meta == -1) {
        send_response_to("Erro: ficheiro de metadados não encontrado.", cmd->response_pipe);
        return;
    }

    Documentos doc;
    int *doc_ids = NULL;
    char **doc_paths = NULL;
    int total_docs = 0;

    while (read(fd_meta, &doc, sizeof(Documentos)) == sizeof(Documentos)) {
        if (doc.ativo) {
            doc_ids = realloc(doc_ids, (total_docs + 1) * sizeof(int));
            doc_paths = realloc(doc_paths, (total_docs + 1) * sizeof(char*));

            doc_ids[total_docs] = doc.id;
            doc_paths[total_docs] = malloc(512 * sizeof(char));
            snprintf(doc_paths[total_docs], 512, "%s/%s", base_folder, doc.path);

            total_docs++;
        }
    }

    close(fd_meta);

    if (total_docs == 0) {
        send_response_to("Nenhum documento encontrado.", cmd->response_pipe);
        return;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        send_response_to("Erro ao criar pipe.", cmd->response_pipe);
        goto cleanup;
    }

    if (num_processos > total_docs) {
        num_processos = total_docs;
    }

    for (int i = 0; i < num_processos; i++) {
        pid_t pid = fork();
        if (pid == -1) continue;

        if (pid == 0) {
            close(pipe_fd[0]);

            for (int j = i; j < total_docs; j += num_processos) {
                int devnull = open("/dev/null", O_WRONLY);
                pid_t grep_pid = fork();

                if (grep_pid == 0) {
                    dup2(devnull, STDERR_FILENO);
                    close(devnull);
                    execlp("grep", "grep", "-q", "-w", keyword, doc_paths[j], NULL);
                    _exit(1);
                }

                close(devnull);
                int status;
                waitpid(grep_pid, &status, 0);

                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    write(pipe_fd[1], &doc_ids[j], sizeof(int));
                }
            }

            close(pipe_fd[1]);
            for (int j = 0; j < total_docs; j++) free(doc_paths[j]);
            free(doc_paths);
            free(doc_ids);
            _exit(0);
        }
    }

    close(pipe_fd[1]);

    int *ids_encontrados = NULL;
    int count = 0, id;

    while (read(pipe_fd[0], &id, sizeof(int)) == sizeof(int)) {
        int duplicado = 0;
        for (int i = 0; i < count; i++) {
            if (ids_encontrados[i] == id) {
                duplicado = 1;
                break;
            }
        }

        if (!duplicado) {
            ids_encontrados = realloc(ids_encontrados, (count + 1) * sizeof(int));
            ids_encontrados[count++] = id;
        }
    }

    close(pipe_fd[0]);
    while (wait(NULL) > 0);

    if (count == 0) {
        send_response_to("Nenhum documento encontrado com essa palavra-chave.", cmd->response_pipe);
    } else {
        // Ordenar
        for (int i = 0; i < count - 1; i++) {
            for (int j = 0; j < count - i - 1; j++) {
                if (ids_encontrados[j] > ids_encontrados[j + 1]) {
                    int temp = ids_encontrados[j];
                    ids_encontrados[j] = ids_encontrados[j + 1];
                    ids_encontrados[j + 1] = temp;
                }
            }
        }

        // Resposta dinâmica
        size_t resposta_size = 64;
        char *resposta = malloc(resposta_size);
        strcpy(resposta, "[");

        for (int i = 0; i < count; i++) {
            char temp[32];
            snprintf(temp, sizeof(temp), "%s%d", i > 0 ? ", " : "", ids_encontrados[i]);

            if (strlen(resposta) + strlen(temp) + 2 > resposta_size) {
                resposta_size *= 2;
                resposta = realloc(resposta, resposta_size);
            }

            strcat(resposta, temp);
        }
        strcat(resposta, "]");

        send_response_to(resposta, cmd->response_pipe);
        free(resposta);
    }

    free(ids_encontrados);

cleanup:
    for (int i = 0; i < total_docs; i++) {
        free(doc_paths[i]);
    }
    free(doc_paths);
    free(doc_ids);
}
