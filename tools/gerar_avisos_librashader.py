#!/usr/bin/env python3
"""Gera os avisos de licenca do binario librashader distribuido pelo Fofita.

Lista, com `cargo tree --locked`, as dependencias de execucao do
librashader-capi (somente runtime OpenGL, Windows x64, mesmas opcoes da
compilacao) a partir do Cargo.lock da tag fixada e copia os textos de licenca presentes em cada pacote, incluindo codigo C/C++
embutido. Falha se algum componente so permitir licenca GPL/LGPL/AGPL ou nao
declarar licenca.

Uso: gerar_avisos_librashader.py <fonte_librashader> <pasta_saida>
"""
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

VERSAO = "0.12.0"
COMMIT = "87e8a97b50516d997defeaa168173dcd185d4022"
ALVO = "x86_64-pc-windows-msvc"
NOMES = re.compile(r"^(licen[cs]e|copying|copyright|notice|unlicense)([-_.].*)?$", re.I)
MIT_PADRAO = """Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE."""
CODIGO = {".rs", ".c", ".cpp", ".h", ".hpp", ".py", ".json"}


def so_copyleft(expressao):
    """True quando nenhuma alternativa (OR) evita GPL/LGPL/AGPL."""
    alternativas = re.split(r"\s+OR\s+|/", expressao.replace("(", "").replace(")", ""))
    return all(re.search(r"\b(A?GPL|LGPL)", a) for a in alternativas)


def main():
    fonte, saida = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
    head = subprocess.check_output(["git", "-C", str(fonte), "rev-parse", "HEAD"], text=True).strip()
    if head != COMMIT:
        sys.exit(f"commit inesperado: {head} (fixado: {COMMIT})")
    meta = json.loads(subprocess.check_output(
        ["cargo", "metadata", "--locked", "--format-version", "1",
         "--filter-platform", ALVO], cwd=fonte, text=True))
    pacotes = {p["id"]: p for p in meta["packages"]}
    por_nome = {(p["name"], p["version"]): i for i, p in pacotes.items()}
    arvore = subprocess.check_output(
        ["cargo", "tree", "--locked", "-p", "librashader-capi",
         "--no-default-features", "--features", "runtime-opengl",
         "--target", ALVO, "-e", "normal", "--prefix", "none",
         "--format", "{p}"], cwd=fonte, text=True)
    visitados = set()
    for linha in arvore.splitlines():
        partes = linha.split()
        if len(partes) >= 2:
            visitados.add(por_nome[(partes[0], partes[1].lstrip("v"))])
    nomes = {pacotes[i]["name"] for i in visitados}
    if "librashader-runtime-gl" not in nomes or "librashader-runtime-d3d9" in nomes:
        sys.exit("arvore de dependencias nao corresponde a compilacao OpenGL")

    saida.mkdir(parents=True, exist_ok=True)
    resumo, textos, sem_arquivo, problemas = [], {}, [], []
    for pid in sorted(visitados, key=lambda i: (pacotes[i]["name"], pacotes[i]["version"])):
        p = pacotes[pid]
        rotulo = f'{p["name"]} {p["version"]}'
        interno = p["source"] is None
        licenca = p.get("license") or ""
        origem = p.get("repository") or f'https://crates.io/crates/{p["name"]}/{p["version"]}'
        resumo.append(f"- {rotulo}: {licenca or 'ver arquivo de licenca'} — {origem}")
        if not licenca and not p.get("license_file") and not interno:
            problemas.append(f"{rotulo}: sem licenca declarada")
        # O proprio librashader e licenciado MPL-2.0 OR GPL-3.0-only: usamos MPL.
        if licenca and so_copyleft(licenca):
            problemas.append(f"{rotulo}: apenas copyleft forte ({licenca})")
        if interno:
            continue
        base = Path(p["manifest_path"]).parent
        achados = sorted(f for f in base.rglob("*") if f.is_file()
                         and NOMES.match(f.name) and f.suffix.lower() not in CODIGO)
        if p.get("license_file"):
            extra = (base / p["license_file"]).resolve()
            if extra.is_file() and extra not in achados:
                achados.append(extra)
        if not achados:
            autores = ", ".join(p.get("authors") or []) or "ver repositorio"
            sem_arquivo.append(f"- {rotulo} ({licenca}); autores: {autores}; {origem}")
        for arquivo in achados:
            conteudo = arquivo.read_text(encoding="utf-8", errors="replace").replace("\r\n", "\n").strip()
            chave = hashlib.sha256(conteudo.encode()).hexdigest()
            textos.setdefault(chave, (conteudo, []))[1].append(
                f"{rotulo}: {arquivo.relative_to(base).as_posix()}")

    if problemas:
        sys.exit("Licencas que exigem revisao:\n" + "\n".join(problemas))

    with open(saida / "THIRD_PARTY_NOTICES.txt", "w", encoding="utf-8") as f:
        f.write(f"Componentes do librashader {VERSAO} (commit {COMMIT}), alvo {ALVO}.\n")
        f.write("Lista obtida do Cargo.lock oficial (dependencias normais; pode incluir\n")
        f.write("componentes de compilacao como macros, que nao vao para o binario).\n")
        f.write("O binario tambem contem a biblioteca padrao do Rust (MIT OR Apache-2.0,\n")
        f.write("https://github.com/rust-lang/rust) e o CRT estatico da Microsoft.\n\n")
        f.write("\n".join(resumo) + "\n")
        if sem_arquivo:
            f.write("\n== Pacotes publicados sem arquivo de licenca (vale a expressao SPDX;\n")
            f.write("textos-padrao em https://spdx.org/licenses/) ==\n")
            f.write("\n".join(sem_arquivo) + "\n")
            f.write("\nTexto padrao da licenca MIT, aplicavel aos itens MIT acima, com os\n")
            f.write("direitos autorais pertencentes aos respectivos autores:\n\n" + MIT_PADRAO + "\n")
        for conteudo, usos in textos.values():
            f.write("\n" + "=" * 78 + "\n" + "\n".join(usos) + "\n" + "-" * 78 + "\n")
            f.write(conteudo + "\n")

    # Pacotes sem arquivo proprio que permitem Apache-2.0 dependem do texto completo.
    if not any("TERMS AND CONDITIONS FOR USE, REPRODUCTION" in c for c, _ in textos.values()):
        sys.exit("texto completo da Apache-2.0 ausente dos avisos")

    shutil.copyfile(fonte / "LICENSE.md", saida / "librashader-MPL-2.0.md")
    (saida / "LEIA-ME-librashader.txt").write_text(
        f"""Este pacote inclui librashader.dll {VERSAO}, compilada pelo CI do Fofita a partir
do codigo oficial sem modificacoes (somente runtime OpenGL, CRT estatico),
(c) autores do librashader. O projeto oferece "MPL-2.0 OR GPL-3.0-only";
o Fofita o redistribui sob a Mozilla Public License 2.0
(librashader-MPL-2.0.md).

Receita de compilacao: tools/compilar_librashader.sh no repositorio do Fofita.
Codigo-fonte correspondente (MPL-2.0, secao 3.2):
https://github.com/SnowflakePowered/librashader/tree/librashader-v{VERSAO}
commit {COMMIT}

A licenca MPL aplica-se aos arquivos do librashader, nao ao restante do Fofita.
Componentes de terceiros: THIRD_PARTY_NOTICES.txt.
O header librashader.h usado na compilacao e licenciado sob MIT.
""", encoding="utf-8")
    print(f"{len(visitados)} pacotes; {len(textos)} textos de licenca; "
          f"{len(sem_arquivo)} sem arquivo proprio")
    expressoes = sorted({pacotes[i].get("license") or "(arquivo)" for i in visitados})
    print("Expressoes de licenca encontradas:\n  " + "\n  ".join(expressoes))
    # Resumo no log: componentes C/C++ embutidos e crates sob MPL.
    for _, usos in textos.values():
        for uso in usos:
            if uso.startswith(("glslang-sys ", "spirv-cross-sys ", "spirv-to-dxil-sys ")):
                print("embutido:", uso)
    for linha in resumo:
        if "MPL" in linha:
            print("MPL:", linha)
    if sem_arquivo:
        print("Sem arquivo de licenca:\n" + "\n".join(sem_arquivo))


if __name__ == "__main__":
    main()
