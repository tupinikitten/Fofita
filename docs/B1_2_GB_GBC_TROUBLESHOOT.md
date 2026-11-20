# Fallback visual e pastas de GB/GBC

Correção após teste da B1.2.

## Motivo

O Fofita só lista no cabeçalho os sistemas cujo arquivo de core foi encontrado em `cores/`. Por isso, GB e GBC exigem uma **DLL `gambatte_libretro.dll` descompactada**, junto do Fofita B1.2 ou posterior. Copiar somente o ZIP, usar uma subpasta extra ou abrir um `Fofita.exe` antigo impede que apareçam.

Outra falha era no modo cartucho: GB/GBC não têm molde ainda, portanto nenhuma imagem era desenhada. Agora, se o molde não existir, o carrossel mostra um **card digital automaticamente**, independentemente da preferência global. Para NES com molde, nada muda.

O pacote Windows também cria `roms/nes/`, `roms/gb/` e `roms/gbc/` com instruções curtas. Os arquivos README não são ROMs.

## Layout esperado ao descompactar

```text
Fofita/
  Fofita.exe
  cores/
    fceumm_libretro.dll
    gambatte_libretro.dll     <- extrair daqui, nao deixar .zip
  roms/
    nes/
      jogo.nes
    gb/
      jogo.gb
    gbc/
      jogo.gbc
```

## Diagnóstico manual

- Conferir no Explorer se `gambatte_libretro.dll` tem exatamente esse nome e está na pasta `cores` ao lado do `Fofita.exe` **que é executado**.
- Descompactar o pacote Windows mais recente da branch `feature/backend-foundation`, em vez de reaproveitar um executável de antes da B1.2.
- Reiniciar o programa depois de copiar a DLL (a lista de consoles é montada na inicialização).
- Se GB/GBC ainda não aparecerem, verificar a configuração de consoles ocultos e restaurá-los.
- Se aparecerem sem jogos, conferir extensões e subpastas; `roms/gb` aceita `.gb` e `.dmg`, `roms/gbc` aceita `.gbc`.
- O build verde **não valida execução real** do Gambatte; testar no Windows com ROMs que você pode usar legalmente.

## Preservação

Nenhum código de moldura, sidebar ou tema foi alterado. NES ainda usa o molde original no modo cartucho. A branch `main` não foi modificada.
