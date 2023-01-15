#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>



// Structures
struct CommandLine {
    /*
    Structure for a line of a command.
    Stores parallel commands and the number of parallel commands
    */
    char **commands;
    int size;
};

struct ExeCommand {
    /*
    Structure for an executable command that can be passed into execv
    Stores the individual arguments of a command as well as number of args
        and redirection flag and filename.
    If there is redirection, then the flag is 1, otherwise it is 0. 
    */
    char **parsed_command;
    int size;
    int redirection_flag;
    char *redirection_filename;
};

struct GlobalStorage {
    /*
    Structure that contains global information
    Stream determines whether interactive or batch mode
    Also keeps track of paths and the number of paths. 
    */
    FILE *stream;
    char **paths;
    int num_paths;
};

struct MemoryStorage {
    /*
    Structure that contains temporary memory for each line of command
    Stores a command_line structure
    Stores an array of ExeCommand structures
    Stores the number of valid commands 
    Stores an array of strings (general purpose strings)
    Stores number of strings in string array
    Stores the current string that is pointed to
    Stores a input string for getline()
    */
    struct CommandLine command_line;
    struct ExeCommand *executable_commands;
    int num_valid_commands; 
    char **strings;
    int num_strings;
    int string_index;
    char *input; 
};

// Global Variables
char ERROR_MESSAGE[30] = "An error has occurred\n";
struct GlobalStorage storage = {};
struct MemoryStorage memory = {};

// Function Declarations
void initialize_global();
void initialize_memory();
void free_global();
void free_memory();
void error_exit_shell();
void exit_shell();
void display_error_message();
int shell();
char *allocate_memory(int size);
int run_command_line(char *input);
int buffer_operators(char *input, char **buffered_input);
int preprocess_command_line(char *buffered_input);
int preprocess_commands();
int clean_and_split_arguments(char *command, int index); 
int set_redirection(int index);
int execute_commands();
int exit_command(int index);
int cd_command(int index);
int path_command(int index);
char *is_executable(char *argument);
int is_builtin(char *command);
int num_builtin_commands();

// Debugging Functions
void pm();

/*
Prints Hello Worlds
Param:
    none
Return:
    none
*/
void pm() {
    printf("Hello Worlddddddd\n");
}
void ps();

/*
Prints a string
Param:
    string
Return:
    none
*/
void ps(char *string) {
    printf("%s\n", string);
}

void pcl(struct CommandLine command_line);

/*
Prints the contents in a CommandLine structure
Param:
    command_line - a CommandLine struct
Return:
    none
*/
void pcl(struct CommandLine command_line) {
    for (int i = 0; i < command_line.size; i++) {
        printf("%s\n", command_line.commands[i]); 
    }
}
void pec(); 

/*
Prints all of the ExeCommand structs and all their contents
Param:
    none
Return:
    none
*/
void pec() {
    for (int i = 0; i < memory.command_line.size; i++) {
        printf("Command:\n");
        for (int j = 0; j < memory.executable_commands[i].size; j++) {
            printf("%s\n", memory.executable_commands[i].parsed_command[j]);
        }
    }
}
void pp();

/*
Prints all of the paths
Param: 
    none
Return:
    none
*/
void pp() {
    for (int i = 0; i < storage.num_paths; i++) {
        printf("%s\n", storage.paths[i]);
    }
}

void pstrings();

/*
Prints all of the strings in the MemoryStorage struct
Param: 
    none
Return:
    none
*/
void pstrings() {
    for (int i = 0; i < memory.num_strings; i++) {
        printf("%d: %s\n", i, memory.strings[i]);
    }
}


// Builtins
char *builtin_commands[] = {
    "exit",
    "cd",
    "path"
};

// Builtins redirection
int (*builtin_functions[])(int index) = {
    &exit_command,
    &cd_command,
    &path_command
};

// Main function
int main(int argc, char *argv[]) {
    // Initialize all global variables
    initialize_global();
    // Interactive mode
    if (argc == 1) {
        storage.stream = stdin;
        shell();
    }
    // Batch mode
    else if (argc == 2) {
        storage.stream = fopen(argv[1], "r");
        if (storage.stream == NULL) {
            error_exit_shell();
        }
        shell();
    }
    // Wrong inputs
    else {
        error_exit_shell();
    }

    // Free all global memory
    free_global();
    return 0;
}

/*
Initializes some necessary global variables
Param: 
    none
Return:
    none
*/
void initialize_global() {
    storage.stream = NULL;
    storage.paths = calloc(2, sizeof(char *));
    storage.paths[0] = calloc(5, sizeof(char));
    strcpy(storage.paths[0], "/bin");
    storage.paths[1] = calloc(9, sizeof(char));
    strcpy(storage.paths[1], "/usr/bin");
    storage.num_paths = 2;
}

/*
Initializes some necessary memory variables
Param: 
    none
Return:
    none
*/
void initialize_memory() {
    struct CommandLine command_line;
    command_line.size = 0;
    command_line.commands = NULL;
    memory.command_line = command_line;

    struct ExeCommand *executable_commands = NULL;
    memory.executable_commands = executable_commands;
    memory.num_valid_commands = 0;

    memory.strings = calloc(3, sizeof(char *));
    memory.num_strings = 3;
    memory.string_index = 0;

    memory.input = NULL; 
}

/*
Prints wish> if interactive, and reads in and runs each command line
Param: 
    none
Return:
    0 if shell did not encounter problems
*/
int shell() {
    while (1)
    {
        initialize_memory();
        // If interactive, print wish>
        if (storage.stream == stdin) {
            printf("wish> ");
        }

        // Read in lines
        size_t len = 0;
        if (getline(&memory.input, &len, storage.stream) != -1) {
            run_command_line(memory.input);
            free_memory();
        }
        // Otherwise we encounter EOF, so return to main
        else {
            free_memory();
            return 0;
        }
    }
    return 0;
}

/*
Function that parses and executes command
Param: 
    char *input: the command line
Return:
    0 if no problems
    -1 if a command line has errors
*/
int run_command_line(char *input) {
    char *buffered_input = NULL;

    // Buffer the operators with white space
    buffer_operators(input, &buffered_input);

    // Processing command line
    preprocess_command_line(buffered_input);

    // Process the commands
    if (preprocess_commands() == -1) {
        return -1; 
    }

    // Execute the commands
    execute_commands();
    return 0;
}

/*
Gets a command line and buffers all > and & operators with spaces
Param: 
    char *input: command line
    char **buffered_input: pointer to the new buffered command line
Return:
    0 
*/
int buffer_operators(char *input, char **buffered_input) {
    char *output = allocate_memory(strlen(input) * 2);
    int j = 0;
    for (int i = 0; i < strlen(input); i++) {
        if (input[i] == '>') {
            output[j] = ' ';
            j++;
            output[j] = input[i];
            j++;
            output[j] = ' ';
            j++;
        }
        else {
            output[j] = input[i];
            j++;
        }
    }
    *buffered_input = output;
    return 0;
}

/*
Parses out parallel commands 
Param: 
    char *buffered_input: the buffered command line
Return:
    0
*/
int preprocess_command_line(char *buffered_input) {
    memory.command_line.commands = calloc(strlen(buffered_input), sizeof(char*));

    // Use strtok to parse commands with &
    char *command = strtok(buffered_input, "&");
    int i = 0;
    memory.command_line.commands[i] = allocate_memory(strlen(buffered_input));
    memory.command_line.commands[i] = command;
    i++;
    while ((command = strtok(NULL, "&")) != NULL) {
        memory.command_line.commands[i] = command;
        i++; 
    }

    // This variable stores the number of parallel commands
    memory.command_line.size = i; 
    return 0;
}

/*
Parses commands, cleans white space and checks for redirection
Param: 
    none
Return:
    0: no problems
    -1: errors with redirection
*/
int preprocess_commands() {
    memory.executable_commands = (struct ExeCommand*) calloc(memory.command_line.size, sizeof(struct ExeCommand));
    for (int i = 0; i < memory.command_line.size; i++) {
        
        if (clean_and_split_arguments(memory.command_line.commands[i], i) == -1) {
            continue; 
        } 
        if (set_redirection(i) == -1) {
            return -1; 
        }

        // Keeps track of number of valid commands
        memory.num_valid_commands++;
    }
    return 0;
}

/*
Cleans the white space and splits arguments
Param: 
    char *command: a command
    int index: the command index (ex: 1st parallel command or 2nd parallel command, etc)
Return:
    0: no problems
    -1: The command is composed on entirely whitespace
*/
int clean_and_split_arguments(char *command, int index) {
    
    // Parse all white space out using strtok
    char delimit[] = " \t\r\n\v\f";
    char *token = strtok(command, delimit);
    if (token == NULL) {
        return -1; 
    }

    // Declare array to store all the arguments in a command
    memory.executable_commands[index].parsed_command = calloc(strlen(command) * 10, sizeof(char*));
    
    // For item, allocate space to store each argument as a string
    int i = 0;
    memory.executable_commands[index].parsed_command[i] = allocate_memory(strlen(command));

    strcpy(memory.executable_commands[index].parsed_command[i], token);

    while ((token = strtok(NULL, delimit)) != NULL) {
        i++;
        memory.executable_commands[index].parsed_command[i] = token; 
    }
    i++;
    memory.executable_commands[index].parsed_command[i] = NULL;
    memory.executable_commands[index].size = i;
    return 0;
}


/*
Checks for redirection and set the appropriate redirection flag
Param: 
    int index: the command index
Return:
    0: no problems
    -1: problems with redirection 
*/
int set_redirection(int index) {
    for (int i = 0; i < memory.executable_commands[index].size; i++) {
        if (strcmp(memory.executable_commands[index].parsed_command[i], ">") == 0) {
            // If redirection occurs too early 
            //     ls > text.txt text.txt
            // Or if redirection doesn't have any output file
            //     ls >
            // Return error
            if ((memory.executable_commands[index].size - i - 1) > 1 || memory.executable_commands[index].size < 3) {
                display_error_message();
                return -1; 
            }

            // If no error with redirection

            // Set flag to 1
            memory.executable_commands[index].redirection_flag = 1;

            // Save filename
            memory.executable_commands[index].redirection_filename = memory.executable_commands[index].parsed_command[i+1];
            
            // Get rid of redirection and the filename from the parsed command
            memory.executable_commands[index].parsed_command[i] = NULL;
            memory.executable_commands[index].parsed_command[i+1] = NULL;
            memory.executable_commands[index].size -= 2;
            return 0; 
        }  
    }

    // If there is not redirection

    // Set flag to 0
    memory.executable_commands[index].redirection_flag = 0;

    // Set filename to NULL
    memory.executable_commands[index].redirection_filename = NULL;
    return 0; 
}

/*
Determines if the command is an executable or builtin, and runs it
Param: 
    none
Return:
    none
*/
int execute_commands() {

    // Save default stdout and stderr
    int default_stdout = dup(STDOUT_FILENO);
    int default_stderr = dup(STDERR_FILENO); 
    int file_desc = 0;
    
    // For each valid command
    for (int i = 0; i < memory.num_valid_commands; i++) {

        // Check if the command is an executable command
        char *executable_path = is_executable(memory.executable_commands[i].parsed_command[0]);      
        
        // Check if the command is a builtin command
        int builtin_command = is_builtin(memory.executable_commands[i].parsed_command[0]);
        
        // Set all stdout and stderr to default first
        dup2(default_stdout, 1);
        dup2(default_stderr, 2);

        //set redirection
        if (memory.executable_commands[i].redirection_flag == 1) {
            char *file = allocate_memory(strlen(memory.executable_commands[i].redirection_filename)*2);
            strcpy(file, memory.executable_commands[i].redirection_filename);
            file_desc = open(file, O_TRUNC | O_RDWR | O_CREAT, S_IRWXU);
            dup2(file_desc, 1);
            dup2(file_desc, 2);
        }

        // If command is executable, then run it using fork and execv
        if (executable_path != NULL) {
            pid_t pid = fork();
            if (pid < 0) {
                exit(1);
            }
            else if (pid == 0) {
                execv(executable_path, memory.executable_commands[i].parsed_command);
            }
        }

        // Else if the command is a builtin, find the builtin command and run it
        else if (builtin_command >= 0) {
            (*builtin_functions[builtin_command])(i); 
        }
        else {
            display_error_message();
        }
    }

    // Parent has to wait for all children to finish
    for (int i = 0; i < memory.num_valid_commands; i++) {
        wait(NULL);

        // Reset stdout and stderr
        dup2(default_stdout, 1);
        dup2(default_stderr, 2);
    }
    return 0;
}

/*
Checks if a command is an executable command
Param: 
    char *argument: the first argument in a command
Return:
    executable_path: the path if the command exists in the one of the paths
    NULL: if the command does not exist in any specified path
*/
char *is_executable(char *argument) {
    for (int i = 0; i < storage.num_paths; i++) {
        char *executable_path = allocate_memory(strlen(argument)*10);
        strcpy(executable_path, storage.paths[i]);
        strcat(executable_path, "/");
        strcat(executable_path, argument);
        if (access(executable_path, X_OK) == 0) {
            return executable_path;
        }
    }
    return NULL;
}

/*
Checks if a command is a builtin command
Param: 
    char *argument: the first string of a command
Return:
    i: if it is a builtin, return which builtin command to run
    -1: the argument does not match any of the built in commandss
*/
int is_builtin(char *argument) {
    for (int i = 0; i < num_builtin_commands(); i++) {
        if (strcmp(argument, builtin_commands[i]) == 0) {
            return i;
        }
    }
    return -1; 
}

/*
Exit command
Param: 
    int index: which parallel command it is
Return:
    0: no errors
*/
int exit_command(int index) {

    // If exit isn't the last command, then print error
    // Ex: exit ls
    if (memory.executable_commands[index].size > 1) {
        display_error_message();
    }
    else {
        free_global();
        free_memory();
        
        exit(0);
    }
    return 0; 
}

/*
Cd command
Param: 
    int index: which parallel command it is
Return:
    0: no problems
*/
int cd_command(int index) {

    // If there are more than two commands in a cd command, then error
    // Ex: cd test test2
    if (memory.executable_commands[index].size > 2) {
        display_error_message();
    }

    // If only given cd, then return error
    else if (memory.executable_commands[index].size == 1) {
        display_error_message();
    }

    // Else, change directory using chdir
    else {
        int change_dir = chdir(memory.executable_commands[index].parsed_command[1]);
        if (change_dir != 0) {
            display_error_message();
        }
    }
    return 0;
}

/*
Changes the path
Param: 
    int index: which parallel command it is
Return:
    0: no problems
*/
int path_command(int index) {
    // Wipe out all existing paths
    if (storage.num_paths > 0) {
        for (int i = 0; i < storage.num_paths; i++)
        {
            free(storage.paths[i]);
        }
        free(storage.paths);
    }

    storage.paths = calloc(memory.executable_commands[index].size - 1, sizeof(char*));
    storage.num_paths = memory.executable_commands[index].size - 1;

    // Add new paths
    for (int i = 0; i < storage.num_paths; i++) {
        storage.paths[i] = calloc(strlen(memory.executable_commands[index].parsed_command[i+1]) + 1, sizeof(char*));
        strcpy(storage.paths[i], memory.executable_commands[index].parsed_command[i+1]);
    }
    return 0; 
}

/*
Returns total number of builtin commands
Param: 
    none
Return:
    number of builtin commands
*/
int num_builtin_commands() {
    return sizeof(builtin_commands) / sizeof(char *);
}

/*
Handles allocation of strings and saves each string to a struct to free later
Param: 
    int size: size of string to be allocated
Return:
    char *output: a pointer to a string
*/
char *allocate_memory(int size) {
    char *output = calloc(size*2, sizeof(char*));

    // Reallocate if needed
    if (memory.string_index + 2 == memory.num_strings) {  
        memory.num_strings = memory.num_strings * 2;
        memory.strings = realloc(memory.strings, memory.num_strings * sizeof(char*));
        
        // Initialize after reallocation
        for (int i = memory.string_index + 1; i < memory.num_strings; i++) {
            memory.strings[i] = NULL;
        }
        
    }

    // Save string to struct
    memory.strings[memory.string_index] = output;
    memory.string_index++;
    return output;
}

/*
Frees all global variables in GlobalStorage struct
Param: 
    none
Return:
    none
*/
void free_global() {
    // Free all paths
    if (storage.num_paths > 0) {
        for (int i = 0; i < storage.num_paths; i++) {
            free(storage.paths[i]);
        }
    }
    free(storage.paths);

    // Free stream
    if (storage.stream != NULL) {

        // Close stream
        fclose(storage.stream);
    }
}

/*
Frees all allocated memory in MemoryStorage struct
Param: 
    none
Return:
    none
*/
void free_memory() {

    // Free all the strings in memory.strings
    if (memory.num_strings > 0) {
        for (int i = 0; i < memory.num_strings; i++) {
            free(memory.strings[i]);
        }
        free(memory.strings);
    }

    // Free the commands
    if (memory.command_line.commands != NULL) {
        free(memory.command_line.commands);
    }

    if (memory.executable_commands != NULL) {
        for (int i = 0; i < memory.command_line.size; i++) {
            free(memory.executable_commands[i].parsed_command);
        }
        free(memory.executable_commands);
    }

    if (memory.input != NULL) {
        free(memory.input);
    }
}

/*
Function for errors that break shell
Exits with 1 and writes error message stderr
Param: 
    none
Return:
    none
*/
void error_exit_shell() {
    write(STDERR_FILENO, ERROR_MESSAGE, strlen(ERROR_MESSAGE));
    if (storage.stream != NULL) {
        fclose(storage.stream);
    }
    exit(1);
}

/*
Function that exists shell
Param: 
    none
Return:
    none
*/
void exit_shell() {
    if (storage.stream != NULL) {
        fclose(storage.stream);
    }
    exit(0);
}

/*
Function that displays error message
Param: 
    none
Return:
    none
*/
void display_error_message() {
    write(STDERR_FILENO, ERROR_MESSAGE, strlen(ERROR_MESSAGE));
}
