#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_INPUT 1024
#define MAX_ARGS 100
#define MAX_PATH 256

// Caminhos padrões para busca de executáveis
char **search_paths = NULL;
int num_paths = 0;

void execute_batch(char *filename, char *current_path);
int execute_external(char **args, char *output_file);
void parse_input(char *input, char **args);
int check_output_redirection(char **args, char **output_file);
int execute_builtin(char **args, char *current_path, char *output_file);
void execute_commands_in_parallel(char *input, char *current_path);

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

int check_output_redirection(char **args, char **output_file) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            *output_file = args[i + 1];
            args[i] = NULL;
            return 1;
        }
    }
    return 0;
}

// Função para executar comandos internos
int execute_builtin(char **args, char *current_path, char *output_file) {
    if (strcmp(args[0], "exit") == 0) {
        printf("Saindo do shell.\n");
        exit(2); // Retorna 2 para indicar que o comando exit foi chamado
    } else if (strcmp(args[0], "cd") == 0) {
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
        return 1; // Comando builtin executado com sucesso
    } else if (strcmp(args[0], "path") == 0) {
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
        return 1; // Comando builtin executado com sucesso
    }
    return 0; // Não é um comando builtin
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

    while ((read = getline(&line, &len, file)) != -1) { // Lê o arquivo linha por linha
        if (line[read - 1] == '\n') line[read - 1] = '\0'; // Remove a nova linha no final da entrada
        parse_input(line, args); // Divide a entrada em argumentos

        if (args[0] == NULL) { // Comando vazio
            continue; // Pule para a próxima linha
        }

        if (strcmp(args[0], "batch") == 0) { // Prevenção de loop recursivo
            fprintf(stderr, "Erro: comando 'batch' não permitido em arquivo batch.\n");
            continue; // Pule para a próxima linha
        }

        char *output_file = NULL;
        if (check_output_redirection(args, &output_file)) {
            if (execute_builtin(args, current_path, output_file) == 2) {
                break; // Sai do loop se o comando 'exit' foi executado
            }
            if (execute_builtin(args, current_path, output_file) == 0) { // Se não é built-in, execute externo
                execute_external(args, output_file);
            }
        } else {
            if (execute_builtin(args, current_path, NULL) == 2) {
                break; // Sai do loop se o comando 'exit' foi executado
            }
            if (execute_builtin(args, current_path, NULL) == 0) { // Se não é built-in, execute externo
                execute_external(args, NULL);
            }
        }
    }

    free(line); // Libera a linha lida para evitar vazamentos de memória
    fclose(file); // Fecha o arquivo após a leitura
}

// Função para buscar o executável no caminho definido
char *find_executable(char *program) {
    // Verifica se é um caminho absoluto ou relativo
    if (program[0] == '.' || program[0] == '/') {
        if (access(program, X_OK) == 0) {
            return strdup(program);
        }
    }

    // Procura nos caminhos definidos, incluindo o diretório dos binários personalizados
    for (int i = 0; i < num_paths; i++) {
        char path[MAX_INPUT];
        snprintf(path, sizeof(path), "%s/%s", search_paths[i], program);
        if (access(path, X_OK) == 0) {
            return strdup(path);
        }
    }

    // Caso não encontre, retorna NULL
    return NULL;
}


// Função para executar programas externos
int execute_external(char **args, char *output_file) {
    pid_t pid = fork();
    if (pid == 0) { // Processo filho
        // Redireciona a saída se necessário
        if (output_file != NULL) {
            int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out < 0) {
                perror("Erro ao abrir arquivo de saída");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        // Encontra o caminho do programa
        char *program = find_executable(args[0]);
        if (program == NULL) {
            fprintf(stderr, "Erro: programa '%s' não encontrado.\n", args[0]);
            exit(1);
        }

        // Executa o programa
        if (execv(program, args) == -1) {
            perror("Erro ao executar programa");
            exit(1);
        }
    } else if (pid < 0) {
        perror("Erro ao criar processo");
    } else { // Processo pai
        return pid; // Retorna o PID do processo filho
    }
    return 0;
}

void execute_commands_in_parallel(char *input, char *current_path) {
    char *commands[MAX_ARGS];
    int num_commands = 0;

    // Dividir a entrada em comandos separados por '&'
    commands[num_commands] = strtok(input, "&");
    while (commands[num_commands] != NULL && num_commands < MAX_ARGS - 1) {
        num_commands++;
        commands[num_commands] = strtok(NULL, "&");
    }

    pid_t pids[MAX_ARGS];
    int num_pids = 0;

    // Executar cada comando em um processo separado
    for (int i = 0; i < num_commands; i++) {
        char *args[MAX_ARGS];
        parse_input(commands[i], args);

        if (args[0] == NULL) {
            continue;
        }

        char *output_file = NULL;
        if (check_output_redirection(args, &output_file)) {
            if (execute_builtin(args, current_path, output_file) != 1) {
                pids[num_pids++] = execute_external(args, output_file);
            }
        } else {
            if (execute_builtin(args, current_path, NULL) != 1) {
                pids[num_pids++] = execute_external(args, NULL);
            }
        }
    }

    // Esperar que todos os processos terminem
    for (int i = 0; i < num_pids; i++) {
        if (pids[i] > 0) {
            int status;
            waitpid(pids[i], &status, 0);
            printf("Processo %d finalizado com status %d.\n", pids[i], WEXITSTATUS(status));
        }
    }
}

int main() {
    // Caminhos padrão para busca de executáveis
    search_paths = (char **)malloc(sizeof(char *));
    search_paths[0] = strdup("/bin");
    num_paths = 1;

    // Diretório onde os binários mycat e myls estão localizados
    char custom_bin_dir[MAX_PATH];
    if (getcwd(custom_bin_dir, sizeof(custom_bin_dir)) == NULL) {
        perror("Erro ao obter diretório atual");
        return 1; // Erro crítico
    }

    // Adiciona o diretório dos binários ao search_paths
    search_paths = realloc(search_paths, sizeof(char *) * (num_paths + 1));
    search_paths[num_paths] = strdup(custom_bin_dir);
    num_paths++;

    char input[MAX_INPUT];
    char current_path[MAX_PATH];
    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        perror("Erro ao obter diretório atual");
        return 1; // Erro crítico
    }

    while (1) {
        printf("%s$ ", current_path); // Exibe o prompt com o caminho atual
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break; // Sai do loop se houver erro na leitura da entrada
        }

        // Remove a nova linha no final da entrada
        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }

        // Verifica se a entrada está vazia
        if (strlen(input) == 0) {
            continue; // Volta ao prompt
        }

        // Verifica se a entrada é um comando batch
        if (strncmp(input, "batch ", 6) == 0) {
            execute_batch(input + 6, current_path);
            continue;
        }

        // Executa comandos em paralelo
        execute_commands_in_parallel(input, current_path);
    }

    // Libera memória alocada para search_paths
    for (int i = 0; i < num_paths; i++) {
        free(search_paths[i]);
    }
    free(search_paths);

    return 0;
}

