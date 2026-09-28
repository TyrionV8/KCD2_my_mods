# Como criamos o mod Instant Prepare para Kingdom Come: Deliverance II

Este texto registra o caminho de investigação e implementação para servir de
modelo ao próximo mod. A versão funcional está em
`/home/davi/repos/KCD2_my_mods/mods/instant-prepare/`. **O teste real feito em
28/09/2026 confirmou a receita 2 na montagem Steam 15693, no Linux/Proton**:

```text
Loaded for Steam assembly 15693 (31 recipes)
Recipe 2: created one quality-4 potion
```

O usuário confirmou que, ao usar **Prepare**, a poção apareceu diretamente e os
ingredientes não foram separados na mesa. Testes posteriores confirmaram o
rendimento de 5 unidades na receita 2 e, após correção dos itens não divisíveis,
na receita 13. O usuário também relatou funcionamento das outras receitas
originais testadas. A versão 0.1.6 incluiu receitas do LDPotions, e o usuário
confirmou que o preparo imediato funcionou com esse mod.

## 1. Traduzir o pedido em uma sequência verificável

O comportamento desejado foi dividido em etapas:

1. Descobrir qual receita está aberta no livro da mesa de alquimia.
2. Ler a lista e as quantidades de ingredientes dessa receita.
3. Conferir no inventário se há ingredientes suficientes, inclusive ervas secas.
4. Descontar os ingredientes e adicionar a quantidade correta de poções da melhor qualidade.
5. Impedir que a ação original espalhe os ingredientes na mesa.
6. Manter a ação original se a receita não for conhecida ou faltarem ingredientes.

Essa decomposição define o ponto de entrada no código do jogo e os resultados
que precisavam ser observados no teste. Um *gancho* (*hook*) é um desvio que
permite executar nosso código quando determinada função do jogo é chamada.

## 2. Fixar a versão do jogo antes de pesquisar endereços

A instalação usada era a edição Steam, montagem **15693**. A biblioteca principal
`WHGame.dll` tinha o resumo SHA-256
`bdf8f9e4a11257a72b64c84700e284c29e4c4ccaf5b8d4bfa7d0b2a7294479f7`.
SHA-256 significa *Secure Hash Algorithm* de 256 bits: um resumo do conteúdo
que ajuda a identificar se o arquivo mudou. A extensão também verifica o
carimbo de compilação, o tamanho em memória e os primeiros bytes das funções
que pretende interceptar. Uma atualização do jogo pode deslocar funções mesmo
que os nomes e menus continuem iguais.

Exemplo de conferência no Linux, executado da pasta do jogo:

```bash
# Calcula o resumo da biblioteca do jogo, sem modificá-la.
sha256sum Bin/Win64MasterMasterSteamPGO/WHGame.dll
```

`sha256sum` lê o arquivo indicado e imprime o resumo seguido do caminho. Um
resumo diferente exige nova análise; não se deve reutilizar endereços internos
às cegas. Também consultamos `whdlversions.json` e o cabeçalho do próprio
arquivo executável do Windows, conhecido como **PE** (*Portable Executable*).

## 3. Encontrar os dados das receitas

`Data/Tables.pak` contém tabelas do jogo. Um `.pak` é um pacote de arquivos;
nesse caso, pode ser lido como arquivo ZIP (formato de arquivo compactado).
`Libs/Tables/minigame/AlchemyRecipe.xml` descreve cada receita, seus
ingredientes e produtos por qualidade. XML (*Extensible Markup Language*) é um
formato textual de dados em elementos e atributos. As tabelas de itens associam
as ervas frescas às variantes secas.

O script `tools/generate_recipes.py` lê essas tabelas e gera
`src/recipes_generated.h`. Para cada receita, ele escolhe o produto cujo campo
`Quality` é 4, a melhor qualidade presente nos dados analisados. Também confere
identificadores, quantidades positivas, ausência de identificadores repetidos
e o limite de quatro ingredientes. Resultaram **31 receitas**.

Para gerar novamente a tabela **somente se você estiver trabalhando com a mesma
montagem**, execute na pasta `mods/instant-prepare`:

```bash
# Lê as tabelas do jogo e reescreve a tabela C de receitas do projeto.
python3 tools/generate_recipes.py \
  "/home/davi/.steam/debian-installation/steamapps/common/KingdomComeDeliverance2/Data/Tables.pak" \
  src/recipes_generated.h
```

`python3` executa o gerador; o primeiro argumento é a origem e o segundo é o
arquivo que será **reescrito**. `GUID` (*Globally Unique Identifier*) é o
identificador em texto usado pelo jogo para diferenciar itens e produtos.

## 4. Descobrir a ação interna de “Prepare”

O texto de ajuda da interface no pacote `Data/IPL_GameData.pak` associa a opção
**Prepare** à ação interna `read_select`. A ação ocorre no contexto
`alchemy_reading`, quando o jogador lê uma receita na mesa. Essa descoberta foi
crucial: interceptar uma função genérica de inventário poderia alterar muitas
outras ações; o par ação/contexto permite tratar somente o uso pretendido.

O projeto aberto [libKCD2](https://github.com/JerryYOJ/libKCD2) ajudou a
identificar estruturas do jogo. Os endereços efetivos e os bytes iniciais das
funções foram conferidos contra o `WHGame.dll` local. Parte dessa análise é
**engenharia reversa**: observar um programa compilado para entender suas
estruturas e chamadas, sem ter o código original do jogo. Essas estruturas são
internas e não prometem estabilidade entre versões.

No arquivo `src/instant_prepare.c`:

- `hook_start` registra a sessão de alquimia ativa.
- `hook_end` encerra esse registro.
- `hook_trigger` observa a ação `read_select`, mas só age se a sessão estiver
  ativa e o contexto for `alchemy_reading`.
- `open_recipe` encontra a receita aberta e procura seus dados na tabela gerada.

Os endereços relativos dentro da biblioteca são chamados **RVA** (*Relative
Virtual Address*, endereço relativo à base carregada em memória). O código soma
esses deslocamentos à base de `WHGame.dll`. É por isso que a checagem da
montagem é obrigatória.

## 5. Fazer a alteração do inventário

O jogo expõe um sistema de scripts Lua. Lua é uma linguagem leve usada pelo
jogo para rotinas de comportamento. A extensão C monta e executa uma pequena
rotina Lua que, para a receita escolhida:

1. conta a versão fresca e, se cadastrada, a versão seca de cada ingrediente;
2. planeja quanto retirar de cada variante, antes de mudar o inventário;
3. cria o produto de qualidade 4;
4. retira exatamente as quantidades planejadas;
5. informa sucesso ou falha para a extensão.

Se a verificação inicial mostra falta de ingredientes, o gancho deixa
`read_select` seguir seu caminho normal. Depois que a rotina tenta alterar o
inventário, o gancho suprime a ação original, evitando que o jogo também
separe ingredientes. Em caso de falha após uma alteração parcial, a rotina tenta
recriar os ingredientes retirados e remover o produto criado. Esse é um
**mecanismo de compensação**, não uma transação atômica garantida pelo jogo.
Por isso o teste em um save separado continua importante.

Ao concluir com sucesso, o código também chama a função interna que limpa os
espaços de ingredientes da mesa e escreve uma linha em `InstantPrepare.log`.
O registro facilita distinguir “o arquivo carregou” de “uma receita foi criada”.

### Correção posterior do rendimento

O primeiro teste revelou que a versão inicial criava sempre uma unidade. Na
versão 0.1.1, o gerador passou a ler `MaxYield` do produto de qualidade 4 em
`AlchemyRecipe.xml`. A rotina Lua consultava `player.soul:HasPerk` para os
talentos **Secrets of Matter I** e **Secrets of Matter II**. O teste no jogo
entregou 3 unidades da receita 2 apesar de **Secrets of Matter II** estar
adquirido; portanto, essa consulta não refletiu o bônus nesse cenário.

Na versão 0.1.2, a rotina passou a consultar `player.soul:GetDerivedStat('ala')`,
o bônus de rendimento usado pelos efeitos dos talentos. O teste no jogo
retornou zero mesmo com o talento II adquirido. A tabela
`buff__perk.xml` da montagem analisada registra `ala+1` para o primeiro talento
e `ala+2` para o segundo. A receita 2 tem rendimento básico 3; com apenas o
segundo talento, o resultado esperado é 5. A versão 0.1.3 compara também
`g_localActor.soul`, experimenta as consultas por identificador e nome, e
registra nível de alquimia, bônus e resultado de talento de cada referência.
O teste confirmou novamente 3 unidades: bônus zero nas consultas por script,
mesmo com o talento II na tela do personagem e cinco unidades na produção manual.

A versão 0.1.4 usa a função nativa `C_Soul::GetPerkStatModifier` para ler o
modificador `ala` (identificador 73) do personagem dono da mesa. O código
confere se a função encontrada na tabela virtual do personagem aponta para o
endereço verificado na biblioteca local do jogo. Se não encontrar o personagem,
a função ou um valor inteiro plausível, deixa a ação original **Prepare** seguir.
Essa versão compilou e passou nos testes simulados de quantidade e inventário.
No teste dentro do jogo, a receita 2 criou 5 poções de qualidade 4 duas vezes;
o registro mostrou `native yield bonus 2` nas duas produções. Isso confirma
essa leitura nativa para o talento **Secrets of Matter II** no save testado.

O teste posterior de outras receitas revelou falha na receita 13, **Saviour
Schnapps**. A tabela `item__alchemy.xml` marca esse produto como não divisível.
A versão 0.1.5 consulta esse atributo ao gerar a tabela e cria cada unidade de
produtos não divisíveis separadamente. A rotina também registra a etapa da
falha para facilitar o diagnóstico caso a mudança não resolva o problema.
Essa alteração passou na simulação de inventário. No teste dentro do jogo, a
receita 13 criou 5 poções de qualidade 4 com bônus nativo 2.

### Receitas adicionadas pelo LDPotions

O usuário relatou que receitas de mods ainda seguiam o preparo manual. A tabela
gerada até a versão 0.1.5 continha somente as 31 receitas do jogo. Entre os
pacotes instalados, `LDPotions.pak` contém `AlchemyRecipe__ldpotions.xml` com
16 receitas e `item__ldpotions.xml` com os produtos. Os 16 livros de receita
usam os mesmos identificadores, de 157 a 173, exceto 167. A versão 0.1.6
permite passar esse pacote ao gerador por `--recipe-mod` e resulta em 47 receitas.
O pacote do LDPotions permanece uma dependência separada. O usuário confirmou
o funcionamento da versão 0.1.6 com receitas desse mod dentro do jogo, sem
fornecer um registro de teste individual das 16 receitas.

## 6. Compilar uma extensão Windows no Linux

Como não havia Windows disponível, usamos **compilação cruzada**: um compilador
executado no Linux produz uma biblioteca que roda como código Windows de 64
bits. `build-linux.sh` compila `src/instant_prepare.c` junto do
[MinHook](https://github.com/TsudaKageyu/minhook), biblioteca pequena que
implementa os desvios de funções. O pacote usa
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) para
carregar `InstantPrepare.asi` quando o jogo inicia.

Na pasta `mods/instant-prepare`, com `x86_64-w64-mingw32-gcc-posix` disponível:

```bash
# Produz dist/InstantPrepare.asi para Windows de 64 bits.
./build-linux.sh

# Monta os pacotes de instalação e código-fonte, após conferir os binários.
python3 tools/package.py

# Confere se todos os arquivos do pacote de instalação podem ser lidos.
unzip -t dist/InstantPrepare-15693-mod.zip
```

`./build-linux.sh` executa o script local de compilação. `python3` executa o
empacotador. `unzip -t` apenas testa a integridade do arquivo ZIP; não o extrai.
O empacotador exige `dist/dinput8.dll` na revisão prevista e verifica se os
dois binários são Windows de 64 bits. O pacote de instalação preserva os
caminhos sob `Bin/Win64MasterMasterSteamPGO` e inclui avisos de licença.

No ambiente da criação, o compilador foi baixado para uma pasta temporária em
`/tmp`, sem instalação permanente. Esse caminho temporário pode desaparecer;
para trabalhos futuros, instale um compilador cruzado equivalente ou ajuste a
variável `CC` (*C Compiler*, caminho do compilador C) ao chamar o script.

## 7. Testes em camadas

Os testes foram separados porque cada um responde a uma pergunta diferente:

| Teste | O que comprovou |
| --- | --- |
| Leitura das tabelas | 31 receitas identificadas; produto de qualidade 4 presente |
| Compilação com avisos tratados como erros | Código C aceito pelo compilador e biblioteca gerada |
| Rotina Lua com inventário simulado | Planejamento de ingredientes, caso sem estoque e tentativa de compensação |
| Wine com programa pequeno | Carregador encontra a extensão e a proteção rejeita uma biblioteca de jogo diferente |
| Teste do usuário no jogo | A receita 2 criou uma poção de qualidade 4 por Prepare |

**Wine** é a camada que executa programas Windows no Linux; **Proton** a utiliza
como parte do ambiente da Steam. Testes isolados não provam o resultado no
jogo. A confirmação do usuário fechou esse ponto para a receita 2 e montagem
15693. O registro foi:

```text
Loaded for Steam assembly 15693 (31 recipes)
Recipe 2: created one quality-4 potion
```

## 8. Modelo para o próximo mod

Para repetir o método, defina primeiro uma ação pequena e observável; registre
a montagem e o resumo da biblioteca do jogo; encontre os dados relevantes nos
pacotes e a ação interna da interface; altere apenas o contexto necessário;
adicione checagens de versão e registro; compile; teste o carregamento em
isolamento; por fim valide o comportamento dentro do jogo com save de teste.
Anote separadamente o que foi observado no jogo e o que ainda é apenas
inferência técnica.

O código está sob a GNU General Public License versão 3; as dependências têm
licenças próprias no projeto. Para publicar, consulte
`KCD2_INSTANT_PREPARE_PUBLICACAO.md` em `codex-work` ou `docs/PUBLICACAO.md` no
repositório.
