#include "lab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef TEST
#define main main_exclude
#endif
static void usage(FILE *s){fprintf(s,"Usage: myapp -f <from> -t <to> [-s subject] [-b body] [-p port]\n          [-H helo-host] <server>\n");}
static char *read_stdin(void){char *body=NULL,block[1024];size_t used=0U,count;while((count=fread(block,1U,sizeof(block),stdin))){char *next=realloc(body,used+count+1U);if(!next){free(body);return NULL;}body=next;memcpy(body+used,block,count);used+=count;}if(ferror(stdin)){free(body);return NULL;}if(!body)body=malloc(1U);if(body)body[used]='\0';return body;}
int main(int argc,char **argv){int opt,fd,status;const char *from=NULL,*to=NULL,*subject="",*body_arg=NULL,*port="25",*helo="localhost",*server;char *body,error[256];smtp_message m;smtp_transport t;if(argc==1){usage(stdout);return 0;}while((opt=getopt(argc,argv,"f:t:s:b:p:H:"))!=-1){switch(opt){case 'f':from=optarg;break;case 't':to=optarg;break;case 's':subject=optarg;break;case 'b':body_arg=optarg;break;case 'p':port=optarg;break;case 'H':helo=optarg;break;default:usage(stderr);return 1;}}if(!from||!to||optind!=argc-1){usage(stderr);return 1;}server=argv[optind];body=body_arg?strdup(body_arg):read_stdin();if(!body){fprintf(stderr,"Unable to read message body\n");return 2;}m=(smtp_message){from,to,subject,body,helo};if(strpbrk(from,"\r\n")||strpbrk(to,"\r\n")||strpbrk(subject,"\r\n")||strpbrk(helo,"\r\n")){fprintf(stderr,"Address, subject, and HELO host must not contain CR or LF\n");free(body);return 1;}fd=smtp_connect(server,port);if(fd<0){fprintf(stderr,"Unable to connect to %s:%s\n",server,port);free(body);return 2;}t=(smtp_transport){smtp_socket_read,smtp_socket_write,&fd};status=smtp_run_session(t,&m,error,sizeof(error));smtp_socket_close(fd);free(body);if(status){fprintf(stderr,"%s\n",error);return 2;}return 0;}
