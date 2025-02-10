#include "ae.h"
#include "anet.h"


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

aeEventLoop* el = NULL;
int sofd = -1;
int max = 2;
char unixsocket[] = "./redis.sock";

void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask)
{
    int nread, readlen = 128;
    char readbuf[128] = "";
    nread = read(fd, readbuf, readlen);
    if (nread == -1) {
    } else if (nread == 0) {

    }

    if (nread) {
        printf("read from fd:%d buf:%s\n", fd, readbuf);
    }
}


void acceptUnixHandler(aeEventLoop *el, int fd, void *privdata, int mask)
{
    char neterr[128] = "";

    while(max--) {
        int cfd = anetUnixAccept(neterr, fd);
        if (cfd == ANET_ERR) {
            if (errno != EWOULDBLOCK)
                printf("Accepting client connection: %s\n",
            neterr);
            return;
        }

        printf("Accepted connection to %s\n", unixsocket);
        if (cfd != -1) {
            anetNonBlock(NULL, cfd);
            anetEnableTcpNoDelay(NULL, cfd);

            if (aeCreateFileEvent(el ,cfd, AE_READABLE, readQueryFromClient, NULL) == AE_ERR) {
                close(cfd);

                return;
            }
        }
    }
}


int main()
{
    char neterr[128] = "";

    mode_t unixsocketperm = 666;
    int backlog = 2;
    printf("lhh\n");
    int setsize = 64;
    aeEventLoop* el = aeCreateEventLoop(setsize);

    sofd = anetUnixServer(neterr, unixsocket, unixsocketperm, backlog);
    if (sofd == ANET_ERR) {
        printf("anetUnixServer failed. %s\n", neterr);
        return -1;
    }

    anetNonBlock(NULL, sofd);

    if (sofd > 0 && aeCreateFileEvent(el, sofd, AE_READABLE, acceptUnixHandler, NULL) == AE_ERR) {
        printf("aeCreateFileEvent failed.\n");
        return -1;
    }

    aeMain(el);

    aeDeleteEventLoop(el);

    return 0;
}