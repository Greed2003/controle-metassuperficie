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
