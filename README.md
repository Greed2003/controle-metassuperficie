# Controle de metassuperfície

Projeto para desenvolver um controlador de até 32 colunas de uma
metassuperfície, utilizando o microcontrolador ATmega2560.

O firmware será desenvolvido em C/C++.

## Escopo planejado

- Controlar 32 canais por meio de relés.
- Receber comandos pela comunicação serial.
- Selecionar modos de operação e padrões de ativação.
- Testar a lógica de controle no computador antes da integração ao hardware.

O desenvolvimento será realizado em etapas, com versionamento Git
e uso de Docker para compilação e testes.

## Ambiente de desenvolvimento

Requer Docker Desktop em execução com contêineres Linux e Docker Compose.

Para construir a imagem:

```powershell
docker compose build
```

Para verificar as ferramentas no ambiente:

```powershell
docker compose run --rm dev g++ --version
docker compose run --rm dev cmake --version
```

A pasta do projeto é compartilhada em `/workspace` dentro do contêiner.

## Compilação e testes

A implementação atual valida modos de 1 a 32 e gerencia o estado
lógico dos 32 canais por meio da classe `ControllerState`.

Regras implementadas:

- A inicialização mantém todas as saídas em 0, sem modo selecionado.
- O valor interno 0 indica ausência de seleção; não é um modo válido.
- Toda seleção válida de modo zera as saídas, inclusive ao repetir o modo atual.
- No modo N, somente os canais de 1 a N podem ser ativados.
- Modos inválidos e combinações de saídas incompatíveis preservam o estado anterior.

As saídas são representadas por uma máscara de 32 bits:
o bit 0 corresponde ao canal 1 e o bit 31 corresponde ao canal 32.

Os testes verificam a validação dos modos, a inicialização,
as mudanças de estado e os limites dos canais permitidos.

Execute os comandos na pasta do projeto:

```powershell
docker compose run --rm dev cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
docker compose run --rm dev cmake --build build --parallel
docker compose run --rm dev ctest --test-dir build --output-on-failure
```

Os arquivos gerados pela compilação ficam na pasta build.

## VS Code com Docker

Requer Docker Desktop em execução e a extensão Dev Containers no VS Code.

Na raiz do projeto, execute no PowerShell:

```powershell
$projectPath = (Get-Location).Path
$projectHex = -join ([System.Text.Encoding]::UTF8.GetBytes($projectPath) | ForEach-Object { $_.ToString("x2") })
code --new-window --folder-uri "vscode-remote://dev-container+$projectHex/workspace"
```

A nova janela utiliza o compilador e os cabeçalhos do container.
Os comandos de Git e Docker continuam no PowerShell do Windows.

## Integração contínua com Jenkins

O Jenkins executa a compilação e os testes em um agente Docker
identificado como `metasurface-cpp`, com um executor.
O controlador possui zero executores.

Arquivos de configuração:

- `compose.jenkins.yaml`: controlador, agente e volumes persistentes.
- `ci/Dockerfile.agent`: ambiente de compilação e testes do agente.
- `ci/configure-agent.groovy`: cadastro do agente pela CLI administrativa.
- `Jenkinsfile`: definição do pipeline.

A configuração inicial requer os plugins `workflow-aggregator`,
`git` e `junit`, o cadastro do agente e sua chave de conexão
em `.jenkins-secrets/agent-secret`.
Essa pasta é ignorada pelo Git e pelo contexto de build do Docker.

Após a configuração inicial, inicie os serviços com:

```powershell
docker compose -f compose.jenkins.yaml up -d --build
docker compose -f compose.jenkins.yaml ps
```

O Jenkins fica disponível em http://127.0.0.1:8080.

O job deve usar Pipeline from SCM, com este repositório Git,
a branch desejada e o caminho `Jenkinsfile`.

O pipeline obtém o código, configura o CMake, compila e executa
os testes com CTest. Os resultados são publicados no Jenkins
a partir de `build/test-results.xml`, inclusive quando há testes falhando.
A ausência de testes ou de resultados também causa falha.

Após uma execução bem-sucedida, a biblioteca de compilação para
o computador, `build/libmetasurface_core.a`, é arquivada como artefato.

Após a primeira execução do job, o Jenkins consulta alterações
na branch configurada a cada cinco minutos, enquanto estiver em execução.
São mantidas as últimas 20 execuções.

Para parar os serviços preservando os volumes:

```powershell
docker compose -f compose.jenkins.yaml stop
```

### Cadastro do job

O script `ci/configure-job.groovy` cria ou atualiza o job
`controle-metassuperficie`, que utiliza o `Jenkinsfile` deste repositório.
A branch pode ser informada como primeiro argumento; o padrão é `main`.
O script deve ser executado pela CLI administrativa do Jenkins.

### Entrada binária

A função `parse_binary_input` converte uma sequência de caracteres
em uma máscara de 32 bits, sem alocação dinâmica.
Aceita somente `0` e `1`, com comprimento igual ao modo informado.
O primeiro caractere corresponde ao canal 1.
Por exemplo, no modo 4, `1010` corresponde à máscara 5: canais 1 e 3 ativos.

O comprimento não inclui o terminador nulo. O buffer deve conter
pelo menos a quantidade de bytes informada; não precisa terminar em nulo.
Em caso de erro, a função retorna `false` e preserva o argumento de saída.

### Comandos de modo

A função `apply_mode_command` interpreta comandos de `M1` a `M32`
e aplica a seleção por meio de `ControllerState`.
Os comandos usam `M` maiúsculo, sem espaços nem zeros à esquerda.

Uma seleção válida zera todas as saídas, inclusive ao repetir o modo atual.
Comandos inválidos retornam `false` e preservam o modo e as saídas anteriores.

O comprimento informado inclui somente os caracteres do comando,
sem terminador nulo ou quebra de linha. O buffer deve conter pelo menos
a quantidade de bytes informada e não precisa terminar em nulo.

### Processamento de comandos

A função `process_command` recebe um comando completo e atualiza
o estado lógico por meio dos parsers e de `ControllerState`.

- `M1` a `M32`: selecionam o modo e zeram as saídas.
- Uma sequência binária com N caracteres atualiza as saídas no modo N.
- Entradas binárias antes da seleção de modo são rejeitadas.
- Comandos inválidos preservam o modo e as saídas anteriores.

O retorno é `true` para um comando aplicado e `false` para um comando rejeitado.
Por exemplo, `M4` seguido de `1010` deixa os canais 1 e 3 em 1.

Cada chamada recebe somente os caracteres de um comando, sem quebra de linha.
O buffer deve conter pelo menos a quantidade de bytes informada
e não precisa terminar em nulo.

### Recepção por linhas

A classe `LineReceiver` recebe um caractere por chamada e armazena
até 32 caracteres de comando em um buffer fixo, sem alocação dinâmica.
O comando completo é encaminhado a `process_command` ao receber LF (`\n`).

Também aceita CRLF (`\r\n`). O CR só é permitido imediatamente antes do LF.
Linhas vazias são ignoradas.

Linhas maiores que 32 caracteres ou com CR fora de posição são
descartadas até o próximo LF. Uma linha rejeitada preserva o estado anterior.
Após o LF, o receptor fica pronto para receber outro comando.

Retornos de `receive`:

- `Pending`: aguardando a conclusão da linha ou um novo comando.
- `Applied`: comando completo aceito e aplicado ao estado lógico.
- `Rejected`: linha concluída e rejeitada.

### Catálogo de padrões

O catálogo contém 362 combinações: 2 em M1, 4 em M2, 8 em M3
e 12 por modo de M4 a M32, sem repetição dentro de cada modo.
A numeração segue os padrões iniciais de Tabelas_M1_M32.xlsx.

`pattern_count(mode)` retorna a quantidade de tabelas do modo
ou zero para um modo inválido.

`get_pattern(mode, table, outputs)` consulta uma tabela, numerada a partir de 1.
Retorna `true` e escreve a máscara em `outputs` quando a consulta é válida.
Em caso de erro, retorna `false` e preserva o valor anterior de `outputs`.

O bit 0 corresponde ao canal 1. Canais fora do modo permanecem em zero.
Por exemplo, M4/T5 corresponde a `1100`, representado pela máscara `0x00000003`.
O catálogo usa uma tabela fixa, sem alocação dinâmica.

## Comandos de tabela

O processador aceita `T<n>` para aplicar uma tabela do modo selecionado.
M1 aceita T1 a T2; M2, T1 a T4; M3, T1 a T8; M4 a M32, T1 a T12.
O comando substitui as saidas pelo padrao do catalogo e mantem o modo atual.
T1 desliga todas as saidas. Repetir uma tabela reaplica o mesmo padrao.

Exemplo: `M4`, seguido de `T5`, aplica `1100` aos canais 1 a 4.
Depois disso, uma entrada manual como `0101` pode substituir esse padrao.

Use T maiusculo, sem zeros a esquerda, espacos ou sinais.
Sem modo selecionado, com numero fora do intervalo ou sintaxe invalida,
o comando e rejeitado sem alterar o modo nem as saidas.
`M<n>` e `T<n>` compartilham a validacao da sintaxe numerica.

Na recepcao por linhas, o comando so e aplicado ao receber LF; CRLF tambem e aceito.

## Comando LISTA

`LISTA` mostra as tabelas do modo atual sem alterar o modo nem as saidas.
Use exatamente LISTA em maiusculas, sem argumentos ou espacos.
A recepcao por linhas executa a consulta ao receber LF e tambem aceita CRLF.

Exemplo de resposta no modo M2:

```text
M2
T1: 00
T2: 01
T3: 10
T4: 11
```

Cada padrao tem exatamente a quantidade de bits do modo, preservando zeros a esquerda.
O primeiro bit corresponde ao canal 1. As linhas da resposta terminam com LF.

A listagem usa `TextOutput` para enviar uma linha por vez, com um buffer fixo.
Passe o destino de saida a `process_command` ou a `LineReceiver::receive`.
Sem modo selecionado ou sem destino de saida, LISTA e rejeitado.
A conexao dessa interface com a UART sera adicionada na etapa de hardware.

## Respostas aos comandos

Com um destino `TextOutput`, comandos M, T e binarios enviam uma confirmacao de sucesso.
LISTA continua respondendo com o catalogo, sem uma linha OK adicional.
Erros preservam o modo e as saidas e informam como corrigir a entrada.

Exemplos no modo M4:

| Entrada | Resposta |
| --- | --- |
| `M4` | `OK: modo M4; saidas desligadas.` |
| `T2` | `OK: tabela aplicada.` |
| `1010` | `OK: saidas atualizadas.` |
| `101` | `ERRO: entrada binaria invalida; envie exatamente 4 bits (0 ou 1).` |
| `T13` | `ERRO: tabela invalida; use T1 a T12.` |
| `M33` | `ERRO: modo invalido; use M1 a M32.` |

Antes da selecao de modo, dados, tabelas e LISTA solicitam M1 a M32.
Entradas binarias comecam com 0 ou 1; outros comandos desconhecidos recebem orientacao de sintaxe.
O intervalo de tabelas acompanha o modo: T1 a T2, T4, T8 ou T12.

`LineReceiver` responde somente ao receber LF, inclusive em linhas terminadas com CRLF.
Linhas vazias sao ignoradas. Linhas longas ou com CR fora do terminador geram um unico erro no LF.
O receptor fica pronto para o proximo comando depois de cada linha.

`TextOutput::write` pode receber fragmentos da resposta; consuma os bytes antes de retornar.
Todas as respostas terminam com LF. Passe o mesmo destino em cada chamada de recepcao.
Sem destino de saida, M, T e binarios continuam funcionando; LISTA exige um destino.
As confirmacoes indicam atualizacao do estado logico; a integracao com GPIO e UART sera feita depois.

## Compilacao para ATmega2560

As imagens de desenvolvimento e do agente Jenkins incluem gcc-avr, avr-libc e binutils-avr.
O arquivo `cmake/avr-mega2560.cmake` seleciona avr-g++, o ATmega2560 e as ferramentas de arquivo AVR.
O nucleo continua usando C++11. A compilacao AVR desativa excecoes e RTTI.

Reconstrua a imagem de desenvolvimento depois de alterar o Dockerfile:

```powershell
docker compose build dev
```

Para compilar o nucleo para o microcontrolador:

```powershell
docker compose run --rm dev cmake -S . -B build/avr "-DCMAKE_TOOLCHAIN_FILE=cmake/avr-mega2560.cmake" -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_TESTING=OFF
docker compose run --rm dev cmake --build build/avr --parallel 2
docker compose run --rm dev avr-objdump -f build/avr/libmetasurface_core.a
```

A biblioteca AVR fica em `build/avr/libmetasurface_core.a`; a biblioteca nativa permanece em `build/libmetasurface_core.a`.
Os objetos do ATmega2560 usam a arquitetura avr:6, mostrada pelo avr-objdump.
Os 153 testes GoogleTest continuam sendo executados no computador, na pasta build.
BUILD_TESTING deve ficar OFF na compilacao cruzada, para nao misturar GoogleTest nativo com objetos AVR.

O Jenkins inclui a etapa Compilar AVR e arquiva as duas bibliotecas.
Depois de alterar o Dockerfile do agente, reconstrua a imagem e atualize o servico antes de executar a pipeline:

```powershell
docker compose -f compose.jenkins.yaml build agent
docker compose -f compose.jenkins.yaml up -d --no-deps agent
```

Esta etapa verifica a compilacao do nucleo para AVR. Ainda nao gera firmware .hex nem grava a placa.
A ligacao do programa completo e a integracao com UART e GPIO serao adicionadas nas proximas etapas.

## Catalogo na memoria Flash

No AVR, o catalogo usa PROGMEM e cada padrao e lido com pgm_read_dword.
A matriz de 32 x 12 valores de 32 bits ocupa 1536 bytes, que deixam de precisar de uma copia na SRAM.
Os 362 padroes validos e o comportamento dos comandos permanecem iguais.
Na compilacao nativa, o acesso ao catalogo continua sendo direto.

Depois de compilar a biblioteca AVR, confira sua tabela de simbolos:

```powershell
docker compose run --rm dev cmake "-DARCHIVE=build/avr/libmetasurface_core.a" -P ci/check-avr-catalog.cmake
```

O script exige que kPatterns ocupe 1536 bytes em uma secao .progmem.data. O Jenkins executa essa verificacao na etapa Compilar AVR.
A verificacao inspeciona a biblioteca, sem executar codigo na placa. Os 153 testes continuam sendo executados na compilacao nativa.
pgm_read_dword requer o catalogo nos primeiros 64 KiB da Flash; a posicao final e o uso total de memoria devem ser conferidos ao ligar o firmware.

## Controle das saidas digitais

A interface DigitalOutput separa a aplicacao dos estados da escrita no hardware.
O canal 1 corresponde a D22 e o canal 32 a D53; o canal n usa D(n + 21).
O bit 0 representa o canal 1. true significa HIGH/ligado e false significa LOW/desligado.

initialize_outputs escreve LOW nos 32 pinos antes de configurar qualquer um como saida.
O adaptador deve guardar o nivel em write mesmo antes de configure_output e preserva-lo ao habilitar a saida.
Chame initialize_outputs na inicializacao, antes de aplicar comandos.
apply_outputs escreve os 32 niveis de ControllerState, incluindo LOW nos canais fora do modo.
As escritas sao sequenciais. Essas funcoes nao alteram o estado logico nem interpretam comandos.

Os 14 novos testes usam um substituto de DigitalOutput para verificar pinos, niveis, ordem de inicializacao e aplicacao dos comandos.
Nesta etapa, o chamador invoca apply_outputs explicitamente; o receptor de linhas continua independente do hardware.
O adaptador AVR, a integracao com UART e a validacao fisica ficam para as proximas etapas.
