/* Biblioteca falsa para testar recusas controladas do carregador.
   STUB_ABI define a ABI informada. Os demais simbolos ficam ausentes. */
#include <stddef.h>
#ifdef _WIN32
#define EXPORTAR __declspec(dllexport)
#else
#define EXPORTAR __attribute__((visibility("default")))
#endif
#ifndef STUB_ABI
#define STUB_ABI 2
#endif
EXPORTAR size_t libra_instance_abi_version(void) { return STUB_ABI; }
EXPORTAR size_t libra_instance_api_version(void) { return 5; }
