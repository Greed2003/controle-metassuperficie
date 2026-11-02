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
SerialTransport implementa essa interface e envia as respostas pela UART.

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
No firmware integrado, as confirmacoes sao geradas depois das escritas no GPIO, sem medir o circuito fisico.

## Compilacao para ATmega2560

As imagens de desenvolvimento e do agente Jenkins incluem gcc-avr, avr-libc e binutils-avr.
O arquivo `cmake/avr-mega2560.cmake` seleciona avr-g++, o ATmega2560 e as ferramentas de arquivo AVR.
O nucleo continua usando C++11. A compilacao AVR desativa excecoes e RTTI.

Reconstrua a imagem de desenvolvimento depois de alterar o Dockerfile:

```powershell
docker compose build dev
```

Para compilar as bibliotecas e o firmware para o microcontrolador:

```powershell
docker compose run --rm dev cmake -S . -B build/avr "-DCMAKE_TOOLCHAIN_FILE=cmake/avr-mega2560.cmake" -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_TESTING=OFF
docker compose run --rm dev cmake --build build/avr --parallel 2
docker compose run --rm dev avr-objdump -f build/avr/libmetasurface_core.a
```

A biblioteca AVR fica em `build/avr/libmetasurface_core.a`; a biblioteca nativa permanece em `build/libmetasurface_core.a`.
Os objetos do ATmega2560 usam a arquitetura avr:6, mostrada pelo avr-objdump.
Os 224 testes GoogleTest continuam sendo executados no computador, na pasta build.
BUILD_TESTING deve ficar OFF na compilacao cruzada, para nao misturar GoogleTest nativo com objetos AVR.

O Jenkins inclui a etapa Compilar AVR e arquiva as bibliotecas, o ELF, o HEX, o mapa de ligacao e o relatorio de memoria.
Depois de alterar o Dockerfile do agente, reconstrua a imagem e atualize o servico antes de executar a pipeline:

```powershell
docker compose -f compose.jenkins.yaml build agent
docker compose -f compose.jenkins.yaml up -d --no-deps agent
```

A compilacao AVR liga o programa completo e gera o firmware .hex, conforme a secao Firmware executavel.
A gravacao e a validacao fisica na placa ainda nao sao executadas pelo projeto.

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
A verificacao inspeciona a biblioteca, sem executar codigo na placa. Os 224 testes continuam sendo executados na compilacao nativa.
pgm_read_dword requer o catalogo nos primeiros 64 KiB da Flash. ci/check-avr-firmware.cmake verifica essa condicao no ELF final, junto dos limites de Flash e SRAM.

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
ControllerApplication coordena a recepcao dos comandos e a aplicacao das saidas pelas interfaces abstratas.
Os adaptadores de GPIO e UART estao descritos abaixo e sao integrados pelo firmware. A validacao fisica permanece pendente.

## GPIO do ATmega2560

AvrDigitalOutput implementa DigitalOutput e aceita somente D22 a D53.
No AVR, o construtor padrao referencia os registradores reais do ATmega2560.
O construtor alternativo recebe referencias a registradores, usadas pelos testes nativos; os registradores devem existir enquanto o adaptador for usado.
Nenhum construtor escreve nos registradores.

| Pinos | Bits da porta, na mesma ordem |
| --- | --- |
| D22 a D29 | PA0 a PA7 |
| D30 a D37 | PC7 a PC0 |
| D38 | PD7 |
| D39 a D41 | PG2 a PG0 |
| D42 a D49 | PL7 a PL0 |
| D50 a D53 | PB3 a PB0 |

Referencia: https://docs.arduino.cc/resources/pinouts/A000067-full-pinout.pdf

write altera somente o bit correspondente em PORTx. configure_output habilita somente esse bit em DDRx, preservando o nivel previamente escrito.
Pinos fora do intervalo sao ignorados. No AVR, cada alteracao usa ATOMIC_BLOCK(ATOMIC_RESTORESTATE), restaurando o estado anterior das interrupcoes.
Os demais bits das portas sao preservados. Os 32 canais continuam sendo atualizados sequencialmente.
Os pinos devem estar disponiveis como GPIO: perifericos como SPI, memoria externa e saidas de temporizadores nao podem assumir esses mesmos pinos.
O adaptador nao configura nem desativa esses perifericos.

No firmware AVR, SerialTransport e ControllerApplication coordenam initialize_outputs e apply_outputs usando AvrDigitalOutput.
A biblioteca separada fica em build/avr/libmetasurface_avr.a e depende de metasurface_core.
O Jenkins compila e arquiva essa biblioteca junto das bibliotecas do nucleo.
Os testes de GPIO verificam mapeamento, bits preservados, inicializacao e aplicacao dos estados.
Os testes usam registradores simulados. A compilacao AVR gera o ELF e o HEX, mas nao valida sinais na placa.

## UART do ATmega2560

AvrUart usa a USART0: D0/RX0 e D1/TX0. O construtor AVR referencia os registradores reais; o alternativo recebe referencias para testes.
Os registradores referenciados devem existir enquanto o adaptador for usado. Os construtores nao escrevem neles.

initialize configura 115200 baud nominais, 8 bits, sem paridade e 1 bit de parada, com RX e TX habilitados e interrupcoes da UART desabilitadas.
O clock real deve ser 16 MHz. O CMake define F_CPU=16000000UL; essa definicao nao altera o clock fisico.
U2X0=1 e UBRR0=16 produzem aproximadamente 117647 baud, desvio de +2,12% em relacao ao nominal; a comunicacao fisica sera validada na placa.
A inicializacao habilita o clock da USART0 preservando os outros bits de PRR0. No AVR, restaura o estado anterior das interrupcoes globais.
Chame initialize antes da comunicacao, com o transmissor ocioso; desabilitar RX descarta dados pendentes no hardware.

| Operacao | Resultado |
| --- | --- |
| try_read(value) | Received entrega um byte, incluindo 0x00. Empty indica ausencia de byte disponivel ou receptor/clock desabilitado. |
| try_read(value) com FE, DOR ou UPE | Error consome o byte com erro e preserva value. |
| try_write(value) | true indica byte aceito pelo buffer; false indica buffer ocupado ou transmissor/clock desabilitado. |

As operacoes retornam imediatamente. true em try_write nao significa que o ultimo bit ja saiu pelo pino.
Use a UART por consulta de flags, a partir de um unico fluxo de execucao, sem outro driver ou ISR acessando a USART0.
O chamador deve consultar a recepcao frequentemente para evitar perda de dados. Ao receber Error, deve invalidar a linha atual ate um novo LF valido.
SerialTransport liga a UART a aplicacao e ao GPIO; o adaptador AvrUart continua responsavel apenas pela troca de bytes.

A UART integra libmetasurface_avr.a, ja compilada e arquivada pelo Jenkins.
Os 11 testes da UART cobrem configuracao, disponibilidade, todos os valores de byte e erros.
Os registradores simulados verificam a logica e as escritas; nao reproduzem temporizacao nem todos os efeitos automaticos do hardware, como o avanco da FIFO.
Referencia: ATmega2560 Datasheet, secoes 22.5, 22.7 e 22.10, tabela 22-12.

## Aplicacao do controlador

ControllerApplication possui o estado e o receptor de linhas, e recebe DigitalOutput e um TextOutput opcional.
Essas dependencias devem existir enquanto a aplicacao for usada. O construtor nao executa I/O.
initialize reinicia o modo, as saidas logicas e a recepcao; escreve LOW nos 32 pinos antes de configura-los como saida.
Antes de initialize, receive retorna Pending e ignora os bytes; discard_line tambem nao produz efeitos.

receive recebe um byte valido por chamada. M, T e bits aceitos escrevem os 32 niveis antes de iniciar a resposta de sucesso.
LISTA e comandos rejeitados nao escrevem nem reconfiguram os pinos. Linhas vazias nao geram respostas.
ReceiveResult::Applied significa comando aceito, incluindo LISTA; a aplicacao ja cuida das escritas necessarias.
state permite consultar o estado sem modifica-lo. A confirmacao indica escritas solicitadas ao adaptador, sem medir o circuito fisico.

Ao detectar erro de recepcao, o chamador deve usar discard_line e nao encaminhar o byte com erro a receive.
A linha inteira e descartada ate o proximo LF recebido sem erro. Nesse LF, retorna Rejected e emite uma unica resposta de linha invalida.
Erros repetidos durante o descarte nao geram respostas adicionais. O modo e as saidas anteriores sao preservados.
Se o proprio LF foi perdido ou recebido com erro, o descarte continua ate outro LF valido, podendo incluir a linha seguinte.

Sem TextOutput, M, T e bits continuam funcionando; LISTA e rejeitado por nao haver destino para a listagem.
process_command e LineReceiver aceitam um DigitalOutput opcional, previamente inicializado; chamadas existentes continuam validas.
Use um unico fluxo de execucao, sem chamadas reentrantes pelos adaptadores.
SerialTransport conecta esta aplicacao a UART; firmware/main.cpp instancia os adaptadores e executa o transporte continuamente.

Os 17 testes da aplicacao verificam inicializacao, todos os modos e tabelas, ordem das respostas, preservacao das saidas e recuperacao de linhas.

## Transporte serial

SerialTransport possui ControllerApplication e conecta a interface Uart ao controle das saidas. AvrUart implementa Uart.
O construtor nao executa I/O. initialize reinicia o estado, coloca os 32 pinos em LOW e os configura antes de inicializar a UART.
As dependencias devem existir enquanto o transporte for usado. Use um unico fluxo de execucao, sem reentrancia ou outro consumidor da UART.

Chame poll continuamente: cada chamada tenta ler um byte, processa no maximo um evento recebido e tenta transmitir no maximo um byte.
Nao ha espera pela disponibilidade da UART. Uma escrita recusada e tentada novamente sem avancar a resposta.
A resposta inteira e copiada para um buffer fixo de 512 bytes antes de iniciar seu envio; LISTA em M32 usa 451 bytes.
O comando seguinte so e processado depois que todos os bytes da resposta anterior forem aceitos pela UART.
response_pending informa bytes ainda no buffer de software; false nao garante que o ultimo byte ja terminou no fio.

Durante o envio, a recepcao continua em uma fila fixa de 64 eventos, incluindo marcadores de erro.
Erros da UART invalidam a linha na ordem em que chegaram; o byte com erro nunca e interpretado como dado ou LF.
Se a fila lotar, a entrada seguinte descarta toda a fila ainda nao processada e invalida a linha, preservando a resposta em curso.
O proximo LF valido processado encerra o descarte e produz ERRO de linha invalida. Comandos descartados nao alteram as saidas.
Erros ou estouros repetidos antes desse realinhamento continuam o descarte. Um LF com erro nao encerra a linha.

O computador deve enviar um comando e aguardar sua resposta completa antes do seguinte; a fila nao garante rajadas ilimitadas.
Em LISTA, aguarde o cabecalho e todas as tabelas: 2 em M1, 4 em M2, 8 em M3 e 12 em M4 a M32.
As demais respostas terminam em um LF. Linhas vazias nao recebem resposta.
poll deve ser chamado com frequencia suficiente; a fila de software nao evita perda de bytes enquanto a UART deixa de ser atendida.

Inicialize ou reinicialize com a comunicacao parada e o transmissor ocioso. A reinicializacao limpa as filas de software e o estado.
Bytes ja entregues ao hardware ou recebidos pelo computador nao podem ser recolhidos por essa limpeza.
Se uma futura resposta ultrapassar 512 bytes, faulted fica true e o transporte para ate initialize, sem transmitir um prefixo truncado.
Esse caso indica quebra do limite interno, nao erro de comando; nao ha desfazimento de comandos ja aplicados.

Os 18 novos testes cobrem pausas no envio, listagens completas, ordem dos comandos, erros, estouro da fila, reinicializacao e o adaptador AVR.
Total: 224 testes nativos. A geracao do firmware esta descrita abaixo; a validacao fisica permanece pendente.

## Firmware executavel

firmware/main.cpp instancia AvrDigitalOutput, AvrUart e SerialTransport com duracao estatica.
Os construtores nao executam I/O. O inicio do programa desabilita o watchdog antes da inicializacao da RAM e dos construtores C++.
main inicializa o transporte: escreve LOW em D22 a D53, configura as saidas e so depois habilita a UART.
Em seguida, executa poll continuamente, sem atrasos e com interrupcoes globais desabilitadas.
O firmware usa C++11 e avr-libc, sem Arduino Core, alocacao dinamica, excecoes ou RTTI.
A inicializacao das saidas vale a partir da execucao do programa; nao define os niveis eletricos durante reset ou bootloader.

A configuracao AVR inclui cmake/firmware-avr.cmake. O build padrao gera:

| Arquivo em build/avr | Conteudo |
| --- | --- |
| metasurface_firmware.elf | Executavel AVR com simbolos |
| metasurface_firmware.hex | Imagem Intel HEX das secoes .text e .data |
| metasurface_firmware.map | Mapa de ligacao |
| metasurface_firmware.size.txt | Limites, consumo de memoria, secoes e simbolos |

Antes de gerar o HEX, ci/check-avr-firmware.cmake exige um ELF executavel avr:6 e verifica:

- Imagem de Flash de ate 253952 bytes, reservando 8192 bytes para o bootloader do Mega 2560.
- Uso estatico da SRAM interna de ate 6144 dos 8192 bytes, deixando ao menos 2048 bytes para a pilha.
- Catalogo kPatterns de 1536 bytes na Flash, inteiramente nos primeiros 65536 bytes, conforme pgm_read_dword.
- Somente as secoes alocadas previstas: .text, .data, .bss e .noinit.

A medicao considera os enderecos finais e inclui os dados inicializados na imagem de Flash.
A SRAM estatica inclui os objetos e buffers globais. A reserva de pilha e um limite de build, nao uma medicao do pior caso em execucao.
O target firmware_artifacts refaz a verificacao e gera o HEX a cada build AVR, mesmo sem recompilar os fontes.
O mapa e produzido ao ligar o ELF. O Jenkins arquiva os quatro arquivos apos o sucesso da pipeline.
A compilacao nativa continua independente das ferramentas AVR e executa os mesmos 224 testes.

O firmware usa USART0 a 115200 baud nominais, 8N1, com clock de 16 MHz.
Envie um comando por vez e aguarde a resposta completa, conforme Transporte serial.
Esta etapa gera os arquivos; nao grava a placa, altera fuses nem valida temporizacao ou sinais fisicos.
