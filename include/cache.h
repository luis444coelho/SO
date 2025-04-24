#ifndef CACHE_H
#define CACHE_H

#include "utils.h"

typedef struct CacheNode {
    Documentos doc;
    struct CacheNode *prev, *next;
} CacheNode;

typedef struct {
    int tamanho;
    int capacidade;
    CacheNode *head;
    CacheNode *tail;
} Cache;

Cache* criar_cache(int capacidade);

Documentos* procurar_na_cache(Cache *c, int id);

void adicionar_na_cache(Cache *c, Documentos d);

#endif
