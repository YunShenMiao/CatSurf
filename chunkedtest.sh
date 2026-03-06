#!/bin/bash

# Test 1: Einfacher Chunked Request
echo "=== Test 1: Simple Chunked ==="
(
  printf "POST /cgi-python/echo.py HTTP/1.1\r\n"
  printf "Host: catsurf.com\r\n"
  printf "Transfer-Encoding: chunked\r\n"
  printf "Connection: close\r\n"
  printf "\r\n"
  printf "5\r\n"
  printf "Hello\r\n"
  printf "6\r\n"
  printf " World\r\n"
  printf "0\r\n"
  printf "\r\n"
) | nc 127.0.0.1 8080

# Test 2: Größerer Chunked Body (Größe-Check)
echo ""
echo "=== Test 2: Large Chunked (should work) ==="
(
  printf "POST /cgi-python/echo.py HTTP/1.1\r\n"
  printf "Host: catsurf.com\r\n"
  printf "Transfer-Encoding: chunked\r\n"
  printf "Connection: close\r\n"
  printf "\r\n"
  printf "7d0\r\n"
  printf "%0.s#" {1..2000}  # 2000 Zeichen
  printf "\r\n"
  printf "0\r\n"
  printf "\r\n"
) | nc 127.0.0.1 8080

# Test 3: Ungültiger Chunk (sollte BadRequest geben)
echo ""
echo "=== Test 3: Invalid Chunk Size ==="
(
  printf "POST /cgi-python/echo.py HTTP/1.1\r\n"
  printf "Host: catsurf.com\r\n"
  printf "Transfer-Encoding: chunked\r\n"
  printf "Connection: close\r\n"
  printf "\r\n"
  printf "GGGG\r\n"  # Ungültiges Hex
  printf "test\r\n"
  printf "0\r\n"
  printf "\r\n"
) | nc 127.0.0.1 8080

echo ""
echo "=== Tests completed ==="