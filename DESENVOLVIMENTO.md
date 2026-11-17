# Desenvolvimento do Fofita com GitHub Actions e Codespaces

Esta configuração foi criada numa branch separada para não modificar o dashboard sem revisão.

## Gerar Fofita.exe automaticamente

1. Abra **Actions > Build Fofita - Windows**.
2. Quando houver uma execução concluída com sucesso, abra a execução.
3. Em **Artifacts**, baixe `Fofita-Windows`.
4. Extraia os ZIPs e execute `Fofita.exe` no Windows.

O pacote inclui recursos presentes no repositório (`ui/`, `assets/`, `labels/`, `templates/`, `cores/`) e as DLLs MinGW/SDL2 localizadas. **ROMs não são incluídas.**
Se os recursos do projeto usarem subpastas diferentes, ajuste a seção de empacotamento no workflow.

O Windows build usa MSYS2 + MinGW64, SDL2 e SDL2_image. A execução do build, a abertura da janela e o lançamento de jogos precisam ser validados na aba Actions e no seu PC.

## Abrir o Codespaces

1. Acesse o repositório no GitHub.
2. Selecione a branch `setup/ci-windows-codespaces`.
3. Clique em **Code > Codespaces > Create codespace on ...**.
4. Depois da configuração inicial, abra o terminal.
5. Execute `bash .devcontainer/check-linux.sh`.

No Codespaces/Linux, o script verifica a sintaxe dos módulos portáveis. **Não compila nem executa a aplicação completa**, porque `libretro_core.c` depende de APIs do Windows. Para obter um EXE Windows, use o workflow de Actions.

## Próximos passos

- Validar o primeiro build no GitHub Actions.
- Incorporar a versão mais recente do redesign em uma branch de trabalho sem sobrescrever a main.
- Ajustar dependências e empacotamento conforme os erros reais do build.
- Posteriormente acrescentar um ambiente e compilação nativa para o Nintendo Switch via devkitPro.
