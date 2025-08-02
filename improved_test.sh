#\!/bin/bash

echo "=== TESTING KEY COMPATIBILITY ===" 

# Test 1: Verify existing keys are a proper pair
echo "Test 1: Verifying existing key pair compatibility..."
openssl rsa -in private_key.pem -pubout > temp_derived_public.pem 2>/dev/null
if diff -q public_key.pem temp_derived_public.pem > /dev/null; then
    echo "[PASS] Existing private and public keys are a matching pair"
    
    # Test 2: Cross-platform test with matching keys
    echo "Test 2: Cross-platform signing and verification..."
    ./loyalty_signer sign test_results/test_ezcard.csv test_cross_platform_fixed.csv private_key.pem > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "[PASS] Cross-platform signing successful"
        
        ./loyalty_signer verify test_cross_platform_fixed.csv public_key.pem > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            echo "[PASS] Cross-platform verification successful"
            echo "SUCCESS: All cross-platform tests passed\!"
        else
            echo "[FAIL] Cross-platform verification failed"
        fi
    else
        echo "[FAIL] Cross-platform signing failed"
    fi
else
    echo "[FAIL] Existing keys don't match - this was the original problem"
fi

rm -f temp_derived_public.pem test_cross_platform_fixed.csv
