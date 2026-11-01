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
