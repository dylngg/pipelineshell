#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "context.h"
#include "errors.h"
#include "exec.h"

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
}

