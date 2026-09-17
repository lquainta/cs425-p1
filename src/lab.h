#ifndef LAB_H
#define LAB_H

#include <stddef.h>
#include <sys/types.h>

#define SMTP_LINE_MAX 4096
typedef ssize_t (*smtp_read_fn)(void *context, char *buffer, size_t size);
typedef ssize_t (*smtp_write_fn)(void *context, const char *buffer, size_t size);
typedef struct { smtp_read_fn read; smtp_write_fn write; void *context; } smtp_transport;
typedef struct { smtp_transport transport; char buffer[SMTP_LINE_MAX]; size_t start; size_t end; } smtp_reader;
typedef struct { const char *from; const char *to; const char *subject; const char *body; const char *helo_host; } smtp_message;

/* Pure protocol helpers. Returned strings are owned by the caller. */
int smtp_parse_reply_code(const char *line);
int smtp_reply_is_final(const char *line);
int smtp_has_bare_newline(const char *text);
char *smtp_build_command(const char *verb, const char *argument);
char *smtp_dot_stuff(const char *body);
char *smtp_build_data_payload(const smtp_message *message);

/* Protocol session layer. */
void smtp_reader_init(smtp_reader *reader, smtp_transport transport);
int smtp_read_line(smtp_reader *reader, char *line, size_t line_size);
int smtp_read_reply(smtp_reader *reader, int *code, char *line, size_t line_size);
int smtp_write_all(smtp_transport transport, const char *data, size_t length);
int smtp_send_command(smtp_reader *reader, const char *command, int expected, char *error, size_t error_size);
int smtp_run_session(smtp_transport transport, const smtp_message *message, char *error, size_t error_size);

/* Socket transport layer. */
int smtp_connect(const char *host, const char *port);
ssize_t smtp_socket_read(void *context, char *buffer, size_t size);
ssize_t smtp_socket_write(void *context, const char *buffer, size_t size);
void smtp_socket_close(int socket_fd);
#endif
