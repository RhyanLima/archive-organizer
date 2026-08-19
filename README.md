## 1. O Dicionário de Configurações (`ogz_types.h`)

Este arquivo define como as regras do usuário são armazenadas na memória.

* **`OrgMode`**: É uma enumeração (uma lista de opções) que define qual é a regra matemática. Por exemplo: `MODE_GREATER` significa "maior que", `MODE_BANDS` significa "agrupar em faixas", etc.
  
* **`ExtConfig`, `SizeConfig`, `DurConfig`**: São estruturas (`structs`) que guardam as regras de cada módulo. Elas têm um campo `active` (booleano) que diz se o usuário ativou aquele filtro no terminal.
  
* **`Config`**: É a estrutura principal (a "prancheta" do orquestrador). Ela junta todas as configurações anteriores e adiciona o `target_dir` (a pasta que será organizada).

---

## 2. Os Módulos de Filtro

Cada módulo possui duas funções principais: uma para **validar** o arquivo e outra para **nomear** a pasta de destino.

### Organizador por Extensão (`org_ext.c`)
* **`get_extension()`**: É uma função auxiliar que usa `strrchr` (uma função nativa do C que busca a última ocorrência de um caractere, neste caso, o ponto `.`). Ela retorna tudo o que vem depois do ponto.
  
* **`match_ext()`**: Verifica se o arquivo tem a extensão que o usuário pediu (caso ele tenha pedido uma específica).
  
* **`get_ext_foldername()`**: Define o nome da pasta. Se o arquivo for `.jpg`, a pasta se chamará `jpg`.

### Organizador por Tamanho (`org_size.c`)
* **`match_size()`**: Usa a biblioteca nativa `<sys/stat.h>`. A função `stat()` lê os "metadados" do arquivo diretamente do sistema operacional (tamanho, data de criação, etc.) sem precisar abri-lo, o que é muito rápido. Ela extrai o tamanho em bytes e compara com a regra (maior que, menor que, etc.).
  
* **`get_size_foldername()`**: Faz a matemática para dar nomes legíveis às pastas. Se você usou o modo "band" (faixas) de 50MB, ela divide o tamanho do arquivo por 50MB para descobrir em qual "gaveta" (índice) ele deve cair.

### Organizador por Duração (`org_dur.c`)
* **`get_duration_via_ffprobe()`**: Como o C não sabe ler mídia nativamente, usa-se a função `popen()`. Ela abre um terminal "invisível" dentro do programa, roda o comando do `ffprobe` e lê a resposta retornada. A função `atof()` converte o texto retornado em um número decimal (`double`).
  
* **`match_dur()` e `get_dur_foldername()`**: Seguem exatamente a mesma lógica do organizador de tamanho, mas baseados em segundos em vez de bytes.

---

## 3. O Orquestrador (`ogz.c`)

Este é o cérebro da operação. Ele amarra tudo e interage com o sistema de arquivos.

### Leitura de Argumentos (`parse_args`)
Ele percorre o vetor `argv` (que contém o que você digitou no terminal). 
* Se encontra `-x`, ativa a struct de extensão. 
* Se encontra `-s`, lê a regra seguinte (ex: `band:50`), faz o cálculo (multiplicando por megabytes) e salva na struct de tamanho.

### O Loop Principal (`main`)
A função usa `<dirent.h>` para abrir a pasta que você indicou e começa um loop `while` lendo arquivo por arquivo.

**1. Filtro em Cascata:**
```c
if (!match_ext(...)) continue;
if (!match_size(...)) continue;
if (!match_dur(...)) continue;
```
Isso é o que torna o programa acumulativo. O `continue` diz ao C: "pule para o próximo arquivo". Se você pediu para separar vídeos `.mp4` maiores que `50MB`, o arquivo tem que passar pelos dois primeiros testes. Se for `.mp4` mas tiver só `10MB`, ele é barrado no segundo `if` e ignorado.

**2. Decisão da Pasta:**

O código verifica qual filtro o usuário quer usar como "nomeador" principal das pastas e chama a respectiva função `get_..._foldername`.

**3. Movimentação Física (`rename`):**

O programa monta o caminho completo de origem (onde o arquivo está) e o caminho completo de destino (para onde vai).

A função `rename()` do C é usada para mover o arquivo. No Linux, se a origem e o destino estiverem no mesmo disco, o `rename` não copia o arquivo byte por byte (o que seria lento); ele apenas altera o "índice" do arquivo no disco, fazendo com que a organização seja praticamente instantânea, mesmo para arquivos de dezenas de gigabytes.

## 4. Compilar

O projeto conta com um `Makefile`. Para compilar todos os módulos e incorporar o texto de ajuda no binário, simplesmente execute:

```bash
make
```
