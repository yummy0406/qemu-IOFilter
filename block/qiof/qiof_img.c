#include <stdlib.h>
#include "qiof_img.h"
#include "qiof_log.h"

int QIOF_img_cp(uint32_t size, char **argv)
{
    // char cmd[100];
    // sprintf(cmd, "qemu-img convert -p %s %s", argv[0], argv[1]);
    // FILE *fp = popen(cmd, "r");
    // if (!fp)
    // {
    //     QIOF_LOG("open error\n");
    //     return -1;
    // }
    // pclose(fp);
    // return 0;
    if (size < 2 || !argv[0] || !argv[1])
    {
        QIOF_LOG("Invalid arguments\n");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        QIOF_LOG("Fork failed\n");
        return -1;
    }

    if (pid == 0)
    {
        // Child process
        char *cmd[] = {"qemu-img", "convert", "-p", argv[0], argv[1], NULL};
        execvp(cmd[0], cmd);

        // If execvp fails
        QIOF_LOG("execvp failed\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process: wait for the child process to complete
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            return 0; // Success
        }
        else
        {
            QIOF_LOG("Command failed with status %d\n", WEXITSTATUS(status));
            return -1;
        }
    }
}
