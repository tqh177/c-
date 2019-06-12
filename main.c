#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include "sock/server.h"
#include "config.h"

void shut_down(int sin)
{
   exit(0);
}

int main(void)
{
   config *con;
   signal(SIGINT, shut_down);
   server_init();
   con = getConfig();
   server_listen(con->ip, con->port);
   server_close();
   return 0;
}