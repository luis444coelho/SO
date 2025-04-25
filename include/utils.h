#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <glib.h>

#ifndef utils_H
#define utils_H

#define PIPE_NAME "doc_pipe"
#define METADATA_FILE "metadata.txt"

typedef struct {
    int id;
    char title[200];
    char authors[200];
    int year;
    char path[64];
    int ativo; //1 para ativo, 0 para inativo
}Documentos;

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
    int id;
    char keyword[64];
    Documentos doc;
    int num_processos;
    char response_pipe[128]; 
} Comando;

Comando parse_comando(int argc, char *argv[]);

#endif
