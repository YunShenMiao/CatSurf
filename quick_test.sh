#!/bin/bash

# Quick Rate Limit Test - 70 rapid requests
echo "Testing Rate Limit: 70 rapid requests"
echo "======================================"

blocked=0
success=0

for i in {1..70}; do
    code=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/)
    
    if [ "$code" = "429" ]; then
        echo "Request $i: BLOCKED (429) ❌"
        blocked=$((blocked + 1))
    else
        echo "Request $i: OK ($code) ✓"
        success=$((success + 1))
    fi
done

echo ""
echo "Results:"
echo "  Success: $success"
echo "  Blocked: $blocked"
echo ""

if [ $blocked -gt 0 ]; then
    echo "✅ Bot detection WORKING - some requests blocked!"
else
    echo "❌ Bot detection NOT WORKING - no blocks detected"
fi
