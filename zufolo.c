#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

void usage(const char *progname)
{
    printf("Usage: %s [-n] -s /dev/ttyX -c command [-d] [-t timeout]\n", progname);
    exit(1);
}

void daemonize(void)
{
    int pid, dfd;

    if ((pid = fork()) < 0)
    {
        perror("fork");
        exit(1);
    } else if (pid > 0)
        exit(0);

    chdir("/");
    close(0);
    dfd = open("/dev/null", O_RDWR);
    dup2(dfd, 0);
    dup2(dfd, 1);
    dup2(dfd, 2);
}    

int main(int argc, char *argv[])
{
    int fd;
    int opt;
    int daemon = 0, dryrun = 0;
    int state;
    useconds_t timeout = 10000;
    const char *command = NULL;
    const char *port = NULL;

    while ((opt = getopt(argc, argv, "dnc:s:t:")) != -1)
    {
        switch (opt)
        {
            case 'd':
                daemon = 1;
                break;
            case 'n':
                dryrun = 1;
                break;
            case 'c':
                command = optarg;    
                break;
            case 's':
                port = optarg;
                break;
            case 't':
                timeout = atoi(optarg);
                break;
            default:
                usage(argv[0]);
        }
    }

    if (!port || (optind != argc))
        usage(argv[0]);

    if (!(command || dryrun))
        usage(argv[0]);

    if (daemon)
        daemonize();

    if ((fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
    {
        perror("open");
        return 1;
    }

    printf("Starting...\n");
    while (1)
    {
        if (ioctl(fd, TIOCMIWAIT, TIOCM_CAR) != 0)
        {
            perror("TIOCMIWAIT");
            continue;
        }

        usleep(timeout);

        state = 0;
        if (ioctl(fd, TIOCMGET, &state) != 0)
        {
            perror("TIOCMGET");
            continue;
        }

        if (state & TIOCM_CAR)
        {
            printf("TIOCM_CAR detected\n");
            if (!dryrun)
                system(command);
        }
        else
            printf("Spurious serial input\n");

        usleep(100000);
    }
    return 0;
}
