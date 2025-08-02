#!/bin/bash

# Generate WireGuard Client Configuration
# Usage: bash generate_client.sh [client_name] [client_ip]
# Example: bash generate_client.sh phone 10.8.0.2

set -e

CLIENT_NAME=${1:-client1}
CLIENT_IP=${2:-10.8.0.2}

echo "=== Generating WireGuard Client Configuration ==="
echo "Client name: $CLIENT_NAME"
echo "Client IP: $CLIENT_IP"

# Generate client keys
echo "Generating client keys..."
wg genkey | tee ${CLIENT_NAME}_private.key | wg pubkey > ${CLIENT_NAME}_public.key

# Get server public key
if [[ ! -f /etc/wireguard/public.key ]]; then
    echo "Error: Server public key not found. Run setup_server.sh first."
    exit 1
fi

SERVER_PUBLIC_KEY=$(sudo cat /etc/wireguard/public.key)

# Get external IP
EXTERNAL_IP=$(curl -s ifconfig.me)
if [[ -z "$EXTERNAL_IP" ]]; then
    echo "Warning: Could not determine external IP. Please replace YOUR_EXTERNAL_IP in the config."
    EXTERNAL_IP="YOUR_EXTERNAL_IP"
fi

# Get client keys
CLIENT_PRIVATE_KEY=$(cat ${CLIENT_NAME}_private.key)
CLIENT_PUBLIC_KEY=$(cat ${CLIENT_NAME}_public.key)

# Create client configuration
cat > ${CLIENT_NAME}.conf << EOF
[Interface]
PrivateKey = $CLIENT_PRIVATE_KEY
Address = $CLIENT_IP/24
DNS = 10.8.0.1

[Peer]
PublicKey = $SERVER_PUBLIC_KEY
Endpoint = $EXTERNAL_IP:51820
AllowedIPs = 192.168.68.0/24, 10.8.0.0/24
PersistentKeepalive = 25
EOF

echo ""
echo "=== CLIENT CONFIGURATION GENERATED ==="
echo "Files created:"
echo "- ${CLIENT_NAME}.conf (client configuration)"
echo "- ${CLIENT_NAME}_private.key (keep secure)"
echo "- ${CLIENT_NAME}_public.key (for server)"
echo ""
echo "Client public key (add to server):"
echo "$CLIENT_PUBLIC_KEY"
echo ""
echo "External IP detected: $EXTERNAL_IP"
echo ""
echo "Next steps:"
echo "1. Add client to server using: bash add_client.sh $CLIENT_NAME $CLIENT_IP"
echo "2. Import ${CLIENT_NAME}.conf into WireGuard client app"
echo "3. Generate QR code with: qrencode -t ansiutf8 < ${CLIENT_NAME}.conf"
echo "==============================="