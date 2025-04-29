#include "utils.h"
#include "cache.h"

#ifndef executar_H
#define executar_H

extern int proximo_id;

extern char base_folder[256];

void processar_add(Comando *cmd);

Documentos processar_consult(Comando *cmd, Cache *cache);

int processar_shutdown(Comando *cmd);

void processar_remove(Comando *cmd);

void processar_lines(Comando *cmd);

void processar_search(Comando *cmd);

void processar_search_parallel(Comando *cmd);



#endif