#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#define MATRON_SOCK_NAME "./tmp/matron_sock"
#define CRONE_SOCK_NAME "./tmp/crone_sock"
#define CLIENT_BUFSIZE 1024

// child process id's
pid_t matron_pid;
pid_t crone_pid;

// client thread id's
pthread_t matron_client_tid, matron_client_rx_tid;
pthread_t crone_client_tid;

// utility to initialize local socket data
void init_sock(struct sockaddr_un *addr, const char* name) {
  memset(addr, 0, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  sprintf(addr->sun_path, name);
}

// utilitiy to launch a joinable thread with no data
void launch_thread(pthread_t *tid, void *(*start_routine) (void *), void* data) {
  pthread_attr_t attr;
  int s;
  s = pthread_attr_init(&attr);
  if(s) { printf("error initializing thread attributes \n"); }
  s = pthread_create(tid, &attr, start_routine, data);
  if(s) { printf("error creating thread\n"); }
  pthread_attr_destroy(&attr);
}

// on connection to the matron client,
// launch the executable and redirect stdio to the connected socket
void launch_matron(int sock) {
  char *argv[] = { "matron", "57120", "8888", NULL};
  char *env[] = { NULL };

  // redirect stdout, stderr
  dup2( sock, STDOUT_FILENO );
  dup2( sock, STDERR_FILENO );
  // redirect stdin to the same socket... seems to work...
  dup2( sock, STDIN_FILENO );
  // no longer need the original handle
  close(sock);
	
  execv("matron/matron", argv);
  perror("execv");
}

// start a local socket serer wrapping the matron executable
void run_matron(void) {
  struct sockaddr_un addr;
  int sock, connect;
  int res;
  socklen_t addr_len;

  sock = socket(PF_UNIX, SOCK_STREAM, 0);
  if(sock <0) { printf("socket() failed \n"); return; }
  
  unlink(MATRON_SOCK_NAME);
  init_sock(&addr, MATRON_SOCK_NAME);
  
  res = bind(sock,
			 (struct sockaddr *) &addr, 
			 sizeof(struct sockaddr_un));
  if(res != 0) { printf("bind() failed \n"); return; }

  res = listen(sock, 5);
  if(res!= 0) { printf("listen() failed\n"); return; }

  // wait for the client to connect (we only care about the first connection)
  connect = accept(sock, (struct sockaddr *) &addr, &addr_len);
  if(connect > 0) { 
	launch_matron(connect);
  } else {
	printf("error connecting to matron server \n");
  }
  
}

// run the client that will glue stdin/stdout to matron by local socket

// rx thread
void* matron_client_rx(void* psock) {
  char rxbuf[CLIENT_BUFSIZE];
  int nb;
  int sock = *((int*)psock);
  while(1) { 
	// print server response
	nb = read(sock, rxbuf, CLIENT_BUFSIZE-1);
	if(nb > 0) {
	  rxbuf[nb] = '\0';
	  printf("%s", rxbuf);
	}
  }
}

// connect / tx thread
void* matron_client(void* x) {
  struct sockaddr_un addr;
  int sock;
  char txbuf[CLIENT_BUFSIZE];
  int res;
  int nb;
  int ch;
  int newline = 0;

  sock = socket(PF_UNIX, SOCK_STREAM, 0);
  if(sock < 0) {
	printf("run_matron_client(): socket() failed\n");
	return NULL;
  }
  
  init_sock(&addr, MATRON_SOCK_NAME);
  // wait a bit for the server
  usleep(100000);
  
  res = connect(sock,
				(struct sockaddr*)&addr,
				sizeof(struct sockaddr_un));
  if(res != 0) {
	printf("connect() failed \n");
	return NULL;
  }

  // launch the rx thread with the connected socket
  launch_thread(&matron_client_rx_tid, &matron_client_rx, &sock);  
  
  while(1) {
	// FIXME: weird issues with getline() and threads/children (?)
	// for now, doing this craziness instead
	nb = 0;
	txbuf[0] = '\0';
	newline = 0;
	//	printf("waiting for input\n");
	// read from stdin
	while(newline != 1) {
	  if(nb < (CLIENT_BUFSIZE-1)) {
		ch = getchar();
		txbuf[nb++] = (char)ch;
		if(ch == 10) { newline = 1; }
	  } else {
		newline = 1;
	  }
	}
	txbuf[nb] = '\0';	
	// send to server
	//	printf("sending %dB to server: \n%s\n", nb, txbuf);
	write(sock, txbuf, nb);
	//	printf("... sent\n");
  }
}

void run_crone(void) {
  // TODO
  while(1) { ;; }
}


void* crone_client(void* x) {
  // TODO
  while(1) { ;; }
}

int main(int argc, char** argv) {

  // fork a child process to run matron
  matron_pid = fork();
  if(matron_pid < 0) {
	printf("fork() returned an error\n");
	return 1;
  }
  if(matron_pid == 0) {
	run_matron();
	goto child_exit; // shouldn't get here
  }
  
  // parent continues.

  // fork child process to run crone
  crone_pid = fork();
  if(crone_pid < 0) {
	printf("fork() returned an error\n");
	return 1;
  }
  if(crone_pid == 0) {
	run_crone();
	goto child_exit; // shouldn't get here
  }

  // create threads to handle the children's i/o
  launch_thread(&matron_client_tid, &matron_client, NULL);
  launch_thread(&crone_client_tid, &crone_client, NULL);
  
  // wait for threads to exit
  pthread_join(matron_client_tid, NULL);
  pthread_join(crone_client_tid, NULL);
 
  // wait for child processes to exit
  wait(0);
  
  printf("norns parent exit\n");
  return 0;

 child_exit:
  printf("norns child exit. this is an error! \n");
  return 0;
}

