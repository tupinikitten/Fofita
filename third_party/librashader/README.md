# librashader 0.12.0

- `librashader.h`: cópia sem alterações de `include/librashader.h` da tag `librashader-v0.12.0` (commit `87e8a97b50516d997defeaa168173dcd185d4022`). Licença **MIT** (aviso completo no início do arquivo).
- A biblioteca (`librashader.dll`) **não** é versionada aqui. O CI a baixa da release oficial com SHA-256 fixo (`tools/obter_librashader.sh`). Licença usada na redistribuição: **MPL-2.0** (o projeto oferece `MPL-2.0 OR GPL-3.0-only`).
- Os avisos de licença do pacote são gerados no CI por `tools/gerar_avisos_librashader.py` a partir do `Cargo.lock` oficial.

Ao atualizar a versão, atualizar juntos: o header, a versão/hash do script de download e a versão/commit do script de avisos.
