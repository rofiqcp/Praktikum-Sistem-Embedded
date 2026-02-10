#!/bin/bash
# Quick test script - Build a sample of programs to verify setup
# For full testing, use: python3 test_all_programs.py

echo "🔍 Quick Build Test - Testing Sample Programs"
echo "=============================================="

# Test a few representative programs
SAMPLE_PROJECTS=(
    "Modul-01-GPIO-Digital-IO/praktikum/STM32/STM32_01_LED_Blink"
    "Modul-01-GPIO-Digital-IO/praktikum/ESP32/ESP32_01_LED_Blink"
    "Modul-09-FreeRTOS-Task/praktikum/STM32/STM32_01_Task_Creation"
    "Modul-09-FreeRTOS-Task/praktikum/ESP32/ESP32_01_Task_Creation"
)

PASSED=0
FAILED=0
TOTAL=${#SAMPLE_PROJECTS[@]}

for PROJECT in "${SAMPLE_PROJECTS[@]}"; do
    echo ""
    echo "Testing: $PROJECT"
    echo "----------------------------------------"
    
    if [ -d "$PROJECT" ]; then
        cd "$PROJECT" || continue
        
        if pio run --silent-errors 2>&1 | grep -q "SUCCESS"; then
            echo "✅ PASSED"
            ((PASSED++))
        else
            echo "❌ FAILED"
            ((FAILED++))
        fi
        
        cd - > /dev/null || exit
    else
        echo "⚠️  Directory not found"
        ((FAILED++))
    fi
done

echo ""
echo "=============================================="
echo "📊 Quick Test Summary"
echo "=============================================="
echo "Total:  $TOTAL"
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo ""
echo "For complete testing of all 343 programs:"
echo "  python3 test_all_programs.py"
