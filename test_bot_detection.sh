#!/bin/bash

# Bot Detection Testing Script for CatSurf
# Tests various bot patterns and rate limiting

PORT=8080
HOST="localhost:$PORT"
URL="http://$HOST/"

echo "======================================"
echo "CatSurf Bot Detection Test Suite"
echo "======================================"
echo ""

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test 1: Normal human behavior (should pass)
echo -e "${GREEN}Test 1: Normal Human Behavior${NC}"
echo "Making 5 requests with random delays (1-3 seconds)..."
for i in {1..5}; do
    response=$(curl -s -o /dev/null -w "%{http_code}" "$URL")
    echo "  Request $i: HTTP $response"
    sleep $((1 + RANDOM % 3))
done
echo ""

# Test 2: Rate limit test - rapid fire (should trigger 429)
echo -e "${YELLOW}Test 2: Rate Limit Test - Rapid Fire${NC}"
echo "Making 65 requests as fast as possible (should block at ~60)..."
blocked=0
for i in {1..65}; do
    response=$(curl -s -o /dev/null -w "%{http_code}" "$URL")
    if [ "$response" = "429" ]; then
        echo -e "  Request $i: ${RED}HTTP $response - BLOCKED!${NC}"
        blocked=$((blocked + 1))
    else
        echo "  Request $i: HTTP $response"
    fi
    sleep 0.01  # Minimal delay to avoid overload
done
echo "  Total blocked: $blocked"
echo ""

# Test 3: Mechanical timing pattern (should trigger bot detection)
echo -e "${YELLOW}Test 3: Mechanical Timing - Perfect Intervals${NC}"
echo "Making 12 requests with exactly 200ms intervals (bot pattern)..."
for i in {1..12}; do
    response=$(curl -s -o /dev/null -w "%{http_code}" "$URL")
    if [ "$response" = "429" ]; then
        echo -e "  Request $i: ${RED}HTTP $response - BLOCKED!${NC}"
    else
        echo "  Request $i: HTTP $response"
    fi
    sleep 0.2  # Exactly 200ms - mechanical pattern
done
echo ""

# Test 4: Ultra-fast burst (should trigger immediately)
echo -e "${YELLOW}Test 4: Ultra-Fast Burst${NC}"
echo "Making 10 requests with <50ms intervals..."
for i in {1..10}; do
    response=$(curl -s -o /dev/null -w "%{http_code}" "$URL")
    if [ "$response" = "429" ]; then
        echo -e "  Request $i: ${RED}HTTP $response - BLOCKED!${NC}"
    else
        echo "  Request $i: HTTP $response"
    fi
    sleep 0.05  # 50ms - ultra fast
done
echo ""

# Test 5: Bot User-Agent (should be marked suspicious)
echo -e "${YELLOW}Test 5: Bot User-Agent Test${NC}"
echo "Making requests with crawler user-agent..."
for i in {1..5}; do
    response=$(curl -s -o /dev/null -w "%{http_code}" -A "Mozilla/5.0 (compatible; Googlebot/2.1)" "$URL")
    if [ "$response" = "429" ]; then
        echo -e "  Request $i: ${RED}HTTP $response - BLOCKED!${NC}"
    else
        echo "  Request $i: HTTP $response"
    fi
    sleep 0.3
done
echo ""

# Test 6: Check Retry-After header
echo -e "${GREEN}Test 6: Check Retry-After Header${NC}"
echo "Triggering block and checking response headers..."
# Trigger rate limit
for i in {1..65}; do
    curl -s -o /dev/null "$URL" &
done
wait
sleep 0.5

# Check headers from blocked response
echo "Blocked response headers:"
curl -i -s "$URL" | grep -E "(HTTP|Retry-After|Content-Type)"
echo ""

# Test 7: Wait and retry (should unblock after timeout)
echo -e "${GREEN}Test 7: Wait for Unblock${NC}"
echo "Waiting 10 seconds for potential unblock..."
echo "(Note: Full unblock takes 5 minutes, but new clients may work)"
sleep 10
response=$(curl -s -o /dev/null -w "%{http_code}" "$URL")
echo "  After 10s wait: HTTP $response"
echo ""

echo "======================================"
echo "Test Suite Complete"
echo "======================================"
echo ""
echo "Expected Results:"
echo "  Test 1: All 200 OK (normal human behavior)"
echo "  Test 2: First ~60 requests OK, then 429 (rate limit)"
echo "  Test 3: May block after 8-12 requests (mechanical pattern)"
echo "  Test 4: May block after 3-5 requests (ultra-fast burst)"
echo "  Test 5: May get 200 but logged as suspicious"
echo "  Test 6: Should show 'Retry-After: 300' header"
echo "  Test 7: May still be blocked (5 min cooldown)"
