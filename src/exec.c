#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "context.h"
#include "errors.h"
#include "exec.h"
#include "utils.h"

#define IN 0
#define OUT 1

// Used for blocking SIGCHLD
static sigset_t blocked;

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

Result *pipeline_cmds(EnvStack *stack, int ncmds) {
    assert(ncmds > 0);
    exit_t code = 0;
    int fd[2];
    int prev_fd = 0;
    pid_t pids[ncmds];

    // Block SIGCHLD signals from reaching parent until after we get to the
    // code to process signals. This way we can safely do a waitpid with the
    // assumption that the child hasn't sent a SIGCHLD before we could process
    // it.
    sigaddset(&blocked, SIGCHLD);
    sigprocmask(SIG_BLOCK, &blocked, NULL);

    for (int i = 0; i < ncmds; i++) {
        Env* env = get_env(stack);
        char** argv = env->argv;
        assert(argv[0] != NULL);
        // print_argv(argv);
        // TODO: Handle lambdas here (output = ...)

        pipe(fd);
        pids[i] = fork();
        if (pids[i] == -1) die_errno("Failed to fork");
        if (pids[i] == 0) {
            // Child
            dup2(prev_fd, STDIN_FILENO);

            // If next command, setup future pipe
            if (i < ncmds - 1) dup2(fd[OUT], STDOUT_FILENO);

            close(fd[IN]);
            if (execvp(argv[0], argv) == -1)
                die_errno(argv[0]);
        }
        else {
            // Parent
            close(fd[OUT]);
            prev_fd = fd[IN];
            pop_stack(stack);
        }
    }

    // Unblock here
    sigprocmask(SIG_UNBLOCK, &blocked, NULL);

    int status;
    pid_t dead_pid;
    int ndead = 0;
    do {
        dead_pid = waitpid(0, &status, WNOHANG | WUNTRACED);
        if (dead_pid == -1) break;  // In case no dead children

        // If last command, get exit code
        if (WIFEXITED(status) && dead_pid == pids[ncmds - 1])
            code = WEXITSTATUS(status);

        // FIXME: Handle signals
        // FIXME: Handle possability of core dump and other exit statuses

        for (int j = 0; j < ncmds; j++) {
            if (dead_pid == pids[j]) {
                ndead++;
                break;
            }
        }
    } while(ndead != ncmds);
    return create_cmd_result("", code, prev_fd);
}
