#include <assert.h>
#include <string.h>
#include <nanomsg/nn.h>
#include <unistd.h>
#include <nanomsg/pair.h>
#include <nanomsg/nn.h>
#include <nanomsg/ws.h>
#include <stdio.h>

int echo(int sock)
{
  char *buf = NULL;
  int result = nn_recv (sock, &buf, NN_MSG, 0);
  if (result > 0)
    {
      printf ("RECEIVED \"%s\" len: %d\n", buf, strlen(buf));
      nn_send(sock, buf, strlen(buf)+1, 0);
      printf ("SEND \"%s\"\n", buf);
      nn_freemsg (buf);
    }
  return result;
}

int main (const int argc, const char **argv)
{
  if (argc > 1) {
    int sock = nn_socket (AF_SP, NN_PAIR);
    assert (sock >= 0);
    assert (nn_bind (sock, argv[1]) >= 0);

    int to = 100;
    assert (nn_setsockopt (sock, NN_SOL_SOCKET, NN_RCVTIMEO, &to, sizeof (to)) >= 0);
    to = NN_WS_MSG_TYPE_TEXT;
    assert (nn_setsockopt (sock, NN_WS, NN_WS_MSG_TYPE, &to, sizeof (to)) >= 0);

    printf("serving\n");

    while(1)
        {
            echo(sock);
            sleep(0.1);
        }
  }
  else
  {
      fprintf (stderr, "Usage: pair <URL>\n");
      return 1;
  }
}
