#!/bin/bash

# Simple Bot Simulator - Triggers rate limit detection
# Usage: ./bot_simulator.sh [requests_per_second] [total_requests]

RPS=${1:-20}        # Requests per second (default: 20)
TOTAL=${2:-100}     # Total requests (default: 100)
URL="http://localhost:8080/"

echo "Bot Simulator - Rapid Fire Test"
echo "================================"
echo "Target: $URL"
echo "Rate: $RPS requests/second"
echo "Total: $TOTAL requests"
echo ""

interval=$(echo "scale=3; 1.0 / $RPS" | bc)

blocked_count=0
success_count=0

for i in $(seq 1 $TOTAL); do
    response=$(curl -s -o /dev/null -w "%{http_code}" "$URL")
    
    if [ "$response" = "429" ]; then
        echo "[$i/$TOTAL] HTTP $response - BLOCKED ❌"
        blocked_count=$((blocked_count + 1))
    else
        echo "[$i/$TOTAL] HTTP $response - OK ✓"
        success_count=$((success_count + 1))
    fi
    
    sleep $interval
done

echo ""
echo "================================"
echo "Results:"
echo "  Successful: $success_count"
echo "  Blocked:    $blocked_count"
echo "  Total:      $TOTAL"
echo "================================"
