#!/bin/bash

# Generate QR Code for WireGuard Client Configuration
# Usage: bash generate_qr.sh [client_name]
# Example: bash generate_qr.sh phone

set -e

CLIENT_NAME=${1}

if [[ -z "$CLIENT_NAME" ]]; then
    echo "Usage: bash generate_qr.sh [client_name]"
    echo "Example: bash generate_qr.sh phone"
    exit 1
fi

echo "=== Generating QR Code for WireGuard Client ==="
echo "Client name: $CLIENT_NAME"

# Check if client config exists
if [[ ! -f ${CLIENT_NAME}.conf ]]; then
    echo "Error: Client configuration file ${CLIENT_NAME}.conf not found."
    echo "Run generate_client.sh first to create client configuration."
    exit 1
fi

# Check if qrencode is installed
if ! command -v qrencode &> /dev/null; then
    echo "Installing qrencode..."
    sudo apt update
    sudo apt install -y qrencode
fi

# Generate QR code for terminal display
echo ""
echo "=== QR CODE (scan with WireGuard mobile app) ==="
qrencode -t ansiutf8 < ${CLIENT_NAME}.conf
echo "============================================="

# Generate QR code image file
qrencode -o ${CLIENT_NAME}-qr.png < ${CLIENT_NAME}.conf
echo ""
echo "QR code image saved as: ${CLIENT_NAME}-qr.png"
echo ""
echo "To use:"
echo "1. Open WireGuard app on mobile device"
echo "2. Tap '+' -> 'Create from QR code'"
echo "3. Scan the QR code above"
echo "4. Name the tunnel 'PoolIO VPN'"
echo "5. Save and connect"