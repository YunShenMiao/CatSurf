*This project has been created as part of the 42 curriculum by bbohle, jwardeng.*

# CatSurf

CatSurf is a custom HTTP server written in C++17 for the 42 `webserv` project. It serves static files, supports multiple virtual hosts, handles uploads and deletes, generates directory listings, and executes CGI scripts without blocking the main event loop.

The repository also contains several small demo sites used to exercise the server:

- `www/default` for the landing page
- `www/catshare` for uploads and file serving
- `www/WhatsCat` for a simple CGI-backed chat demo
- `www/catinder` for a CGI-driven cat Tinder

## Description

The goal of this project is to build a real web server from scratch and understand how HTTP works below the framework level. Instead of relying on an existing server such as Nginx or Apache, CatSurf implements its own request parsing, routing, configuration parser, response generation, upload handling, CGI execution, and timeout management.

At a high level, CatSurf provides:

- static file serving
- multiple `server` blocks with host/port matching
- `location`-based routing
- custom error pages
- directory listing with `autoindex`
- file uploads and deletion
- CGI support for Python, PHP, and shell scripts
- configurable request, CGI, and idle timeouts
- auto bot detection

## Features

| Feature | Notes |
| --- | --- |
| Event-driven architecture | Uses a poller abstraction with `epoll`, `kqueue`, or `select` backends depending on platform support |
| Static file serving | Serves files from configured `root` directories with MIME type detection |
| Virtual hosts | Multiple `server` blocks can listen on different ports or hostnames |
| Routing | Per-location behavior with `root`, `index`, `return`, `allow_methods`, and more |
| Upload support | `POST` uploads and `DELETE` cleanup for configured upload locations |
| CGI pipeline | Non-blocking CGI execution for `.py`, `.php`, and `.sh` scripts |
| Error handling | Custom error pages for common HTTP error codes |
| Demo frontends | Landing page, upload page, chat demo, and cat browser included in `www/` |

## Instructions

### Requirements

- `c++` with C++17 support
- `make`
- a Unix-like environment for the provided demo configs
- optional CGI runtimes:
  - `python3`
  - `php-cgi`
  - `/bin/sh`

### Build

From the repository root:

```bash
make
```

This creates the executable:

```bash
./webserv
```

### Run

Run the server with the default configuration:

```bash
./webserv
```

This is equivalent to:

```bash
./webserv config/default.conf
```

Run a different configuration:

```bash
./webserv config/cgi-demo.conf
```

### Important runtime note

The config parser resolves relative paths against the current working directory, not against the location of the config file itself. In practice, that means you should start the server from the repository root when using the provided configs.

### Demo ports in `config/default.conf`

| Port | Purpose |
| --- | --- |
| `7070` | CatSurf landing page in `www/default` |
| `8080` | Main demo site with static files, redirects, autoindex, CGI, and uploads |
| `8081` | Secondary simple static server |
| `7110` | `catinder` demo |
| `8110` | `WhatsCat` demo |
| `9110` | `catshare` upload demo |

### Quick checks

```bash
curl http://127.0.0.1:8080/
curl http://127.0.0.1:8080/stuff/
curl "http://127.0.0.1:8080/cgi-python/query_params.py?hello=world"
curl -X POST --data-binary @README.md http://127.0.0.1:8080/upload
```

### Optional Docker helper image

The repository includes a `Dockerfile` that installs common HTTP testing tools.

Build it:

```bash
docker build -t catsurf .
```

Run it with the repository mounted:

```bash
docker run -it --rm -p 8080:8080 -v "$(pwd)":/app -w /app catsurf
```

## Creating a New Config

If you want to add a new website, test a smaller setup, or create a dedicated CGI/upload server, create a new config file in `config/`.

### 1. Create the directories you want to serve

Example:

```bash
mkdir -p logs
mkdir -p www/mysite
mkdir -p www/mysite/error_pages
mkdir -p uploads/mysite
```

Add at least one index file:

```bash
printf '<h1>Hello from CatSurf</h1>\n' > www/mysite/index.html
printf '<h1>404 - Not Found</h1>\n' > www/mysite/error_pages/404.html
```

### 2. Create a new config file

Example:

```bash
cp config/minimal.conf config/mysite.conf
```

Or create one from scratch:

```nginx
error_log ./logs/error.log;
pid ./webserv.pid;

server
{
    listen 8082;
    server_name mysite.local;
    root www/mysite;
    index index.html;
    error_page 404 /error_pages/404.html;
    client_max_body_size 10M;
    timeout 30s;
    cgi_timeout 45s;
    cgi_idle_timeout 1500ms;

    location /uploads {
        allow_methods GET POST DELETE;
        root uploads/mysite;
        upload_path uploads/mysite;
        autoindex on;
        botdetect no;
    }

    location /cgi-python {
        root www/cgi-bin/python;
        allow_methods GET POST;
        cgi_extension .py;
        cgi_path /usr/bin/python3;
        client_max_body_size 1M;
        index echo.py;
    }
}
```

### 3. Start the server with your config

```bash
./webserv config/mysite.conf
```

### 4. Test it

```bash
curl http://127.0.0.1:8082/
curl http://127.0.0.1:8082/uploads/
curl "http://127.0.0.1:8082/cgi-python/echo.py?test=1"
```

### Supported directives

Global block:

- `error_log`
- `pid`

Server block:

- `listen`
- `root`
- `index`
- `server_name`
- `error_page`
- `client_max_body_size`
- `timeout`
- `cgi_timeout`
- `cgi_idle_timeout`
- `location`

Location block:

- `root`
- `autoindex`
- `index`
- `allow_methods`
- `upload_path`
- `cgi_extension`
- `cgi_path`
- `client_max_body_size`
- `return`
- `botdetect`

### Config tips

- `listen` accepts either `8080` or `127.0.0.1:8080`
- `timeout`, `cgi_timeout`, and `cgi_idle_timeout` accept plain seconds or units like `ms`, `s`, and `m`
- `root` and `upload_path` must point to directories that already exist
- `allow_methods` is configured per `location`
- `error_page` should point to a file inside the configured server root
- if you use CGI, the interpreter path in `cgi_path` must exist on your machine
- the example configs in `config/` are the fastest way to bootstrap a new setup

## Project Layout

| Path | Purpose |
| --- | --- |
| `src/` | server implementation, parser, router, CGI manager, poller, request/response logic |
| `include/` | headers |
| `config/` | sample and test configurations |
| `www/` | static files, demos, CGI scripts, uploads, and error pages |
| `docs/` | design notes and grammar documentation |
| `Makefile` | build entry point |

## Additional Documentation

- `docs/CGI.md` explains the CGI pipeline and example requests
- `docs/Config File Grammar Documentation.md` summarizes the config grammar
- `docs/HTTP-Request-route-Response.md` documents request flow and routing behavior

## Resources

Classic references used for this topic:

- RFC 9110, *HTTP Semantics*: <https://www.rfc-editor.org/rfc/rfc9110.html>
- RFC 9112, *HTTP/1.1*: <https://www.rfc-editor.org/rfc/rfc9112.html>
- RFC 3875, *The Common Gateway Interface (CGI) Version 1.1*: <https://www.rfc-editor.org/rfc/rfc3875.html>
- Nginx Beginner's Guide for config concepts and terminology: <https://nginx.org/en/docs/beginners_guide.html>

Project-specific references included in this repository:

- `docs/rfc9110.pdf`
- `docs/rfc9112.pdf`
- `docs/rfc1122.txt.pdf`
- `docs/rfc1123.txt.pdf`
- `docs/CGI.md`

### AI usage

AI was used for documentation support and asset generation.
