# CatSurf

*This project has been created as part of the 42 curriculum by bbohle, jwardeng.*

CatSurf is a custom HTTP server written in C++17. It serves static files, supports virtual hosts, handles uploads, downloads and deletes, generates directory listings, and executes CGI scripts without blocking the main event loop (multiplexing).

The repository contains several small demo sites:

- `www/default` for the landing page
- `www/catshare` for uploads and file serving
- `www/catinder` for a CGI-driven Cat-Tinder

## Description

The goal of this project is to build a real web server from scratch and understand how HTTP works below the framework level. Instead of relying on an existing server such as Nginx or Apache, CatSurf implements its own configuration parsing, request parsing, routing, response generation, upload handling and CGI execution.

## Features

| Feature | Notes |
| --- | --- |
| Event-driven architecture | Uses a poller abstraction with `epoll`, `kqueue`, or `select` backends depending on platform support |
| Static file serving | Serves files from configured `root` directories with MIME type detection |
| Virtual hosts | Multiple `server` blocks can listen on different ports or hostnames |
| Upload support | `POST` uploads and `DELETE` cleanup for configured upload locations |
| CGI pipeline | Non-blocking CGI execution for `.py`, `.php`, and `.sh` scripts |
| Error handling | Custom error pages for common HTTP error codes |
| Demo frontends | Landing page, upload page, chat demo, and cat browser included in `www/` |

## Instructions

### Requirements

- `c++` with C++17 support
- `make`
- a Unix-like environment for the provided demo configs
- optional CGI:
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

You should start the server from the repository root when using the provided configs.

### Demo ports in `config/catsurf.conf`

| Port | Purpose |
| --- | --- |
| `8080` | Main demo site with static files, redirects, autoindex, CGI, uplaod |
| `7110` | `catinder` demo |
| `9110` | `catshare` upload & download demo |

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
cp config/minimal.con
```

Or create one from scratch:

```
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

    location /uploads {
        allow_methods GET POST DELETE;
        root uploads/mysite;
        upload_path uploads/mysite;
        autoindex on;
        botdetect no;
    }
}
```

### 3. Start the server with your config

```bash
./webserv config/mysite.conf
```

### Supported directives

Server block:

- `listen`
- `root`
- `index`
- `server_name`
- `error_page`
- `client_max_body_size`
- `timeout`
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

- `listen` accepts either port or ip:port
- `timeout`accepts plain seconds or units like `ms`, `s`, and `m`
- `root` and `upload_path` must point to directories that already exist
- `allow_methods` is configured per `location`
- `error_page` should point to a file inside the configured server root
- if you use CGI, the interpreter path in `cgi_path` must exist on your machine

## Project Layout

| Path | Purpose |
| --- | --- |
| `src/` | Server implementation, Parser, Router, CGI manager, Poller, Request/Response logic, BotDetection |
| `include/` | headers |
| `config/` | sample and test configurations |
| `www/` | static files, demos, CGI scripts, uploads, and error pages |
| `docs/` | design notes and grammar documentation |
| `Makefile` | build entry point |

## Resources

References used for this topic:

- RFC 9110, *HTTP Semantics*: https://www.rfc-editor.org/rfc/rfc9110.html
- RFC 9112, *HTTP/1.1*: https://www.rfc-editor.org/rfc/rfc9112.html
- RFC 3875, *The Common Gateway Interface (CGI) Version 1.1*: https://www.rfc-editor.org/rfc/rfc3875.html
- Nginx Beginner’s Guide for config concepts and terminology: https://nginx.org/en/docs/beginners_guide.html
- Parsing & State Machines
https://ptgmedia.pearsoncmg.com/images/0321112547/samplechapter/mertzch04.pdf
- Overview: **I/O Multiplexing (select vs. poll vs. epoll/kqueue)**
https://nima101.github.io/io_multiplexing
- Sources on Path traversal & symlink attacks
https://dev.to/godofgeeks/directory-traversal-attacks-5aj
[https://blogs.jsmon.sh/what-is-path-traversal-directory-traversal-ways-to-exploit-examples-and-impact/](https://www.invicti.com/learn/directory-traversal-path-traversal)
https://medium.com/@instatunnel/symlink-attacks-when-file-operations-betray-your-trust-986d5c761388
- URL encoding & decoding
https://www.codeguru.com/cplusplus/video-game-careers/

### AI usage

AI for documentation support and asset generation. Besides that it was also used for “brainstorming sessions” on specific topics to help deepen understanding.
