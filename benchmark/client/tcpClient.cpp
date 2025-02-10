#include "ae.h"
#include "anet.h"

#include "TaskPool.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

const int servicePort = 6359;
const char serviceAddr[] = "127.0.0.1";
int serviceFd = 0;
TaskPool taskpool;

void readHandler(aeEventLoop *el, int fd, void *privdata, int mask)
{
    int nread, readlen = 128;
    char readbuf[128] = "";
    nread = read(fd, readbuf, readlen);
    if (nread == -1) {
    } else if (nread == 0) {

    }

    if (nread) {
        printf("read from fd:%d buf:%s\n", fd, readbuf);
        char writeBuf[128] = {0};
        printf("input write:");
        scanf("%s", writeBuf);
        int writeLen = anetWrite(fd, writeBuf, 128);
        printf("write len:%d\n", writeLen);
    }
}



int main()
{
    char neterr[128] = "";
    int setsize = 64;
    int backlog = 2;
    taskpool.Init();
    aeEventLoop* el = aeCreateEventLoop(setsize);

    int cifd = anetTcpConnect(neterr, const_cast<char*>(serviceAddr), servicePort);
    if (cifd == ANET_ERR) {
        printf("anetTcpConnect failed. %s\n", neterr);
        return -1;
    }

    if (cifd > 0 && aeCreateFileEvent(el, cifd, AE_READABLE, readHandler, NULL) == AE_ERR) {
        printf("aeCreateFileEvent failed.\n");
        return -1;
    }

    // taskpool.Submit([cifd](){
    //     char writeBuf[128] = {0};
    //     fscanf("input write:%s", writeBuf);
    //     int writeLen = anetWrite(fd, writeBuf, 128);
    //     printf("write len:%d\n", writeLen);
    // });

    aeMain(el);

    aeDeleteEventLoop(el);
    taskpool.Shutdown();
    return 0;
}