# Meus mods para Kingdom Come: Deliverance II

Este repositório reúne mods, pacotes de instalação e anotações de desenvolvimento.

## Mod funcional

| Projeto | Estado | Instalação |
| --- | --- | --- |
| [Instant Prepare](mods/instant-prepare/README.md) | Versão 0.1.6 confirmada pelo usuário com receitas do LDPotions no Steam, montagem 15693, Linux/Proton | [Pacote de instalação](mods/instant-prepare/dist/InstantPrepare-15693-mod.zip) extraído na raiz do jogo |

**Instant Prepare** substitui a opção **Prepare** da mesa de alquimia: desconta
os ingredientes e entrega diretamente poções de qualidade 4. A quantidade usa
o rendimento da receita e os talentos de alquimia do personagem. O
[README do projeto](mods/instant-prepare/README.md) explica instalação,
funcionamento, compatibilidade, limites, recompilação e licenças.

## Documentação

- [Como o mod foi feito](docs/COMO_FOI_FEITO.md): investigação, implementação,
  compilação e testes para ajudar no próximo mod.
- [Publicação no Nexus Mods e na Oficina Steam](docs/PUBLICACAO.md): passos de
  envio e o limite atual do pacote nativo na Oficina.
- [Código-fonte compactado](mods/instant-prepare/dist/InstantPrepare-15693-source.zip)
  para revisão e distribuição junto da licença.

## Experimento anterior

`experiments/kcse-prototype/` e seu arquivo compactado são o primeiro protótipo
com *Kingdom Come Script Extender*. **Não foram compilados nem testados no jogo**.
O mod funcional é somente o projeto em `mods/instant-prepare/`. O protótipo foi
mantido como registro da investigação; não o envie como versão instalável.

Os saves pessoais do jogo permanecem em `codex-work/backups`, fora deste
repositório. Não os adicione a versões públicas.
