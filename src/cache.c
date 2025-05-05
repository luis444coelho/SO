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

// Política LRU
void adicionar_lru(Cache *c, Documentos d) {
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
        if (remover->prev) {
            remover->prev->next = NULL;
            c->tail = remover->prev;
        } else {
            c->head = NULL;
            c->tail = NULL;
        }
        free(remover);
        c->tamanho--;
    }

    CacheNode *novo = malloc(sizeof(CacheNode));
    novo->doc = d;
    novo->prev = NULL;
    novo->next = c->head;

    if (c->head) c->head->prev = novo;
    else c->tail = novo;

    c->head = novo;
    c->tamanho++;
}

// Política FIFO
void adicionar_fifo(Cache *c, Documentos d) {
    CacheNode *n = c->head;
    while (n) {
        if (n->doc.id == d.id) {
            return; 
        }
        n = n->next;
    }

    if (c->tamanho >= c->capacidade) {
        CacheNode *remover = c->head;
        if (remover->next) {
            c->head = remover->next;
            c->head->prev = NULL;
        } else {
            c->head = c->tail = NULL;
        }
        free(remover);
        c->tamanho--;
    }

    CacheNode *novo = malloc(sizeof(CacheNode));
    novo->doc = d;
    novo->next = NULL;
    novo->prev = c->tail;

    if (c->tail) c->tail->next = novo;
    else c->head = novo;

    c->tail = novo;
    c->tamanho++;
}

// Política MRU (Most Recently Used)
void adicionar_mru(Cache *c, Documentos d) {
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
        CacheNode *remover = c->head;
        c->head = remover->next;
        
        if (c->head)
            c->head->prev = NULL;
        else
            c->tail = NULL;
            
        free(remover);
        c->tamanho--;
    }

    CacheNode *novo = malloc(sizeof(CacheNode));
    novo->doc = d;
    novo->prev = NULL;
    novo->next = c->head;
    
    if (c->head) 
        c->head->prev = novo;
    else
        c->tail = novo;
    
    c->head = novo;
    c->tamanho++;
}

void adicionar_na_cache(Cache *c, Documentos d, int politica) {
    if (politica == 1)
        adicionar_lru(c, d);
    else if (politica == 2)
        adicionar_fifo(c, d);
    else if (politica == 3)
        adicionar_mru(c, d);
    else
        return; // Política inválida
}


//Função para debug
void imprimir_cache(Cache *c) {
    printf("Documentos na cache (mais recente -> mais antigo):\n");
    CacheNode *n = c->head;
    while (n) {
        printf("ID: %d\n", n->doc.id);
        n = n->next;
    }
}
