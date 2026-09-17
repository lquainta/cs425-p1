# CS425 P1 — Simple Mail Client

- Name: Landon Quaintance
- Email: landonquaintance@boisestate.edu
- Class: CS425

## Build and run

Run `make all`, then send a message with:

```sh
echo 'message body' | ./build/release/myapp -f me@example.test -t you@example.test -s hello -p 2525 server.example.test
```

The body may instead be supplied with `-b`. Run `make check`, `make report`,
`make leak`, and `make leak-test` to verify the project.

## Design

The project has three layers. Pure helpers parse SMTP reply syntax and construct
CRLF-terminated commands and DATA payloads (including dot stuffing); they have no
I/O. The session layer owns line buffering, multi-line reply handling, sequencing,
and status validation, but receives reads and writes through a transport callback
pair. The socket layer is a small adapter around `getaddrinfo`, `connect`, `recv`,
and `send`. This lets unit tests run complete SMTP conversations using a scripted
in-memory server, without a live mail server.

## Notes

The client uses HELO, does not negotiate TLS or authentication, and rejects bare
CR/LF in envelope and header inputs to prevent command/header injection.

## Known Bugs or Issues

None known. The assignment intentionally has no TLS or authentication support.

## Experience

The main design challenge was treating a socket as a byte stream: SMTP replies can
be split across reads or combined in one read. The buffered transport reader and
scripted tests make those cases repeatable without a live server.

## Analysis

The client verifies the expected SMTP status before advancing each stage, so a
failed greeting, envelope command, DATA command, queued-message reply, or QUIT
reply terminates the session with the received status in its error message.
