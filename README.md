# Shell

Este é um shell customizado desenvolvido em C, capaz de executar comandos internos e externos, incluindo suporte para redirecionamento de saída e execução de scripts batch. O projeto foi desenvolvido para a disciplina de Sistemas Operacionais.

## Funcionalidades

- Executa comandos internos como `cd`, `path`, `exit` e `batch`.
- Suporte para redirecionamento de saída usando `>`.
- Capaz de rodar múltiplos comandos em paralelo.
- Implementação do comando `cat` e `ls` através de binários .

## Requisitos

- Sistema operacional Linux.
- Compilador `gcc`.

## Instalação

1. Clone o repositório para o seu diretório local:
    ```sh
    git clone https://github.com/FelipeDiasKonda/shell_SO.git
    cd shell_SO
    ```
2. Inicie a maquina virtual abra o terimnal e logue:
    ```sh
    ssh seu_usuario@127.0.0.1 -p2222
    ```
    em seguida coloque a senha
3. Entre no Diretório que esta clonado:
 
    ```sh
    cd shell_SO
    ```

4. Compile o código e os arquivos Cat e Ls e instale o readline caso necessário:
    ```sh
    sudo apt-get install libreadline-dev
     ```
    ```sh
    gcc -o shell shell.c -lreadline
    ```
    ```sh
    gcc -o cat catcopy.c
    ```
    ```sh
    gcc -o ls lscopy.c
    ```
## Uso

1. Execute o shell:
    ```sh
    ./shell
    ```

2. Use o shell normalmente, digitando comandos como faria em um terminal comum. Aqui estão alguns exemplos de comandos suportados:

    - Mudar de diretório:
        ```sh
        cd /caminho/para/diretorio
        ```

    - Listar ou definir caminhos de busca:
        ```sh
        path /caminho_desejado
        ```
    após definir o caminho de busca basta digitar o nome do executavel que ira executar.
    - Concatenar arquivos e imprimir no terminal:
        ```sh
        cat arquivo.txt
        ```

    - Executar um script batch:
        ```sh
        batch script.txt
        ```

    - Redirecionar a saída de um comando para um arquivo:
        ```sh
        ls > saida.txt
        ```

    - Executar comandos em paralelo (usando `&`):
        ```sh
        comando1 & comando2 &
        ```

    - Sair do shell:
        ```sh
        exit
        ```

