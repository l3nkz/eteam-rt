#include <eteam.h>

#include <linux/types.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>


int start_energy(pid_t pid)
{
    int err;

    if (pid < 0) {
        errno = EINVAL;
        return -1;
    }

    err = syscall(SYS_start_energy, pid);
    if (err < 0) {
        errno = -err;
        return -1;
    }

    return 0;
}

int stop_energy(pid_t pid)
{
    int err;

    if (pid < 0) {
        errno = EINVAL;
        return -1;
    }

    err = syscall(SYS_stop_energy, pid);
    if (err < 0) {
        errno = -err;
        return -1;
    }

    return 0;
}

int consumed_energy(pid_t pid, struct energy *energy)
{
    char path[100];
    FILE *f;
    int ret;

    if (pid < 0 || !energy) {
        errno = EINVAL;
        return -1;
    } else if (pid == 0) {
        pid = getpid();
    }

    snprintf(path, 100, "/proc/%d/energystat", pid);

    f = fopen(path, "r");
    if (!f)
        return -1;

    ret = 0;
    if (fscanf(f, "%llu %llu %llu %llu", &energy->package, &energy->dram,
                &energy->core, &energy->gpu) != 4)
        ret = -1;

    fclose(f);

    return ret;
}
