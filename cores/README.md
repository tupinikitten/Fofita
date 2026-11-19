# Cores Libretro do Fofita

A pasta contém somente cores cujas licenças e distribuições foram verificadas. Nunca inclua ROMs, arquivos de BIOS proprietários, senhas ou tokens do RetroAchievements.

## Cores configurados

| Sistema | DLL Windows esperada | Situação |
| --- | --- | --- |
| NES | `fceumm_libretro.dll` | Já presente no projeto |
| Game Boy | `gambatte_libretro.dll` | Precisa ser instalada e testada |
| Game Boy Color | `gambatte_libretro.dll` | Usa a mesma DLL do Game Boy |

### Gambatte — como instalar para testar no Windows

1. Obtenha a compilação **Libretro Windows x86_64** do Gambatte por um canal oficial do projeto Libretro/RetroArch, de acordo com as condições de licença.
2. Confira a origem e compatibilidade da DLL. Copie-a para `cores/gambatte_libretro.dll` ao lado do aplicativo.
3. Coloque ROMs de teste que você pode usar legalmente nas pastas `roms/gb` e `roms/gbc` (crie-as se estiverem ausentes).
4. Reinicie o Fofita; a seleção de sistemas só exibe cores cujos arquivos DLL foram encontrados.
5. Teste abrir, executar, controlar e retornar ao menu nos dois sistemas.

Não renomeie `retroarch.exe` como core: o Fofita precisa da **DLL do core Libretro**, não do programa RetroArch.

### Considerações de licença

Gambatte usa **GPLv2** segundo a documentação oficial. Antes de distribuir a DLL junto do Fofita, documentar a versão exata, licença, origem do código-fonte correspondente e obrigações de distribuição. Essa etapa não é atendida automaticamente por apenas adicionar uma DLL.

- [Documentação oficial Gambatte](https://docs.libretro.com/library/gambatte/)
- [Repositório da implementação Libretro](https://github.com/libretro/gambatte-libretro)

**Observação:** este commit não adiciona a DLL Gambatte ao repositório. Assim, o menu continuará mostrando apenas os sistemas ativos na instalação até a DLL ser fornecida.
