#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main ()
{
   int sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
   struct sockaddr_in my_addr; /* my address information */
   struct sockaddr_in their_addr; /* connector's address information */
   socklen_t sin_size;
   if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
   {
      perror ("socket");
      exit (1);
   }
   memset (&their_addr, 0, sizeof (struct sockaddr));
   my_addr.sin_family = AF_INET; /* host byte order */
   my_addr.sin_port = htons (5690); /* short, network byte order */
   my_addr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */
   if (bind (sockfd, (struct sockaddr *) &my_addr, sizeof (struct sockaddr)) == -1)
   {
      perror ("bind");
      exit (1);
   }
   if (listen (sockfd, 100) == -1)
   {
      perror ("listen");
      exit (1);
   }
   int fpid = fork();
   while (1)
   {       /* main accept() loop */
      sin_size = sizeof (struct sockaddr_in);
      if ((new_fd = accept (sockfd, (struct sockaddr *) &their_addr, &sin_size)) == -1)
      {
         perror ("accept");
         continue;
      }
      printf ("server: got connection from %s in fpid=%d,pid=%d\n", inet_ntoa (their_addr.sin_addr), fpid, getpid());
      if (send (new_fd, "Hello, world!\n", 14, 0) == -1)
        perror ("send");
      close (new_fd);   /* parent doesn't need this */
   }

}
