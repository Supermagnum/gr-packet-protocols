#!/usr/bin/env python3
"""
Standalone test script for Tier 4 functionality.
This verifies that Tier 4 modes work correctly.
"""

import sys
import os

# Setup path to use build directory
build_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), 'build'))
test_modules = os.path.join(build_dir, 'test_modules')
if os.path.exists(test_modules):
    sys.path.insert(0, test_modules)

from gnuradio.packet_protocols import adaptive_rate_control, modulation_mode_t

def test_tier4_disabled_by_default():
    """Test that Tier 4 is disabled by default"""
    print("Test 1: Tier 4 disabled by default")
    rate_control = adaptive_rate_control(
        initial_mode=modulation_mode_t.MODE_2FSK
    )
    
    initial_mode = rate_control.get_modulation_mode()
    print(f"  Initial mode: {initial_mode}")
    
    # Try to set a Tier 4 mode - should be rejected
    rate_control.set_modulation_mode(modulation_mode_t.MODE_SOQPSK_1M)
    final_mode = rate_control.get_modulation_mode()
    print(f"  Mode after attempting Tier 4: {final_mode}")
    
    assert final_mode == initial_mode, "Tier 4 mode should be rejected when disabled"
    print("  PASS: Tier 4 mode correctly rejected\n")

def test_tier4_can_be_enabled():
    """Test that Tier 4 can be enabled"""
    print("Test 2: Tier 4 can be enabled")
    rate_control = adaptive_rate_control(
        initial_mode=modulation_mode_t.MODE_2FSK,
        enable_tier4=True
    )
    
    # Should be able to set Tier 4 mode when enabled
    rate_control.set_modulation_mode(modulation_mode_t.MODE_SOQPSK_1M)
    mode = rate_control.get_modulation_mode()
    print(f"  Mode after enabling Tier 4: {mode}")
    
    assert mode == modulation_mode_t.MODE_SOQPSK_1M, "Tier 4 mode should be accepted when enabled"
    print("  PASS: Tier 4 mode correctly accepted\n")

def test_tier4_data_rates():
    """Test that Tier 4 modes have correct data rates"""
    print("Test 3: Tier 4 data rates")
    rate_control = adaptive_rate_control(
        initial_mode=modulation_mode_t.MODE_2FSK,
        enable_tier4=True
    )
    
    test_cases = [
        (modulation_mode_t.MODE_SOQPSK_1M, 1000000),
        (modulation_mode_t.MODE_SOQPSK_5M, 5000000),
        (modulation_mode_t.MODE_SOQPSK_10M, 10000000),
        (modulation_mode_t.MODE_SOQPSK_20M, 20000000),
        (modulation_mode_t.MODE_SOQPSK_40M, 40000000),
    ]
    
    for mode, expected_rate in test_cases:
        rate_control.set_modulation_mode(mode)
        rate = rate_control.get_data_rate()
        print(f"  {mode}: {rate} bps (expected {expected_rate})")
        assert rate == expected_rate, f"Expected {expected_rate} bps, got {rate}"
    
    print("  PASS: All Tier 4 data rates correct\n")

def test_set_tier4_enabled():
    """Test set_tier4_enabled() method"""
    print("Test 4: set_tier4_enabled() method")
    rate_control = adaptive_rate_control(
        initial_mode=modulation_mode_t.MODE_2FSK,
        enable_tier4=False
    )
    
    # Initially disabled - try to set Tier 4 mode
    rate_control.set_modulation_mode(modulation_mode_t.MODE_SOQPSK_1M)
    mode = rate_control.get_modulation_mode()
    print(f"  Mode when disabled: {mode}")
    assert mode != modulation_mode_t.MODE_SOQPSK_1M, "Tier 4 should be rejected"
    
    # Enable Tier 4
    rate_control.set_tier4_enabled(True)
    rate_control.set_modulation_mode(modulation_mode_t.MODE_SOQPSK_1M)
    mode = rate_control.get_modulation_mode()
    print(f"  Mode after enabling: {mode}")
    assert mode == modulation_mode_t.MODE_SOQPSK_1M, "Tier 4 should be accepted"
    
    # Disable Tier 4 while in Tier 4 mode - should fall back
    rate_control.set_tier4_enabled(False)
    mode = rate_control.get_modulation_mode()
    print(f"  Mode after disabling: {mode}")
    assert mode not in [
        modulation_mode_t.MODE_SOQPSK_1M,
        modulation_mode_t.MODE_SOQPSK_5M,
        modulation_mode_t.MODE_SOQPSK_10M,
        modulation_mode_t.MODE_SOQPSK_20M,
        modulation_mode_t.MODE_SOQPSK_40M
    ], "Should have fallen back to non-Tier 4 mode"
    
    print("  PASS: set_tier4_enabled() works correctly\n")

def test_tier4_initial_mode_rejected():
    """Test that Tier 4 initial mode is rejected when disabled"""
    print("Test 5: Tier 4 initial mode rejected when disabled")
    rate_control = adaptive_rate_control(
        initial_mode=modulation_mode_t.MODE_SOQPSK_1M,
        enable_tier4=False
    )
    
    mode = rate_control.get_modulation_mode()
    print(f"  Initial mode (should fallback to 2FSK): {mode}")
    assert mode == modulation_mode_t.MODE_2FSK, "Should fall back to 2FSK"
    print("  PASS: Tier 4 initial mode correctly rejected\n")

if __name__ == '__main__':
    print("=" * 60)
    print("Tier 4 Functionality Tests")
    print("=" * 60)
    print()
    
    try:
        test_tier4_disabled_by_default()
        test_tier4_can_be_enabled()
        test_tier4_data_rates()
        test_set_tier4_enabled()
        test_tier4_initial_mode_rejected()
        
        print("=" * 60)
        print("ALL TESTS PASSED")
        print("=" * 60)
        sys.exit(0)
    except AssertionError as e:
        print(f"\nFAILED: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"\nERROR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

