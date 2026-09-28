# Guia de publicação do Instant Prepare

**Atualizado em 28/09/2026.** O pacote funcional foi testado em
*Kingdom Come: Deliverance II*, edição Steam, montagem 15693, no Linux com
Proton. A publicação nas plataformas ainda **não foi feita**. Antes de seguir
uma interface que possa ter mudado, confira os links oficiais ao fim deste guia.
O pacote atual inclui a correção 0.1.5 para produtos não divisíveis. A receita
13 criou 5 poções no teste dentro do jogo.

## O que está pronto para distribuir

Na pasta `/home/davi/repos/KCD2_my_mods/mods/instant-prepare/dist/`:

- `InstantPrepare-15693-mod.zip`: arquivo principal. Contém
  `Bin/Win64MasterMasterSteamPGO/InstantPrepare.asi`, o carregador
  `dinput8.dll`, instruções e licenças.
- `InstantPrepare-15693-source.zip`: código-fonte e licenças, para revisão e
  cumprimento da licença do código próprio.

`.asi` é uma extensão binária do Windows; `dinput8.dll` é a biblioteca dinâmica
que a carrega. Esse pacote deve ser extraído na **raiz do jogo**, onde estão as
pastas `Bin` e `Data`. Ele **não** pertence à pasta `Mods` e ainda não foi testado
em Windows. O código cobre 31 receitas da montagem indicada. O usuário confirmou
o resultado das receitas 2 e 13 pelo registro e relatou funcionamento das outras
receitas originais testadas. Essas restrições devem estar
visíveis na página pública.

Antes de abrir qualquer página de publicação, confira o pacote sem alterá-lo.
Execute **na pasta** `/home/davi/repos/KCD2_my_mods/mods/instant-prepare`:

```bash
# Testa se cada arquivo do pacote de instalação pode ser lido.
unzip -t dist/InstantPrepare-15693-mod.zip

# Lista os caminhos que serão extraídos pelo usuário.
unzip -Z -1 dist/InstantPrepare-15693-mod.zip

# Calcula resumos para identificar exatamente os arquivos enviados.
sha256sum dist/InstantPrepare-15693-mod.zip dist/InstantPrepare-15693-source.zip
```

`unzip -t` testa a integridade sem extrair. `unzip -Z -1` lista nomes de
arquivos, sem instalar. `sha256sum` imprime resumos SHA-256 (*Secure Hash
Algorithm* de 256 bits) que permitem comparar o arquivo local ao publicado.
Não inclua saves, `InstantPrepare.log`, caminhos da sua conta Steam ou arquivos
originais do jogo no envio.

## Publicar no Nexus Mods

O [Nexus Mods de Kingdom Come: Deliverance II](https://www.nexusmods.com/games/kingdomcomedeliverance2/mods)
aceita páginas de mods para esse jogo. O [guia de autores](https://help.nexusmods.com/article/136-best-practices-for-mod-authors)
recomenda instruções de instalação, versão, requisitos e descrição claros.
A [política de envio](https://help.nexusmods.com/article/28-file-submission-guidelines)
exige atribuição correta, descrição verdadeira e marcação de conteúdo criado
com inteligência artificial.

1. Entre na sua conta Nexus Mods e, na página do jogo, use a opção de adicionar
   um mod. O nome exato do botão pode mudar com a interface.
2. Use um título claro, por exemplo **Instant Prepare — One-Click Best Quality
   Alchemy (Steam build 15693)**. Versão testada: `0.1.5`. Categoria sugerida:
   jogabilidade ou alquimia, conforme as opções disponíveis.
3. Envie `InstantPrepare-15693-mod.zip` como **arquivo principal**. Se desejar,
   envie `InstantPrepare-15693-source.zip` como arquivo adicional de código.
4. Na descrição, explique: a ação **Prepare** consome ingredientes e produz
   poções de qualidade 4 diretamente, conforme o rendimento da receita e o
   bônus ativo do personagem; há 31 receitas mapeadas; as versões 0.1.1 a 0.1.3
   ainda produziam apenas 3 com Secrets of Matter II, e a versão 0.1.4 produziu
   5 em duas tentativas da receita 2; a versão 0.1.5 corrigiu a criação da
   receita 13, que também produziu 5 unidades;
   instalação manual na raiz; `dinput8.dll` não deve
   substituir um carregador já existente sem análise; no Proton é preciso
   `WINEDLLOVERRIDES="dinput8=n,b" %command%`; Windows ainda não foi testado.
5. Liste os limites: compatibilidade exclusiva com a montagem Steam 15693;
   experiência, estatísticas, conquistas e progresso de missões da alquimia
   podem não ser contabilizados; modificações que não afetem o bônus de
   rendimento `ala` não são incluídas.
6. Informe créditos e licenças: código próprio sob GNU General Public License
   versão 3; MinHook e Ultimate ASI Loader com as licenças incluídas no pacote.
   A implementação deste mod foi majoritariamente produzida com auxílio de
   inteligência artificial. A política atual do Nexus indica a marcação
   **AI-Generated Content** para código gerado dessa forma; selecione essa
   marcação e descreva a validação feita no jogo. Não marque apenas
   **AI Assisted** sem a evidência de desenvolvimento humano exigida pela
   política.
7. Revise a prévia da página, arquivos e permissões de redistribuição. Publique
   quando o conteúdo estiver correto. Depois, baixe o arquivo publicado e
   compare seu resumo com o arquivo local antes de divulgar o link.

Texto curto sugerido para a descrição:

> On Steam build 15693, using Prepare at an alchemy bench directly consumes the
> recipe ingredients and grants quality-4 potions using the recipe's maximum
> yield, plus the character's active alchemy yield bonus. Supports 31 mapped
> base recipes. Manual game-root installation required. Earlier versions produced
> the base yield despite Secrets of Matter II; this correction still needs an
> in-game test. Windows and other game builds are untested. Normal alchemy
> experience and quest/stat events may not fire.

Uma imagem de capa pode mostrar o livro aberto e a poção no inventário. Use uma
captura real do jogo e não apresente receitas ou plataformas ainda não testadas
como verificadas. Um mod binário pode passar por análise de segurança da
plataforma; manter o código-fonte e as licenças disponíveis ajuda usuários a
inspecionarem o pacote.

## Oficina Steam: situação deste pacote

A [Oficina Steam do jogo](https://steamcommunity.com/workshop/browse/?appid=1771300)
existe. Contudo, o **Instant Prepare atual não é um pacote oficial da Oficina**.
A documentação de publicação da Warhorse descreve uma pasta de mod com
`mod.manifest` e dados empacotados em `.pak`, enviada pelo
`SteamWorkshopUploader`. Nosso pacote coloca bibliotecas dinâmicas em
`Bin/Win64MasterMasterSteamPGO`. Pela forma como o carregador ASI procura
extensões e como o jogo recebe itens da Oficina, **é uma inferência técnica que
assinar o pacote atual na Oficina não instalará nem ativará o mod**. Não crie
uma página pública na Oficina anunciando este ZIP como funcional por assinatura.

Para oferecer este mesmo comportamento na Oficina, o caminho responsável é:

1. Pesquisar se as ferramentas oficiais permitem interceptar a ação **Prepare**
   e executar a alteração do inventário dentro de um mod `.pak`. Talvez seja
   necessário redesenhar a implementação; a conversão ainda não foi feita.
2. Criar uma pasta com `mod.manifest` e os arquivos de dados/scripts aceitos pelo
   jogo, seguindo a [documentação oficial da Warhorse](https://warhorse.youtrack.cloud/articles/KM-A-58).
3. Testar essa versão no jogo instalado **somente por assinatura**, sem
   `InstantPrepare.asi` e sem `dinput8.dll`, para provar que o pacote da Oficina
   funciona sozinho.
4. Só depois usar o `SteamWorkshopUploader` da ferramenta oficial de modding.
   O [guia da Warhorse para a Oficina](https://warhorse.youtrack.cloud/articles/KM-A-85)
   indica selecionar a pasta que contém `mod.manifest`, criar o item no botão
   `+`, fornecer uma capa de até 1 megabyte, aceitar os termos da Oficina e
   revisar o item inicialmente oculto antes de torná-lo público. A
   [documentação Steamworks](https://partner.steamgames.com/doc/features/workshop/implementation)
   explica o fluxo de criação e atualização de itens.

As ferramentas oficiais de upload são fornecidas como programa Windows. Você
informou que não tem Windows disponível; rodá-las pelo Proton/Wine no Linux
seria um experimento separado e **não foi validado**. Esse é um segundo motivo
para tratar a publicação na Oficina como trabalho futuro. O pacote Nexus pode
ser publicado independentemente, desde que a página exponha os limites acima.

## Fontes para conferir antes de publicar

- [Nexus Mods: boas práticas para autores](https://help.nexusmods.com/article/136-best-practices-for-mod-authors)
- [Nexus Mods: regras de envio e marcação de inteligência artificial](https://help.nexusmods.com/article/28-file-submission-guidelines)
- [Warhorse: publicação de mods](https://warhorse.youtrack.cloud/articles/KM-A-58)
- [Warhorse: publicação na Oficina Steam](https://warhorse.youtrack.cloud/articles/KM-A-85)
- [Steamworks: criação e atualização de itens da Oficina](https://partner.steamgames.com/doc/features/workshop/implementation)
