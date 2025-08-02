#!/bin/bash

# Remove WireGuard Client from Server Configuration
# Usage: bash remove_client.sh [client_name]
# Example: bash remove_client.sh phone

set -e

CLIENT_NAME=${1}

if [[ -z "$CLIENT_NAME" ]]; then
    echo "Usage: bash remove_client.sh [client_name]"
    echo "Example: bash remove_client.sh phone"
    exit 1
fi

echo "=== Removing Client from WireGuard Server ==="
echo "Client name: $CLIENT_NAME"

# Check if running as root for server config access
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root (use sudo) to modify server configuration" 
   exit 1
fi

# Check if client public key exists
if [[ ! -f ${CLIENT_NAME}_public.key ]]; then
    echo "Error: Client public key file ${CLIENT_NAME}_public.key not found."
    exit 1
fi

CLIENT_PUBLIC_KEY=$(cat ${CLIENT_NAME}_public.key)

# Check if client exists in server config
if ! grep -q "$CLIENT_PUBLIC_KEY" /etc/wireguard/wg0.conf; then
    echo "Warning: Client with this public key not found in server configuration."
    exit 1
fi

# Create backup of current config
cp /etc/wireguard/wg0.conf /etc/wireguard/wg0.conf.backup.$(date +%Y%m%d_%H%M%S)

# Remove client peer from server configuration
echo "Removing client peer from server configuration..."

# Create temporary file without the client
awk -v pubkey="$CLIENT_PUBLIC_KEY" '
BEGIN { skip = 0 }
/^\[Peer\]/ { 
    if (skip) skip = 0
    peer_section = ""
    in_peer = 1
    next
}
in_peer && /^PublicKey/ { 
    if ($3 == pubkey) {
        skip = 1
        in_peer = 0
        next
    } else {
        peer_section = peer_section "[Peer]\n" $0 "\n"
        next
    }
}
in_peer && skip == 0 {
    peer_section = peer_section $0 "\n"
    next
}
in_peer && /^$/ {
    if (!skip) print peer_section
    peer_section = ""
    in_peer = 0
    skip = 0
}
!in_peer && !skip { print }
END {
    if (in_peer && !skip) print peer_section
}
' /etc/wireguard/wg0.conf > /etc/wireguard/wg0.conf.tmp

mv /etc/wireguard/wg0.conf.tmp /etc/wireguard/wg0.conf
chmod 600 /etc/wireguard/wg0.conf

# Restart WireGuard to apply changes
echo "Restarting WireGuard server..."
systemctl restart wg-quick@wg0

# Clean up client files
echo "Removing client files..."
rm -f ${CLIENT_NAME}.conf
rm -f ${CLIENT_NAME}_private.key
rm -f ${CLIENT_NAME}_public.key
rm -f ${CLIENT_NAME}-qr.png

echo ""
echo "=== CLIENT REMOVED SUCCESSFULLY ==="
echo "Client: $CLIENT_NAME"
echo "Public Key: $CLIENT_PUBLIC_KEY"
echo ""
echo "Current WireGuard status:"
wg show
echo ""
echo "Client files have been deleted."
echo "Backup configuration saved with timestamp."
echo "========================="