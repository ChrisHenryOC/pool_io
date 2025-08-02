#!/bin/bash

# Test WireGuard VPN Connectivity
# Usage: bash test_vpn.sh

set -e

echo "=== WireGuard VPN Connectivity Test ==="

# Test 1: Check if WireGuard is running
echo "1. Checking WireGuard service status..."
if systemctl is-active --quiet wg-quick@wg0; then
    echo "✓ WireGuard service is running"
else
    echo "✗ WireGuard service is not running"
    echo "  Try: sudo systemctl start wg-quick@wg0"
    exit 1
fi

# Test 2: Check WireGuard interface
echo ""
echo "2. Checking WireGuard interface..."
if ip addr show wg0 &>/dev/null; then
    echo "✓ WireGuard interface wg0 exists"
    ip addr show wg0 | grep -E "inet|state"
else
    echo "✗ WireGuard interface wg0 not found"
    exit 1
fi

# Test 3: Check listening port
echo ""
echo "3. Checking if WireGuard is listening on port 51820..."
if ss -tulpn | grep -q ":51820"; then
    echo "✓ WireGuard is listening on port 51820"
    ss -tulpn | grep ":51820"
else
    echo "✗ WireGuard is not listening on port 51820"
fi

# Test 4: Check IP forwarding
echo ""
echo "4. Checking IP forwarding..."
if [[ $(cat /proc/sys/net/ipv4/ip_forward) == "1" ]]; then
    echo "✓ IP forwarding is enabled"
else
    echo "✗ IP forwarding is disabled"
    echo "  Try: echo 'net.ipv4.ip_forward=1' | sudo tee -a /etc/sysctl.conf && sudo sysctl -p"
fi

# Test 5: Show current peers
echo ""
echo "5. Current WireGuard status:"
sudo wg show

# Test 6: Check external IP
echo ""
echo "6. External IP address:"
EXTERNAL_IP=$(curl -s ifconfig.me || echo "Could not determine")
echo "External IP: $EXTERNAL_IP"

# Test 7: Check firewall
echo ""
echo "7. Checking firewall status..."
if command -v ufw &> /dev/null; then
    UFW_STATUS=$(sudo ufw status | head -1)
    echo "UFW status: $UFW_STATUS"
    if [[ "$UFW_STATUS" == *"active"* ]]; then
        echo "  Make sure to allow port 51820/udp: sudo ufw allow 51820/udp"
    fi
else
    echo "UFW not installed - using default iptables"
fi

# Test 8: Test VPN connectivity (if clients connected)
echo ""
echo "8. Testing VPN connectivity..."
if sudo wg show | grep -q "peer:"; then
    echo "Clients are connected. Testing internal connectivity..."
    
    # Ping VPN gateway
    if ping -c 1 -W 2 10.8.0.1 &>/dev/null; then
        echo "✓ VPN gateway (10.8.0.1) is reachable"
    else
        echo "? VPN gateway ping test (run from VPN client)"
    fi
    
    # Test pool services
    echo ""
    echo "Testing pool services accessibility:"
    
    # Test web interface
    if curl -s -m 5 http://192.168.68.120 &>/dev/null; then
        echo "✓ Pool web interface is accessible"
    else
        echo "? Pool web interface test (run from VPN client)"
    fi
    
    # Test API
    if curl -s -m 5 http://192.168.68.120:3000/health &>/dev/null; then
        echo "✓ Pool API is accessible"
    else
        echo "? Pool API test (run from VPN client)"
    fi
    
    # Test InfluxDB
    if curl -s -m 5 http://192.168.68.120:8086/health &>/dev/null; then
        echo "✓ InfluxDB is accessible"
    else
        echo "? InfluxDB test (run from VPN client)"
    fi
    
else
    echo "No clients currently connected"
    echo "Connect a client and run this test again"
fi

echo ""
echo "=== VPN Test Complete ==="
echo ""
echo "To test from client device:"
echo "1. Connect to VPN"
echo "2. ping 10.8.0.1"
echo "3. curl http://10.8.0.1"
echo "4. Visit http://10.8.0.1 in browser"
echo ""
echo "Troubleshooting:"
echo "- Check router port forwarding: UDP 51820 -> 192.168.68.120"
echo "- Verify external IP in client config: $EXTERNAL_IP"
echo "- Check client AllowedIPs: 192.168.68.0/24, 10.8.0.0/24"