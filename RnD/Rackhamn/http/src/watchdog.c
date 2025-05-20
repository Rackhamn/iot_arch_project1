#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

int main(int argc, char *argv[]) {
	printf(" ### WATCHDOG BEGIN ###\n");
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
		return 1;
	}

	while (1) {
		pid_t pid = fork();

		if (pid == 0) {
			// Child: exec the command
			execvp(argv[1], &argv[1]);
			perror("execvp failed");
			// exit(127);  // common code for command-not-found
			break;
		} else if (pid > 0) {
			// Parent: wait for child
			int status;
			waitpid(pid, &status, 0);

			if (WIFEXITED(status)) {
				int code = WEXITSTATUS(status);
				printf("Child exited with code %d\n", code);

				if (code == 0) {
					printf("Child exited cleanly. Watchdog will stop.\n");
					break;  // don't restart
				} else {
					printf("Restarting due to non-zero exit...\n");
				}
			} else if (WIFSIGNALED(status)) {
				int sig = WTERMSIG(status);
				printf("Child killed by signal %d (%s). Restarting...\n", sig, strsignal(sig));
			} else {
				printf("Child exited unexpectedly. Restarting...\n");
			}

			sleep(1); // prevent restart loops
		} else {
			perror("fork failed");
			return 1;
		}
	}
	
	printf(" ### WATCHDOG END ###\n");
	return 0;
}

