# Bot Detection Testing Guide

## Übersicht

Das CatSurf Session Management & Bot Detection System erkennt automatisch:
- **Rate Limiting**: > 60 Requests/Minute → HTTP 429
- **Mechanical Timing**: Perfekte Intervalle (z.B. exakt alle 250ms) → BLOCKED
- **Ultra-Fast Bursts**: < 100ms zwischen Requests → BLOCKED
- **Known Crawlers**: User-Agent enthält "bot", "crawler", etc. → SUSPICIOUS

## Test-Scripts

### 1. **test_bot_detection.sh** - Umfassende Test-Suite
Testet alle Bot-Detection-Szenarien:

```bash
# Server starten (Terminal 1)
./webserv config/catsurf.conf

# Tests ausführen (Terminal 2)
./test_bot_detection.sh
```

**Tests:**
- ✅ Normal Human Behavior (sollte OK sein)
- ⚠️ Rate Limit (65 Requests → ab ~60 geblockt)
- ⚠️ Mechanical Timing (perfekte 200ms Intervalle)
- ⚠️ Ultra-Fast Burst (50ms Intervalle)
- ⚠️ Bot User-Agent (Googlebot)
- 📋 Retry-After Header Check

---

### 2. **bot_simulator.sh** - Einfacher Rapid-Fire Test
Schneller Test für Rate Limiting:

```bash
# Standard: 20 req/s für 100 requests
./bot_simulator.sh

# Custom: 30 req/s für 200 requests
./bot_simulator.sh 30 200
```

**Erwartung:** Nach ~60 Requests in 60 Sekunden → HTTP 429

---

### 3. **test_bot_patterns.py** - Erweiterte Pattern-Analyse
Python-basierter Tester mit präzisen Timing-Patterns:

```bash
python3 test_bot_patterns.py
```

**Testet:**
1. 🤖 Mechanical Bot (250ms perfekte Intervalle)
2. 🤖 Ultra-Fast Burst (80ms Intervalle)
3. 🤖 Rate Limit (65 Requests)
4. 👤 Human-like (random 1-5s, sollte OK sein)
5. 🤖 Crawler User-Agent

---

## Manuelle Tests mit curl

### Rate Limit testen:
```bash
# 70 Requests so schnell wie möglich
for i in {1..70}; do 
    curl -s -o /dev/null -w "Request $i: %{http_code}\n" http://localhost:8080/
done
```

### Mechanical Timing testen:
```bash
# 15 Requests mit exakt 200ms Intervallen
for i in {1..15}; do 
    curl -s -o /dev/null -w "%{http_code}\n" http://localhost:8080/
    sleep 0.2
done
```

### Ultra-Fast Burst:
```bash
# 10 Requests mit 50ms Intervallen
for i in {1..10}; do 
    curl -s http://localhost:8080/ > /dev/null &
    sleep 0.05
done
```

### Bot User-Agent testen:
```bash
curl -A "Mozilla/5.0 (compatible; Googlebot/2.1)" http://localhost:8080/
```

### 429 Response ansehen:
```bash
curl -i http://localhost:8080/
# Nach Rate Limit:
# HTTP/1.1 429 Too Many Requests
# Retry-After: 300
# Content-Type: text/html
```

---

## Server Logs ansehen

Bot Detection Events werden nach `stderr` geloggt:

```bash
# Server mit Log-Output
./webserv config/catsurf.conf 2>&1 | grep -E "\[BOT_"

# Beispiel Output:
# [BOT_BLOCKED] IP=127.0.0.1 UA=curl/7.68.0 Reason=Rate limit exceeded: 65/60
# [BOT_SUSPICIOUS] IP=127.0.0.1 UA=Mozilla/5.0 (compatible; Googlebot/2.1) Score=55 Reason=Mechanical request pattern
```

---

## Erwartete Ergebnisse

| Szenario | Requests | Erwartung | HTTP Code |
|----------|----------|-----------|-----------|
| Normal Browsing | 5-10/min | ✅ OK | 200 |
| Rapid Fire | 65+ in 60s | ⚠️ BLOCKED | 429 |
| Mechanical (250ms) | 10-15 | ⚠️ BLOCKED | 429 |
| Ultra-Fast (<100ms) | 5-10 | ⚠️ BLOCKED | 429 |
| Crawler UA | Beliebig | ⚠️ SUSPICIOUS (logged) | 200* |
| Human Random | Beliebig | ✅ OK | 200 |

*Wird akzeptiert aber geloggt mit erhöhtem Suspicious Score

---

## Debugging

### Session Count prüfen:
Die Session-Manager-Instanz ist intern, aber du kannst indirekt testen:

```bash
# Viele verschiedene User-Agents erstellen
for i in {1..10}; do
    curl -A "TestBot-$i" http://localhost:8080/ &
done
```

### Memory Leak Check:
```bash
# Server starten
./webserv &
PID=$!

# Baseline Memory
ps aux | grep $PID

# 10.000+ Requests
for i in {1..10000}; do
    curl -s http://localhost:8080/ > /dev/null &
done
wait

# Check Memory danach
ps aux | grep $PID

# Sollte ~10MB für Sessions sein, nicht unbegrenzt wachsen
```

---

## Block-Dauer testen

```bash
# 1. Trigger Block
for i in {1..70}; do curl -s http://localhost:8080/ > /dev/null; done

# 2. Verify Block
curl -i http://localhost:8080/
# HTTP/1.1 429 Too Many Requests
# Retry-After: 300

# 3. Wait 5 minutes (300 seconds)
sleep 300

# 4. Retry
curl -i http://localhost:8080/
# Sollte wieder 200 OK sein (mit neuer Session)
```

---

## Troubleshooting

### Server startet nicht?
```bash
# Check OpenSSL Installation
ldconfig -p | grep -E "libssl|libcrypto"

# Re-compile
make re
```

### Keine Blocks trotz Tests?
- Server neugestartet? (Session State geht verloren)
- Config korrekt? Default: 60 req/min
- Logs prüfen: `./webserv 2>&1 | tee server.log`

### Zu aggressive Blocks?
Config ist im Code: `src/httpResponse/requesthandler.cpp`

```cpp
static BotDetection::BotDetectionConfig bot_config = BotDetection::getDefaultConfig();
bot_config.max_requests_per_minute = 120;  // Erhöhen auf 120
```

---

## Quick Start

```bash
# Terminal 1: Server starten
./webserv config/catsurf.conf

# Terminal 2: Schnelltest
./bot_simulator.sh 20 100

# Erwartung: ~60 Erfolge, ~40 Blocks
```
