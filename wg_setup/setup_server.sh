#!/bin/bash

# WireGuard Server Setup Script for PoolIO Hub
# Run this script as: sudo bash setup_server.sh

set -e

echo "=== WireGuard Server Setup for PoolIO Hub ==="

# Check if running as root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root (use sudo)" 
   exit 1
fi

# Install WireGuard
echo "Installing WireGuard..."
apt update
apt install -y wireguard

# Enable IP forwarding
echo "Enabling IP forwarding..."
echo 'net.ipv4.ip_forward=1' >> /etc/sysctl.conf
sysctl -p

# Create WireGuard directory
echo "Creating WireGuard configuration directory..."
mkdir -p /etc/wireguard

# Generate server keys
echo "Generating server keys..."
wg genkey | tee /etc/wireguard/private.key
cat /etc/wireguard/private.key | wg pubkey | tee /etc/wireguard/public.key

# Set proper permissions
chmod 600 /etc/wireguard/private.key
chmod 644 /etc/wireguard/public.key

# Create server configuration
echo "Creating server configuration..."
SERVER_PRIVATE_KEY=$(cat /etc/wireguard/private.key)

cat > /etc/wireguard/wg0.conf << EOF
[Interface]
PrivateKey = $SERVER_PRIVATE_KEY
Address = 10.8.0.1/24
ListenPort = 51820
PostUp = iptables -A FORWARD -i wg0 -j ACCEPT; iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
PostDown = iptables -D FORWARD -i wg0 -j ACCEPT; iptables -t nat -D POSTROUTING -o eth0 -j MASQUERADE

# Client configurations will be added here
EOF

chmod 600 /etc/wireguard/wg0.conf

# Enable and start WireGuard
echo "Enabling and starting WireGuard..."
systemctl enable wg-quick@wg0
systemctl start wg-quick@wg0

# Display server information
echo ""
echo "=== SERVER SETUP COMPLETE ==="
echo "Server private key:"
cat /etc/wireguard/private.key
echo ""
echo "Server public key:"
cat /etc/wireguard/public.key
echo ""
echo "Server IP: 10.8.0.1"
echo "Listen Port: 51820"
echo ""
echo "WireGuard status:"
wg show
echo ""
echo "Next steps:"
echo "1. Configure port forwarding on your router (UDP 51820 -> 192.168.68.120)"
echo "2. Run generate_client.sh to create client configurations"
echo "3. Use add_client.sh to add clients to the server"
echo "=========================="