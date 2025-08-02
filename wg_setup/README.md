# WireGuard VPN Setup Files

This folder contains all the configuration files and scripts needed to set up WireGuard VPN for your PoolIO Hub.

## Files Overview

### Configuration Templates
- `wg0.conf.template` - Server configuration template
- `client.conf.template` - Client configuration template

### Setup Scripts
- `setup_server.sh` - Complete server setup (run once)
- `generate_client.sh` - Generate client configurations
- `add_client.sh` - Add clients to server
- `remove_client.sh` - Remove clients from server
- `generate_qr.sh` - Generate QR codes for mobile clients
- `test_vpn.sh` - Test VPN connectivity

## Quick Start

### 1. Initial Server Setup
```bash
# Run the complete server setup
sudo bash setup_server.sh
```

### 2. Configure Router Port Forwarding
Forward UDP port 51820 to YOUR_HUB_IP (poolio-hub)

### 3. Generate First Client
```bash
# Generate client configuration for your phone
bash generate_client.sh phone 10.8.0.2

# Add client to server
sudo bash add_client.sh phone 10.8.0.2

# Generate QR code for mobile app
bash generate_qr.sh phone
```

### 4. Test Connection
```bash
# Test server configuration
bash test_vpn.sh
```

## Client IP Allocation

- Server: 10.8.0.1
- Client 1: 10.8.0.2 (usually mobile phone)
- Client 2: 10.8.0.3 (laptop/computer)
- Client 3: 10.8.0.4 (additional devices)
- ... up to 10.8.0.254

## Common Commands

### Add New Client
```bash
bash generate_client.sh laptop 10.8.0.3
sudo bash add_client.sh laptop 10.8.0.3
bash generate_qr.sh laptop
```

### Remove Client
```bash
sudo bash remove_client.sh laptop
```

### Check VPN Status
```bash
sudo wg show
sudo systemctl status wg-quick@wg0
```

### Monitor Connections
```bash
sudo watch wg show
```

## Mobile App Setup

1. Install WireGuard app from App Store/Google Play
2. Run `bash generate_qr.sh [client_name]`
3. In app: tap "+" → "Create from QR code"
4. Scan the QR code
5. Name tunnel "PoolIO VPN"
6. Connect to access pool dashboard

## Access URLs (when VPN connected)

- Pool Dashboard: http://10.8.0.1
- InfluxDB Admin: http://10.8.0.1:8086
- API Health: http://10.8.0.1:3000/health

## Troubleshooting

### VPN Won't Connect
1. Check router port forwarding (UDP 51820)
2. Verify external IP in client config
3. Run `bash test_vpn.sh`

### Can Connect but Can't Access Services
1. Check if pool services are running
2. Verify client AllowedIPs includes 192.168.68.0/24
3. Test direct IP access to YOUR_HUB_IP

### Performance Issues
Add to client config [Interface] section:
```
MTU = 1420
```

## Security Notes

- Keep server private key secure
- Each client should have unique keys
- Remove clients by deleting [Peer] section and restarting
- Monitor connections with `sudo wg show`

## File Permissions

Scripts will be made executable automatically. If needed:
```bash
chmod +x *.sh
```