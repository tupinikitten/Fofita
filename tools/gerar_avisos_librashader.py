#!/usr/bin/env python3
"""Gera os avisos de licenca do binario librashader distribuido pelo Fofita.

Percorre, com `cargo metadata --locked`, as dependencias de execucao do
librashader-capi para Windows x64 (a partir do Cargo.lock da tag fixada) e
copia os textos de licenca presentes em cada pacote, incluindo codigo C/C++
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
    nos = {n["id"]: n for n in meta["resolve"]["nodes"]}
    raiz = next(i for i, p in pacotes.items() if p["name"] == "librashader-capi")

    visitados, fila = set(), [raiz]
    while fila:
        atual = fila.pop()
        if atual in visitados:
            continue
        visitados.add(atual)
        for dep in nos[atual]["deps"]:
            # kind None = dependencia normal; ignora build e dev.
            if any(k["kind"] is None for k in dep["dep_kinds"]):
                fila.append(dep["pkg"])

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
        f.write("componentes de compilacao como macros, que nao vao para o binario).\n\n")
        f.write("\n".join(resumo) + "\n")
        if sem_arquivo:
            f.write("\n== Pacotes publicados sem arquivo de licenca (vale a expressao SPDX;\n")
            f.write("textos-padrao em https://spdx.org/licenses/) ==\n")
            f.write("\n".join(sem_arquivo) + "\n")
        for conteudo, usos in textos.values():
            f.write("\n" + "=" * 78 + "\n" + "\n".join(usos) + "\n" + "-" * 78 + "\n")
            f.write(conteudo + "\n")

    shutil.copyfile(fonte / "LICENSE.md", saida / "librashader-MPL-2.0.md")
    (saida / "LEIA-ME-librashader.txt").write_text(
        f"""Este pacote inclui librashader.dll {VERSAO}, binario oficial sem modificacoes,
(c) autores do librashader. O projeto oferece "MPL-2.0 OR GPL-3.0-only";
o Fofita o redistribui sob a Mozilla Public License 2.0
(librashader-MPL-2.0.md).

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
    if sem_arquivo:
        print("Sem arquivo de licenca:\n" + "\n".join(sem_arquivo))


if __name__ == "__main__":
    main()
