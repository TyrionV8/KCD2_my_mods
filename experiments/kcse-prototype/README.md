# Instant Prepare — Kingdom Come: Deliverance II

Ao manter **Prepare** no livro de receitas da mesa de alquimia, o mod consulta a
receita aberta, desconta as quantidades indicadas de ingredientes do inventário e
cria **uma poção do maior nível de qualidade definido nessa receita**. A ação
normal de separar os ingredientes é substituída. Ervas secas podem atender à
receita quando houver uma variante seca cadastrada no jogo. Sem receita válida
ou sem ingredientes suficientes, a ação original continua disponível.

O código usa um *gancho* (interceptação de uma função do jogo) na ação interna
`read_select`. Por isso é uma extensão nativa e requer **KCSE** (*Kingdom Come
Script Extender*, carregador de extensões), além da biblioteca de endereços
compatível. Não há arquivo `.pak` necessário.

## Compatibilidade e estado

- Projeto feito para a edição **Steam** com montagem **15693**, observada na
  instalação local e correspondente à versão 1.5.6 usada por `libKCD2` e KCSE.
- O código-fonte e o script de compilação estão prontos. **Ainda não houve
  compilação no Windows nem teste dentro do jogo.** Não instale como se fosse
  um mod pronto até gerar o pacote e verificar seu funcionamento.
- O mod concede o item diretamente. Ele não executa a sequência normal da
  alquimia: experiência, estatísticas de poções preparadas, conquistas e
  progresso de missões associados à fabricação normal podem não ser concedidos.
  A quantidade é sempre **uma poção** por uso de Prepare.
- Como toda extensão que acessa estruturas internas do jogo, uma atualização
  pode exigir novas bibliotecas e novos endereços. Não use esta compilação em
  outra montagem do jogo.

## Compilar e criar o pacote no Windows

Requisitos: Windows com **Visual Studio 2022** e o componente *Desktop
development with C++*, **Git**, **CMake** e acesso à internet. A compilação
cria uma **DLL** (*Dynamic Link Library*, biblioteca carregada pelo jogo).

1. Copie ou extraia esta pasta em um diretório gravável no Windows.
2. Abra o PowerShell nessa pasta e execute:

   ```powershell
   # Usa a política de execução somente para este processo do PowerShell.
   powershell -NoProfile -ExecutionPolicy Bypass -File .\build-windows.ps1
   ```

O script baixa revisões fixas de `libKCD2` e KCSE para `_build/`, instala as
dependências de compilação com `vcpkg`, compila as duas DLLs e baixa a biblioteca
de endereços da edição Steam 15693. Ele confere o hash SHA-256 (*Secure Hash
Algorithm 256-bit*, resumo usado para verificar a integridade do arquivo) da
biblioteca. Escreve apenas dentro desta pasta. O resultado esperado é:

```text
dist/InstantPrepare-1.5.6-steam.zip
dist/KingdomComeDeliverance2/Mods/InstantPrepare/KCSE/Plugins/InstantPrepare.dll
dist/KingdomComeDeliverance2/Bin/Win64MasterMasterSteamPGO/dinput8.dll
dist/KingdomComeDeliverance2/KCSE/addresslib/kcd_addresslib_steam_release_1_5-15693.bin
```

Para conferir o pacote no PowerShell:

```powershell
# Lista os três arquivos essenciais sem alterar o pacote.
Get-ChildItem .\dist\KingdomComeDeliverance2\Mods\InstantPrepare\KCSE\Plugins\InstantPrepare.dll, `
    .\dist\KingdomComeDeliverance2\Bin\Win64MasterMasterSteamPGO\dinput8.dll, `
    .\dist\KingdomComeDeliverance2\KCSE\addresslib\kcd_addresslib_steam_release_1_5-15693.bin
```

O arquivo ZIP contém a pasta `KingdomComeDeliverance2`, para ser extraída no
diretório `steamapps/common` da instalação Steam. Antes de copiar, verifique se
já existe `dinput8.dll` no diretório `Bin/Win64MasterMasterSteamPGO`: esse nome
também é usado por outros carregadores de mods. Preserve qualquer arquivo
existente e não o substitua às cegas. No Linux/Proton, pode ser necessário usar
`WINEDLLOVERRIDES="dinput8=n,b" %command%` nas opções de inicialização da Steam
para carregar a DLL local.

## Teste dentro do jogo

Use um salvamento de teste com uma receita conhecida e ingredientes suficientes.
Anote as quantidades no inventário. Abra a receita no livro da mesa e mantenha
**Prepare**. O esperado é receber uma poção do maior nível de qualidade e ver
os ingredientes da receita descontados, sem eles aparecerem separados na mesa.
Se a extensão não carregar, procure a mensagem `[InstantPrepare] loaded` no log
do jogo/KCSE. Ao encontrar falha, retire `InstantPrepare.dll` e compare o
comportamento com a ação original antes de retomar o teste.

## Licença e fontes

Este código é distribuído sob GPLv3, a **GNU General Public License versão 3**,
compatível com a licença de `libKCD2` e KCSE. Consulte [LICENSE](LICENSE).

- `libKCD2`: <https://github.com/JerryYOJ/libKCD2> — revisão
  `10d20f28faba462c4bf98a01abb48225cc51bb91`.
- KCSE: <https://github.com/JerryYOJ/KCSE-for-kcd2> — revisão
  `47ff1bd75c4df4a3a2145e7ba9908d5b2e7f7aec`.
- Biblioteca de endereços: <https://github.com/JerryYOJ/Address-Library-For-KCSE> —
  revisão `0294d829acc68867c8df2e6bbc5770e415161cc4`.
