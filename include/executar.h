#include "utils.h"

#ifndef executar_H
#define executar_H

extern int proximo_id;

extern char base_folder[256];

void processar_add(Comando *cmd);

void processar_consult(Comando *cmd);

void processar_shutdown(Comando *cmd);

void escrever_metadados(Documentos *doc);

void send_response_to(const char *msg, const char *pipe_name);

void processar_remove(Comando *cmd);


#endif