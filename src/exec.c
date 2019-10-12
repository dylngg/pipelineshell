#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "context.h"
#include "errors.h"
#include "exec.h"
#include "utils.h"

exit_t run(char* command, EnvStack* stack) {
    Env* env = get_env(stack);
    char** argv = env->argv;
    pid_t pid;
    if ((pid = fork()) == -1) die_errno("Failed to fork");

    if (pid == 0) {
        // Child
        execvp(command, argv);
        die_errno("Failed to execvp");
    }
    else {
        // Parent process
        int exit_code = 0;
        waitpid(pid, &exit_code, 0);
        // FIXME: Get actual exit code
        return (exit_t) exit_code;
    }
    return 0;
Result *create_cmd_result(char *output, exit_t code, int out_fd) {
    Result *result = must_malloc(sizeof *result);
    result->output = must_strdup(output);
    result->code = code;
    result->out_fd = out_fd;
    return result;
}

Result *create_result(char *output) {
    return create_cmd_result(output, 0, STDOUT_FILENO);
}

Result *create_empty_result() {
    return create_result("");
}

void destroy_result(Result *result) {
    free(result->output);
    if (result->out_fd != STDOUT_FILENO) close(result->out_fd);
    free(result);
}

