# CatSurf CGI Guide

This document explains how CatSurf's non‑blocking CGI pipeline works, how to configure interpreters, and how to test the bundled demo scripts.

## Features

- Fork/exec external interpreters (Python, PHP, POSIX sh, etc.) with per-request environment variables.
- STDIN/STDOUT/STDERR driven by the event loop; no blocking inside the main poller.
- Automatic header buffering and validation; streaming response bodies once headers are complete.
- Chunked transfer fallback when scripts omit `Content-Length` but the client connection stays open.
- PATH_INFO detection so URLs such as `/cgi-bin/echo.py/foo/bar` set:
  - `SCRIPT_NAME=/cgi-bin/echo.py`
  - `PATH_INFO=/foo/bar`
  - `SCRIPT_FILENAME=/absolute/path/to/www/cgi-bin/python/echo.py`
- Configurable `cgi_timeout` (absolute cap) and `cgi_idle_timeout` (inactivity window) per server block.
- Clean termination path (SIGTERM → short wait → SIGKILL) plus SIGCHLD/self-pipe reaping so no zombies remain.

## Configuration

Add CGI support inside a `location` block by pairing the script extensions and interpreter path:

```nginx
location /cgi-python {
    root www/cgi-bin/python;
    allow_methods GET POST;
    cgi_extension .py;
    cgi_path /usr/bin/python3;
    client_max_body_size 5M;
}
```

Server-wide CGI timing knobs:

```nginx
server {
    listen 8080;
    root www;
    cgi_timeout 45s;       # absolute execution ceiling
    cgi_idle_timeout 2s;   # inactivity window between reads/writes
    ...
}
```

Time directives accept `ms`, `s`, or `m` suffixes (`30` means 30 seconds).

### Sample config

`config/cgi-demo.conf` ships with three locations:

| Path | Interpreter | Description |
| ---- | ----------- | ----------- |
| `/cgi-python` | `/usr/bin/python3` | Runs `python/echo.py`, echoes env vars + body |
| `/cgi-php` | `/usr/bin/php-cgi` | Runs `php/upload.php`, dumps POST metadata |
| `/cgi-sh` | `/bin/sh` | Runs `sh/date.sh`, demonstrates scripts without `Content-Length` |

Each location sets explicit `cgi_extension`, `root`, and timeout overrides. Use it via `./webserv config/cgi-demo.conf`.

> **Note:** Adjust the interpreter paths (`/usr/bin/python3`, `/usr/bin/php-cgi`, `/bin/sh`) to match your platform before running the demo.
> For PHP support you need the `php-cgi` binary installed (e.g., `sudo apt install php-cgi` on Debian/Ubuntu).

## Environment variables

Key variables provided to every CGI process:

| Variable | Contents |
| -------- | -------- |
| `GATEWAY_INTERFACE` | `CGI/1.1` |
| `REQUEST_METHOD` | e.g., `GET`, `POST` |
| `QUERY_STRING` | Portion after `?` (no leading `?`) |
| `CONTENT_LENGTH` | Decimal length if a body is present |
| `CONTENT_TYPE` | Forwarded from the HTTP header if provided |
| `SERVER_PROTOCOL` | `HTTP/1.1` or `HTTP/1.0` |
| `SERVER_NAME` / `SERVER_PORT` | From the matched server block |
| `DOCUMENT_ROOT` | Server root |
| `REMOTE_ADDR` / `REMOTE_PORT` | Client IP/port |
| `SCRIPT_NAME` | URI portion that identifies the script |
| `SCRIPT_FILENAME` | Filesystem path to the script |
| `PATH_INFO` | Remaining URI segments after the script |
| `PATH_TRANSLATED` | `SCRIPT_FILENAME + PATH_INFO` |
| `REQUEST_URI` | Original request URI |
| `REDIRECT_STATUS` | `200` (helps PHP-CGI behave like FastCGI) |
| `HTTP_*` | Every remaining HTTP header, uppercased and `_` separated |

`HTTPS=on` is set when the request used `https://` (future TLS work), otherwise omitted.

## Running the demo scripts

Create executable permissions once (on Unix/macOS):

```bash
chmod +x www/cgi-bin/python/echo.py
chmod +x www/cgi-bin/php/upload.php
chmod +x www/cgi-bin/sh/date.sh
```

Launch the server with the demo config:

```bash
./webserv config/cgi-demo.conf
```

Example requests:

1. **Python echo with PATH_INFO**
   ```bash
   curl -v "http://127.0.0.1:8080/cgi-python/echo.py/foo/bar?hello=world" \
        -H "X-Debug: catsurf" \
        -d "sample body"
   ```
   The response dumps env variables, the decoded path info, and the POST body. The script also reads `assets/data.txt` to prove relative file access works.

2. **Chunked upload to PHP CGI**
   ```bash
   printf 'hello world' | \
     curl -v --http1.1 \
          -H "Transfer-Encoding: chunked" \
          -H "Content-Type: text/plain" \
          --data-binary @- \
          http://127.0.0.1:8080/cgi-php/upload.php
   ```
   `curl` handles the chunk framing; send only the raw payload. CatSurf removes `Transfer-Encoding`, injects the computed `Content-Length`, and the PHP script (reading `php://input`) prints the body under “First 256 bytes of body”.

3. **Shell CGI without Content-Length**
   ```bash
   curl -v http://127.0.0.1:8080/cgi-sh/date.sh
   ```
   The response streams until EOF; because the script omits `Content-Length`, CatSurf switches to chunked transfer for HTTP/1.1 clients and closes HTTP/1.0 connections after the body.

4. **Timeout test**
   ```bash
   curl -v http://127.0.0.1:8080/cgi-python/echo.py?sleep=1
   ```
   Adjust the script to `time.sleep(10)` to trigger `cgi_timeout`. The server responds with `504 Gateway Timeout` and logs the termination.

### Additional manual tests

| Scenario | Command | Expected outcome |
| --- | --- | --- |
| Chunked POST (entchunking) | `printf 'hello world' \| curl -v --http1.1 -H "Transfer-Encoding: chunked" -H "Content-Type: text/plain" --data-binary @- http://127.0.0.1:8080/cgi-php/upload.php` | Server strips `Transfer-Encoding`, sets `Content-Length` to the decoded size, and PHP prints the payload under "First 256 bytes of body". |
| Idle timeout | `curl -v "http://127.0.0.1:8080/cgi-python/echo.py?sleep=10"` with `cgi_idle_timeout 1500ms;` | CGI manager kills the script after ~1.5 s of inactivity and returns `504 Gateway Timeout`. |
| Absolute timeout | `curl -v "http://127.0.0.1:8080/cgi-python/echo.py?sleep=60"` with `cgi_timeout 30s;` | Process is terminated around the configured limit, client receives 504. |
| Query parsing per language | `curl -v "http://127.0.0.1:8080/cgi-php/query_params.php?foo=1&foo=2&bar=baz"` (analog für `/cgi-python/...` und `/cgi-sh/...`) | Response lists raw `QUERY_STRING` plus decoded key/value pairs, confirming PATH_INFO + env handling. |

> CatSurf removes the original `Transfer-Encoding` header before invoking CGI scripts and injects a computed `CONTENT_LENGTH` when the body was chunked. This matches the CGI specification and allows runtimes such as PHP (`php://stdin`) to read the entire request body without re‑implementing chunked decoding.

## Logging & troubleshooting

- CGI `stderr` is captured on a dedicated non-blocking pipe and logged to the server's stderr (`[CGI <pid>] ...`). It never leaks into the HTTP response stream.
- If a script exits without sending valid headers, CatSurf returns `502 Bad Gateway`.
- Invalid/missing interpreters or filesystem permission errors also produce `502` along with an error log.
- When CGI output lacks `Content-Length`, CatSurf only keeps the connection alive if the client negotiated HTTP/1.1 keep-alive; otherwise the connection closes after EOF.

## Testing checklist

- [X] GET/POST requests for each interpreter.
- [X] PATH_INFO variations (`/script.py/foo/bar`).
- [X] Chunked request bodies.
- [X] Scripts that emit `Status: 404 Something` (verify response line changes).
- [X] Scripts that emit `Location: /other` without `Status` (server falls back to 302).
- [X] Large POST near `client_max_body_size` (server rejects before spawning CGI).
- [X] Timeout behavior: script sleeps past `cgi_idle_timeout` and `cgi_timeout`.
- [X] Client disconnect mid-stream (`curl ...; kill %curl`) to confirm the CGI is terminated.
- [X] Concurrent CGI requests while static files continue to serve.

Use `config/cgi-demo.conf` together with the scripts in `www/cgi-bin/` for quick verification or adapt the examples to your own interpreters.




