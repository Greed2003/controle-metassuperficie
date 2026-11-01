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
