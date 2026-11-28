# librashader 0.12.0

- `librashader.h`: cópia sem alterações de `include/librashader.h` da tag `librashader-v0.12.0` (commit `87e8a97b50516d997defeaa168173dcd185d4022`). Licença **MIT** (aviso completo no início do arquivo).
- A biblioteca (`librashader.dll`) **não** é versionada aqui. O CI a compila do código oficial sem modificações, no commit acima, com `Cargo.lock` travado (`tools/compilar_librashader.sh`), somente com o runtime OpenGL e CRT estático. Licença usada na redistribuição: **MPL-2.0** (o projeto oferece `MPL-2.0 OR GPL-3.0-only`).
- O binário oficial da release (SHA-256 do zip `521fe0f364bfa705883f9e99fb1733a3a24f0f403bcc5591b6b78f6cff289183`, conferido no CI) não é usado: importa `D3DX9_43.dll`, `MSVCP140.dll` e `VCRUNTIME140*.dll` e falhou ao carregar no runner Windows.
- Os avisos de licença do pacote são gerados no CI por `tools/gerar_avisos_librashader.py` a partir do `Cargo.lock` oficial.

Ao atualizar a versão, atualizar juntos: o header, a tag/commit do script de compilação e a versão/commit do script de avisos (e a chave de cache do workflow).
