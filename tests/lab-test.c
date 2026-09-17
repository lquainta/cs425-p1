#include <stdlib.h>
#include <string.h>
#include "harness/unity.h"
#include "../src/lab.h"

typedef struct { const char *input; size_t at; size_t chunk; char output[16384]; size_t out; int fail_write; } script;
static ssize_t scripted_read(void *v,char *b,size_t n){script *s=v;size_t left=strlen(s->input)-s->at,take=left<n?left:n;if(s->chunk&&take>s->chunk)take=s->chunk;if(!take)return 0;memcpy(b,s->input+s->at,take);s->at+=take;return (ssize_t)take;}
static ssize_t scripted_write(void *v,const char *b,size_t n){script *s=v;if(s->fail_write)return -1;if(n>3U)n=3U;memcpy(s->output+s->out,b,n);s->out+=n;s->output[s->out]='\0';return (ssize_t)n;}
static smtp_message message(void){return (smtp_message){"from@example.test","to@example.test","subject","one\n.starts here\n.","localhost"};}
void setUp(void){} void tearDown(void){}
void test_pure_helpers(void){char *s;TEST_ASSERT_EQUAL_INT(250,smtp_parse_reply_code("250 ok"));TEST_ASSERT_EQUAL_INT(-1,smtp_parse_reply_code("bad"));TEST_ASSERT_TRUE(smtp_reply_is_final("250 ok"));TEST_ASSERT_FALSE(smtp_reply_is_final("250-more"));TEST_ASSERT_TRUE(smtp_has_bare_newline("a\nb"));TEST_ASSERT_FALSE(smtp_has_bare_newline("a\r\nb"));s=smtp_build_command("HELO","host");TEST_ASSERT_EQUAL_STRING("HELO host\r\n",s);free(s);s=smtp_build_command("MAIL FROM:","a@example.test");TEST_ASSERT_EQUAL_STRING("MAIL FROM:<a@example.test>\r\n",s);free(s);s=smtp_dot_stuff(".x\ry");TEST_ASSERT_EQUAL_STRING("..x\r\ny\r\n",s);free(s);s=smtp_build_data_payload(&(smtp_message){"a","b","c",".x","h"});TEST_ASSERT_EQUAL_STRING("From: a\r\nTo: b\r\nSubject: c\r\n\r\n..x\r\n.\r\n",s);free(s);}
void test_reader_multiline_and_chunks(void){script s={"250-one\r\n250 two\r\n",0,2,"",0,0};smtp_reader r;char line[64];int code=0;smtp_reader_init(&r,(smtp_transport){scripted_read,scripted_write,&s});TEST_ASSERT_EQUAL_INT(0,smtp_read_reply(&r,&code,line,sizeof(line)));TEST_ASSERT_EQUAL_INT(250,code);TEST_ASSERT_EQUAL_STRING("250 two",line);}
void test_reader_failures(void){char too_long[SMTP_LINE_MAX+1U];script s; smtp_reader r;char line[8];memset(too_long,'x',SMTP_LINE_MAX);too_long[SMTP_LINE_MAX]='\0';s=(script){too_long,0,0,"",0,0};smtp_reader_init(&r,(smtp_transport){scripted_read,scripted_write,&s});TEST_ASSERT_EQUAL_INT(-1,smtp_read_line(&r,line,sizeof(line)));}
void test_session_happy_path(void){script s={"220 hi\r\n250-a\r\n250 ok\r\n250 ok\r\n250 ok\r\n354 go\r\n250 queued\r\n221 bye\r\n",0,1,"",0,0};char error[128];smtp_message m=message();TEST_ASSERT_EQUAL_INT(0,smtp_run_session((smtp_transport){scripted_read,scripted_write,&s},&m,error,sizeof(error)));TEST_ASSERT_EQUAL_STRING("HELO localhost\r\nMAIL FROM:<from@example.test>\r\nRCPT TO:<to@example.test>\r\nDATA\r\nFrom: from@example.test\r\nTo: to@example.test\r\nSubject: subject\r\n\r\none\r\n..starts here\r\n..\r\n.\r\nQUIT\r\n",s.output);}
void test_session_bad_each_reply(void){const char *replies[]={"500 no\r\n","220 hi\r\n550 no\r\n","220 hi\r\n250 ok\r\n550 no\r\n","220 hi\r\n250 ok\r\n250 ok\r\n550 no\r\n","220 hi\r\n250 ok\r\n250 ok\r\n250 ok\r\n550 no\r\n","220 hi\r\n250 ok\r\n250 ok\r\n250 ok\r\n354 go\r\n550 no\r\n","220 hi\r\n250 ok\r\n250 ok\r\n250 ok\r\n354 go\r\n250 ok\r\n550 no\r\n"};size_t i;char error[128];smtp_message m=message();for(i=0;i<sizeof(replies)/sizeof(replies[0]);++i){script s={replies[i],0,0,"",0,0};TEST_ASSERT_EQUAL_INT(-1,smtp_run_session((smtp_transport){scripted_read,scripted_write,&s},&m,error,sizeof(error)));TEST_ASSERT_NOT_NULL(strstr(error,"server sent"));}}
void test_session_hangup_and_invalid(void){script s={"220 hi\r\n",0,0,"",0,0};char error[128];smtp_message m=message();TEST_ASSERT_EQUAL_INT(-1,smtp_run_session((smtp_transport){scripted_read,scripted_write,&s},&m,error,sizeof(error)));m.subject="bad\nsubject";TEST_ASSERT_NULL(smtp_build_data_payload(&m));m=message();m.from="bad\nfrom";TEST_ASSERT_EQUAL_INT(-1,smtp_run_session((smtp_transport){scripted_read,scripted_write,&s},&m,error,sizeof(error)));}
void test_socket_transport_wrappers(void){
  int invalid_fd=-1; char buffer[8]={0};
  TEST_ASSERT_EQUAL_INT(-1,smtp_connect(NULL,"25"));
  TEST_ASSERT_EQUAL_INT(-1,smtp_socket_read(&invalid_fd,buffer,sizeof(buffer)));
  TEST_ASSERT_EQUAL_INT(-1,smtp_socket_write(&invalid_fd,"go",2));
  smtp_socket_close(invalid_fd);
}
int main(void){UNITY_BEGIN();RUN_TEST(test_pure_helpers);RUN_TEST(test_reader_multiline_and_chunks);RUN_TEST(test_reader_failures);RUN_TEST(test_session_happy_path);RUN_TEST(test_session_bad_each_reply);RUN_TEST(test_session_hangup_and_invalid);RUN_TEST(test_socket_transport_wrappers);return UNITY_END();}
