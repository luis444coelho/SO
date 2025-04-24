#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "cache.h"

Cache* criar_cache(int capacidade) {
    Cache *cache = (Cache *)malloc(sizeof(Cache));
    cache->tamanho = 0;
    cache->capacidade = capacidade;
    cache->head = cache->tail = NULL;
    return cache;
}


void mover_para_frente(Cache *c, CacheNode *n) {
    if (c->head == n) return;

    if (n->prev) n->prev->next = n->next;
    if (n->next) n->next->prev = n->prev;
    if (c->tail == n) c->tail = n->prev;

    n->prev = NULL;
    n->next = c->head;
    if (c->head) c->head->prev = n;
    c->head = n;
    if (c->tail == NULL) c->tail = n;
}

Documentos* procurar_na_cache(Cache *c, int id) {
    CacheNode *n = c->head;
    while (n) {
        if (n->doc.id == id) {
            mover_para_frente(c, n);
            return &n->doc;
        }
        n = n->next;
    }
    return NULL;
}

//Neste momento estamos a mandar o ultimo de pica

void adicionar_na_cache(Cache *c, Documentos d) {

    CacheNode *n = c->head;
    while (n) {
        if (n->doc.id == d.id) {
            n->doc = d;
            mover_para_frente(c, n);
            return;
        }
        n = n->next;
    }

    if (c->tamanho >= c->capacidade) {
        CacheNode *remover = c->tail;
        if (remover->prev) remover->prev->next = NULL;
        c->tail = remover->prev;
        if (c->head == remover) c->head = NULL;
        free(remover);
        c->tamanho--;
    }

    CacheNode *novo = malloc(sizeof(CacheNode));
    novo->doc = d;
    novo->prev = NULL;
    novo->next = c->head;
    if (c->head) c->head->prev = novo;
    c->head = novo;
    if (c->tail == NULL) c->tail = novo;
    c->tamanho++;
}
