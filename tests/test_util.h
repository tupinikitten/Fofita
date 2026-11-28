#ifndef FOFITA_TEST_UTIL_H
#define FOFITA_TEST_UTIL_H
#include <stdio.h>
static int testes_total, testes_falhos;
#define VERIFICAR(cond, ...) do { \
    ++testes_total; \
    if (!(cond)) { ++testes_falhos; \
        fprintf(stderr, "FALHOU %s:%d: ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } \
} while (0)
static inline int testes_resumo(const char *nome) {
    printf("%s: %d verificacoes, %d falhas\n", nome, testes_total, testes_falhos);
    return testes_falhos ? 1 : 0;
}
#endif
