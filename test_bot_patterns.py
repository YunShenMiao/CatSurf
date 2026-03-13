#!/usr/bin/env python3
"""
Advanced Bot Pattern Simulator
Tests different timing patterns to trigger bot detection
"""

import requests
import time
import sys
from datetime import datetime

URL = "http://localhost:8080/"
SESSION = requests.Session()

def log(msg, color=""):
    colors = {
        "red": "\033[0;31m",
        "green": "\033[0;32m",
        "yellow": "\033[1;33m",
        "reset": "\033[0m"
    }
    c = colors.get(color, "")
    reset = colors["reset"] if c else ""
    print(f"{c}{msg}{reset}")

def test_pattern(name, description, requests_list):
    """
    Test a specific timing pattern
    requests_list: list of (delay_before_request_in_ms, user_agent)
    """
    log(f"\n{'='*60}", "yellow")
    log(f"Test: {name}", "yellow")
    log(f"Description: {description}", "yellow")
    log(f"{'='*60}", "yellow")
    
    blocked = 0
    success = 0
    
    for i, (delay_ms, user_agent) in enumerate(requests_list, 1):
        if delay_ms > 0:
            time.sleep(delay_ms / 1000.0)
        
        headers = {"User-Agent": user_agent} if user_agent else {}
        
        try:
            start = time.time()
            resp = SESSION.get(URL, headers=headers, timeout=5)
            elapsed = (time.time() - start) * 1000
            
            if resp.status_code == 429:
                log(f"  [{i:2d}] HTTP {resp.status_code} - BLOCKED ❌ ({elapsed:.0f}ms)", "red")
                blocked += 1
                
                # Check Retry-After header
                retry_after = resp.headers.get('Retry-After')
                if retry_after:
                    log(f"       Retry-After: {retry_after} seconds", "red")
            else:
                log(f"  [{i:2d}] HTTP {resp.status_code} - OK ✓ ({elapsed:.0f}ms)", "green")
                success += 1
                
        except requests.exceptions.RequestException as e:
            log(f"  [{i:2d}] ERROR: {e}", "red")
    
    log(f"\nResult: {success} success, {blocked} blocked", "yellow")
    return blocked > 0

def main():
    log("="*60, "green")
    log("CatSurf Bot Detection - Advanced Pattern Tests", "green")
    log("="*60, "green")
    
    # Pattern 1: Mechanical Regular Intervals (every 250ms)
    log("\n🤖 Pattern 1: Mechanical Bot - Perfect 250ms intervals")
    pattern_mechanical = [(250, None) for _ in range(15)]
    test_pattern(
        "Mechanical Bot",
        "Perfect regular intervals at 250ms (bot signature)",
        pattern_mechanical
    )
    
    time.sleep(2)
    
    # Pattern 2: Ultra-Fast Burst
    log("\n🤖 Pattern 2: Ultra-Fast Burst - < 100ms intervals")
    pattern_burst = [(80, None) for _ in range(10)]
    test_pattern(
        "Ultra-Fast Burst",
        "Superhuman speed: 80ms between requests",
        pattern_burst
    )
    
    time.sleep(2)
    
    # Pattern 3: Rate Limit Trigger
    log("\n🤖 Pattern 3: Rate Limit - 65 requests in 60 seconds")
    pattern_ratelimit = [(900, None) for _ in range(65)]
    test_pattern(
        "Rate Limit",
        "Exceeding 60 requests/minute threshold",
        pattern_ratelimit
    )
    
    time.sleep(2)
    
    # Pattern 4: Human-like (should NOT block)
    log("\n👤 Pattern 4: Human-like - Random intervals 1-5 seconds")
    import random
    pattern_human = [(random.randint(1000, 5000), "Mozilla/5.0 (X11; Linux x86_64) Chrome/120.0") for _ in range(10)]
    test_pattern(
        "Human Behavior",
        "Random intervals with high variance (normal user)",
        pattern_human
    )
    
    time.sleep(2)
    
    # Pattern 5: Known Crawler User-Agent
    log("\n🤖 Pattern 5: Crawler User-Agent")
    pattern_crawler = [(500, "Mozilla/5.0 (compatible; Googlebot/2.1)") for _ in range(10)]
    test_pattern(
        "Crawler Detection",
        "Known bot user-agent with moderate rate",
        pattern_crawler
    )
    
    log("\n" + "="*60, "green")
    log("All tests complete!", "green")
    log("="*60, "green")
    log("\nCheck server logs for [BOT_BLOCKED] and [BOT_SUSPICIOUS] messages")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        log("\n\nTest interrupted by user", "yellow")
        sys.exit(0)
