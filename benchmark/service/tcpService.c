#include "ae.h"
#include "anet.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

const int port = 6359;
const char *bindaddr = "127.0.0.1";
int sofd = -1;
int max = 2;

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

void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask)
{
    int cport, cfd;
    char cip[128];
    char neterr[128] = "";

    while(max--) {
        int cfd = anetTcpAccept(neterr, fd, cip, sizeof(cip), &cport);
        if (cfd == ANET_ERR) {
            if (errno != EWOULDBLOCK)
                printf("Accepting client connection: %s\n", neterr);
            return;
        }

        printf("Accepted %s connection to %s\n", cip, bindaddr);
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
    int setsize = 64;
    int backlog = 2;
    aeEventLoop* el = aeCreateEventLoop(setsize);

    int sofd = anetTcpServer(neterr, port, bindaddr, backlog);
    if (sofd == ANET_ERR) {
        printf("anetTcpServer failed. %s\n", neterr);
        return -1;
    }

    anetNonBlock(NULL, sofd);

    if (sofd > 0 && aeCreateFileEvent(el, sofd, AE_READABLE, acceptTcpHandler, NULL) == AE_ERR) {
        printf("aeCreateFileEvent failed.\n");
        return -1;
    }

    aeMain(el);

    aeDeleteEventLoop(el);
    return 0;
}

