#!/bin/bash

# Add WireGuard Client to Server Configuration
# Usage: bash add_client.sh [client_name] [client_ip]
# Example: bash add_client.sh phone 10.8.0.2

set -e

CLIENT_NAME=${1:-client1}
CLIENT_IP=${2:-10.8.0.2}

echo "=== Adding Client to WireGuard Server ==="
echo "Client name: $CLIENT_NAME"
echo "Client IP: $CLIENT_IP"

# Check if running as root for server config access
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root (use sudo) to modify server configuration" 
   exit 1
fi

# Check if client public key exists
if [[ ! -f ${CLIENT_NAME}_public.key ]]; then
    echo "Error: Client public key file ${CLIENT_NAME}_public.key not found."
    echo "Run generate_client.sh first to create client keys."
    exit 1
fi

CLIENT_PUBLIC_KEY=$(cat ${CLIENT_NAME}_public.key)

# Check if client already exists in server config
if grep -q "$CLIENT_PUBLIC_KEY" /etc/wireguard/wg0.conf; then
    echo "Warning: Client with this public key already exists in server configuration."
    exit 1
fi

# Add client peer to server configuration
echo "Adding client peer to server configuration..."
cat >> /etc/wireguard/wg0.conf << EOF

[Peer]
# Client: $CLIENT_NAME
PublicKey = $CLIENT_PUBLIC_KEY
AllowedIPs = $CLIENT_IP/32
EOF

# Restart WireGuard to apply changes
echo "Restarting WireGuard server..."
systemctl restart wg-quick@wg0

# Verify client was added
echo ""
echo "=== CLIENT ADDED SUCCESSFULLY ==="
echo "Client: $CLIENT_NAME"
echo "IP: $CLIENT_IP"
echo "Public Key: $CLIENT_PUBLIC_KEY"
echo ""
echo "Updated server configuration:"
echo "$(tail -n 10 /etc/wireguard/wg0.conf)"
echo ""
echo "Current WireGuard status:"
wg show
echo ""
echo "Client can now connect using ${CLIENT_NAME}.conf"
echo "========================="