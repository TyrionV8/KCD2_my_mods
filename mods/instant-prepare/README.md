# Instant Prepare — Kingdom Come: Deliverance II

**Estado:** a versão 0.1.0 foi testada no jogo Steam de montagem **15693** em
Linux com Proton em 28/09/2026. A receita 2 produziu uma poção de qualidade 4;
o registro mostrou `Recipe 2: created one quality-4 potion`. A versão 0.1.1
passou a usar o rendimento básico da receita, mas o teste com **Secrets of
Matter II** entregou 3, quando deveria entregar 5. A versão 0.1.2 também
entregou 3: o bônus consultado no script apareceu como zero. A versão 0.1.3
também entregou 3, apesar do talento aparecer na tela do personagem. A versão
0.1.4 consulta o modificador diretamente pela função nativa do jogo. O usuário
confirmou duas produções de 5 poções da receita 2, ambas registradas com bônus
nativo 2. O resultado corresponde à produção manual observada no mesmo save.
As outras 30 receitas foram mapeadas a partir dos dados dessa montagem, mas
ainda não foram testadas uma a uma dentro do jogo.

Ao usar **Prepare** no livro da mesa de alquimia, o mod lê a receita aberta,
retira os ingredientes do inventário e adiciona imediatamente **poções da
maior qualidade cadastrada para essa receita (nível 4)**. A quantidade parte
do `MaxYield` dessa qualidade nos dados do jogo e soma o bônus efetivo de
rendimento da alquimia. **Secrets of Matter I** acrescenta +1 e **Secrets of
Matter II** acrescenta +2. Por exemplo,
uma receita com rendimento básico 3 entrega 5 unidades com o segundo talento
e sem o primeiro; com ambos, entrega 6. O nível de alquimia permite adquirir
esses talentos, mas a quantidade depende do bônus efetivo do personagem. Os
ingredientes não são colocados sobre a mesa. O mod pode usar a variante seca de uma erva quando
ela existir. Se faltarem ingredientes ou a receita não estiver entre as 31
mapeadas, o jogo executa a ação original.

## Estrutura do projeto

| Caminho | Conteúdo |
| --- | --- |
| `dist/InstantPrepare-15693-mod.zip` | Pacote de instalação manual, extraído na raiz do jogo |
| `dist/InstantPrepare-15693-source.zip` | Código-fonte e licenças para distribuição |
| `src/instant_prepare.c` | Extensão nativa que intercepta a ação Prepare |
| `src/recipes_generated.h` | Identificadores das 31 receitas, poções de qualidade 4 e rendimento básico |
| `tools/generate_recipes.py` | Gera a tabela de receitas a partir de `Data/Tables.pak` |
| `build-linux.sh` | Compila a extensão Windows de 64 bits no Linux |
| `tools/package.py` | Recria e confere os dois arquivos compactados |
| `third_party/` | Código MinHook e licenças das dependências |

`.asi` é uma biblioteca dinâmica do Windows com uma extensão de nome usada por
carregadores de mods. `dinput8.dll` é a biblioteca dinâmica do **Ultimate ASI
Loader** que carrega `InstantPrepare.asi`. O pacote não é um mod `.pak` da pasta
`Mods` do jogo. [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)
e [MinHook](https://github.com/TsudaKageyu/minhook) são projetos independentes.

## Instalação manual

1. Feche o jogo e faça cópia dos seus saves. Localize a **raiz do jogo**, a pasta
   `KingdomComeDeliverance2` que contém `Bin` e `Data`.
2. Confira se já existe `Bin/Win64MasterMasterSteamPGO/dinput8.dll` ou
   `InstantPrepare.asi`. Se existir, identifique qual mod o utiliza e não
   substitua o arquivo sem resolver a compatibilidade.
3. Extraia `dist/InstantPrepare-15693-mod.zip` **na raiz do jogo**. A estrutura
   resultante deve ser:

   ```text
   KingdomComeDeliverance2/
   └── Bin/
       └── Win64MasterMasterSteamPGO/
           ├── dinput8.dll
           └── InstantPrepare.asi
   ```

4. No Linux, abra **Steam → jogo → Propriedades → Geral → Opções de inicialização**
   e acrescente, preservando as opções que já utiliza:

   ```text
   WINEDLLOVERRIDES="dinput8=n,b" %command%
   ```

   Proton é a camada de compatibilidade que executa jogos Windows no Linux.
   Essa opção pede que ele use primeiro a biblioteca local (`n`, *native*) e,
   caso necessário, a biblioteca interna (`b`, *builtin*). `%command%` é o
   espaço reservado da Steam para o comando que inicia o jogo. No Windows,
   essa opção do Proton não se aplica; **esta compilação ainda não foi testada
   em uma instalação Windows**.

5. Inicie o jogo, abra uma receita conhecida com ingredientes suficientes e use
   **Prepare**. Confira a contagem dos ingredientes, a nova poção e a ausência
   de ingredientes separados sobre a mesa.

O registro textual `InstantPrepare.log` aparece ao lado de `InstantPrepare.asi`.
Na montagem testada, ele começa com `Loaded for Steam assembly 15693 (31 recipes)`
e registra cada receita criada. Se aparecer `Unsupported WHGame.dll build`, a
versão do jogo mudou e a extensão não instala suas interceptações. Ela verifica
o cabeçalho da biblioteca do jogo e os bytes iniciais das funções antes de
modificá-las em memória.

### Compatibilidade e limites

- **Somente a montagem Steam 15693 foi validada.** Atualizações do jogo podem
  mudar endereços internos. Não anuncie compatibilidade com outra montagem sem
  conferir os endereços, recompilar e testar no jogo.
- Os itens são criados diretamente no inventário. Experiência de alquimia,
  estatísticas, conquistas e progresso de missões ligados à fabricação normal
  podem não ser concedidos. A quantidade considera o rendimento básico e o
  modificador ativo de rendimento `ala` do personagem. Essa leitura nativa foi
  confirmada para a receita 2 com **Secrets of Matter II**; modificações que não alterem esse bônus não
  são incluídas.
- Em falha na transação de inventário, o mod tenta desfazer as alterações e
  registra o erro. Como a interface interna do jogo não oferece uma transação
  atômica garantida, teste primeiro em um salvamento descartável.
- O arquivo compactado exige **instalação manual na raiz do jogo**. Um gerenciador
  que só instala na pasta `Mods` não posicionará esta extensão corretamente.
- Assinar um item na Oficina Steam não instala bibliotecas em `Bin`. Este pacote
  não deve ser anunciado como instalável pela Oficina sem uma adaptação ao
  formato oficial de mods.

### Desativação

Com o jogo fechado, retire `InstantPrepare.asi` de `Bin/Win64MasterMasterSteamPGO`.
Retire `dinput8.dll` apenas se nenhum outro mod depender dele. Os arquivos
originais do jogo não são substituídos pelo pacote; poções já gravadas no save
permanecem nele.

## Como funciona

O código instala uma interceptação (*hook*, desvio temporário de uma função em
memória) na ação interna `read_select` do contexto `alchemy_reading`. Ele lê a
receita aberta, procura seu identificador na tabela gerada e monta uma pequena
rotina Lua, linguagem de script disponível no jogo. A rotina confere as
quantidades frescas/secas e cria a quantidade de
poções de nível 4 correspondente, desconta os ingredientes e
tenta desfazer a operação se algo falhar. A ação original **Prepare** só é
suprimida quando a transação foi tentada; com ingredientes insuficientes, ela
continua funcionando. O código nativo consulta o modificador ativo de rendimento
no personagem antes de executar essa rotina. Se não conseguir lê-lo com segurança,
deixa a ação original do jogo funcionar. O guia de construção está em `docs/COMO_FOI_FEITO.md` na
raiz do repositório.

## Recompilação no Linux

É necessário `x86_64-w64-mingw32-gcc-posix`, compilador cruzado que gera
bibliotecas Windows de 64 bits a partir do Linux. Na pasta deste projeto:

```bash
# Compila o código C e o MinHook; escreve dist/InstantPrepare.asi.
./build-linux.sh

# Empacota os binários já presentes em dist/ e confere o formato e a integridade.
python3 tools/package.py
```

`./` executa o script desta pasta; `python3` executa o empacotador. O segundo
comando precisa de `dist/dinput8.dll`, edição de 64 bits do Ultimate ASI Loader
9.7.4. Ele verifica o resumo SHA-256 (*Secure Hash Algorithm* de 256 bits)
esperado antes de embalar. O script `install-linux.sh` instala somente quando
o `WHGame.dll` local corresponde exatamente à montagem usada no desenvolvimento
e quando os nomes de destino ainda não existem; é uma conveniência para este
ambiente, não um instalador genérico para qualquer versão do jogo.

## Código-fonte, créditos e licença

O código próprio é distribuído sob a **GNU General Public License versão 3**
([LICENSE](LICENSE)). A tabela contém identificadores e quantidades das receitas
extraídos do `Data/Tables.pak` da montagem analisada, sem copiar texturas,
áudios ou modelos do jogo. Os endereços foram investigados com auxílio do
[libKCD2](https://github.com/JerryYOJ/libKCD2) e conferidos na biblioteca local
do jogo. A implementação foi produzida com assistência de inteligência
artificial e validada pelo usuário no jogo; informe isso ao publicar conforme
as regras da plataforma.

- MinHook: revisão `8af6b4acae5a9388fd742b56fa79ece89d96f823`; licença em
  `third_party/minhook/LICENSE.txt`.
- Ultimate ASI Loader: edição 9.7.4 de 64 bits; licença em
  `third_party/Ultimate-ASI-Loader-LICENSE`.
