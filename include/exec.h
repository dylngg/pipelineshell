#ifndef EXEC_H
#define EXEC_H

/*
 * Runs a command and returns the exit code.
 */
int run(char* command, EnvStack* stack);

#endif  // EXEC_H
