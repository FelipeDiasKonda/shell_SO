#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_INPUT 1024
#define MAX_ARGS 100
#define MAX_PATH 256

// Caminhos padrões para busca de executáveis
char **search_paths = NULL;
int num_paths = 0;

    void execute_batch(char *filename, char *current_path);
    void execute_external(char **args);
// Função para dividir a entrada em argumentos
void parse_input(char *input, char **args) {
    int i = 0;
    args[i] = strtok(input, " \t\n");
    while (args[i] != NULL && i < MAX_ARGS - 1) {
        i++;
        args[i] = strtok(NULL, " \t\n");
    }
    args[i] = NULL; // Último elemento é NULL para execvp
}
// Função para executar comandos internos
int execute_builtin(char **args, char *current_path) {
    if (strcmp(args[0], "exit") == 0) {
        printf("Saindo do shell.\n");
        exit(0);
    }

    if (strcmp(args[0], "cd") == 0) {
        if (args[1] != NULL) {
            if (chdir(args[1]) != 0) {
                perror("Erro ao mudar diretório");
            } else {
                if (getcwd(current_path, MAX_PATH) == NULL) { // Atualiza o diretório atual
                    perror("Erro ao obter diretório atual");
                }
            }
        } else {
            fprintf(stderr, "Erro: caminho não fornecido.\n");
        }
        return 1; // Comando interno executado
    }

    if (strcmp(args[0], "path") == 0) {
        // Liberar caminhos antigos
        for (int i = 0; i < num_paths; i++) {
            free(search_paths[i]);
        }
        free(search_paths);

        // Redefinir caminhos
        num_paths = 0;
        int i = 1;
        while (args[i] != NULL) {
            search_paths = realloc(search_paths, sizeof(char *) * (num_paths + 1));
            search_paths[num_paths] = strdup(args[i]);
            num_paths++;
            i++;
        }
        return 1; // Comando interno executado
    }
if (strcmp(args[0], "cat") == 0) { // Comando "cat"
        if (args[1] != NULL) {
            FILE *file = fopen(args[1], "r");
            if (file == NULL) {
                perror("Erro ao abrir arquivo");
            } else {
                char line[MAX_INPUT];
                while (fgets(line, sizeof(line), file) != NULL) { // Lê o arquivo linha por linha
                    printf("%s", line); // Escreve no terminal
                }
                fclose(file); // Fecha o arquivo após a leitura
            }
        } else {
            fprintf(stderr, "Erro: nome do arquivo não fornecido para 'cat'.\n");
        }
        return 1; // Indica que é um comando interno
    }
    if (strcmp(args[0], "batch") == 0) {
    if (args[1] != NULL) {
        execute_batch(args[1],current_path);
    } else {
        fprintf(stderr, "Erro: nome do arquivo não fornecido para 'batch'.\n");
    }
    return 1;
}

    return 0; // Não é um comando interno
}
// Função para executar comandos de um arquivo batch
void execute_batch(char *filename, char *current_path) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Erro ao abrir arquivo batch");
        return;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char *args[MAX_ARGS];

    while ((read = getline(&line, &len, file)) != -1) {  // Lê o arquivo linha por linha
        if (line[read - 1] == '\n') line[read - 1] = '\0';  // Remove a nova linha no final da entrada
        parse_input(line, args);  // Divide a entrada em argumentos
        if (args[0] == NULL) {  // Comando vazio
            free(line);
            continue;  // Pule para a próxima linha
        }

        if (strcmp(args[0], "batch") == 0) {  // Prevenção de loop recursivo
            fprintf(stderr, "Erro: comando 'batch' não permitido em arquivo batch.\n");
            free(line);  // Libera a linha lida
            continue;  // Pule para a próxima linha
        }

        if (execute_builtin(args, current_path) == 0) {  // Se não é built-in, execute externo
            execute_external(args);
        }

        free(line);  // Libera a linha lida para evitar vazamentos de memória
        line = NULL;  // Necessário para getline alocar memória na próxima leitura
    }

    fclose(file);  // Fecha o arquivo após a leitura
}



// Função para buscar o executável no caminho definido
char *find_executable(char *program) {
    for (int i = 0; i < num_paths; i++) {
        char path[MAX_INPUT];
        snprintf(path, sizeof(path), "%s/%s", search_paths[i], program);
        if (access(path, X_OK) == 0) {
            return strdup(path);
        }
    }
    return NULL;
}

// Função para executar programas externos
void execute_external(char **args) {
    pid_t pid = fork();
    if (pid == 0) { // Processo filho
        // Encontra o caminho do programa
        char *program = find_executable(args[0]);
        if (program == NULL) {
            fprintf(stderr, "Erro: programa '%s' não encontrado.\n", args[0]);
            exit(1);
        }

        // Executa o programa
        if (execvp(program, args) == -1) {
            perror("Erro ao executar programa");
            exit(1);
        }
    } else if (pid < 0) {
        perror("Erro ao criar processo");
    } else { // Processo pai
        int status;
        waitpid(pid, &status, 0); // Espera pelo filho
        printf("Processo finalizado com status %d.\n", WEXITSTATUS(status));
    }
}

int main() {
    // Caminhos padrão para busca de executáveis
    search_paths = (char **)malloc(sizeof(char *));
    search_paths[0] = strdup("/bin");
    num_paths = 1;

    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    char current_path[MAX_PATH];

    // Inicializa o caminho atual
    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        perror("Erro ao obter diretório atual");
        return 1; // Erro crítico
    }

    while (1) { // Laço infinito para manter o shell ativo
        char *username = getlogin();
        printf("\n%s@%s> ", username,current_path); // Mostra o diretório atual no prompt
        if (fgets(input, sizeof(input), stdin) == NULL) { // Lê a entrada do usuário
            if (feof(stdin)) { // Se o shell for interrompido (Ctrl+D)
                printf("\nSaindo do shell.\n");
                break; // Sai do loop para terminar o programa
            } else {
                perror("Erro ao ler entrada"); // Outro erro ao ler
                continue; // Tenta novamente
            }
        }

        parse_input(input, args); // Divide a entrada em argumentos

        if (args[0] == NULL) { // Comando vazio
            continue;
        }

        if (execute_builtin(args, current_path) == 0) { // Se não é built-in, execute externo
            execute_external(args);
        }
    }

    // Liberar memória dos caminhos definidos
    for (int i = 0; i < num_paths; i++) {
        free(search_paths[i]);
    }
    free(search_paths);

    return 0;
}
