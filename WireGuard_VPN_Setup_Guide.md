# WireGuard VPN Setup Guide for PoolIO Hub

## Overview

This guide sets up WireGuard VPN on your poolio-hub server to provide secure remote access to your pool monitoring system from anywhere in the world.

**What you'll get:**
- Secure encrypted access to poolio-hub from mobile devices
- Access to pool dashboard at `http://10.8.0.1`
- Access to InfluxDB admin at `http://10.8.0.1:8086`
- All traffic encrypted and routed through your home network

**Prerequisites:**
- poolio-hub server running Ubuntu with pool monitoring stack
- TP-Link Deco router access for port forwarding
- Mobile device or computer for VPN client

---

## Step 1: Install WireGuard on poolio-hub

```bash
# Install WireGuard
sudo apt update
sudo apt install wireguard

# Enable IP forwarding (required for VPN)
echo 'net.ipv4.ip_forward=1' | sudo tee -a /etc/sysctl.conf
sudo sysctl -p

# Verify IP forwarding is enabled
cat /proc/sys/net/ipv4/ip_forward
# Should output: 1
```

---

## Step 2: Generate Server Keys

```bash
# Create WireGuard directory
sudo mkdir -p /etc/wireguard

# Generate server private and public keys
wg genkey | sudo tee /etc/wireguard/private.key
sudo cat /etc/wireguard/private.key | wg pubkey | sudo tee /etc/wireguard/public.key

# Set proper permissions
sudo chmod 600 /etc/wireguard/private.key
sudo chmod 644 /etc/wireguard/public.key

# Display keys for configuration
echo "=== SERVER KEYS ==="
echo "Server private key:"
sudo cat /etc/wireguard/private.key
echo ""
echo "Server public key:"
sudo cat /etc/wireguard/public.key
echo "==================="
```

**⚠️ Important:** Save these keys securely - you'll need them for configuration.

---

## Step 3: Configure WireGuard Server

```bash
# Create server configuration
sudo tee /etc/wireguard/wg0.conf > /dev/null << 'EOF'
[Interface]
PrivateKey = SERVER_PRIVATE_KEY_HERE
Address = 10.8.0.1/24
ListenPort = 51820
PostUp = iptables -A FORWARD -i wg0 -j ACCEPT; iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
PostDown = iptables -D FORWARD -i wg0 -j ACCEPT; iptables -t nat -D POSTROUTING -o eth0 -j MASQUERADE

# Client configurations will be added here
EOF

# Replace SERVER_PRIVATE_KEY_HERE with actual key
SERVER_PRIVATE_KEY=$(sudo cat /etc/wireguard/private.key)
sudo sed -i "s/SERVER_PRIVATE_KEY_HERE/$SERVER_PRIVATE_KEY/g" /etc/wireguard/wg0.conf

# Set permissions
sudo chmod 600 /etc/wireguard/wg0.conf

# Verify configuration
echo "=== SERVER CONFIG ==="
sudo cat /etc/wireguard/wg0.conf
echo "====================="
```

---

## Step 4: Configure Router Port Forwarding

**Using TP-Link Deco App:**
1. Open **Deco app** on your phone
2. Go to **More** → **Advanced** → **Port Forwarding**
3. Tap **Add** or **+**
4. Configure:
   - **Service Name**: `WireGuard VPN`
   - **Protocol**: `UDP`
   - **External Port**: `51820`
   - **Device**: Select `poolio-hub` (YOUR_HUB_IP)
   - **Internal Port**: `51820`
5. Tap **Save**

**Using Web Interface:**
1. Visit `http://192.168.68.1`
2. Login with admin credentials
3. Go to **Advanced** → **NAT Forwarding** → **Port Forwarding**
4. Add new rule:
   - **Service Name**: `WireGuard VPN`
   - **Protocol**: `UDP`
   - **External Port**: `51820`
   - **Internal IP**: `YOUR_HUB_IP` (poolio-hub)
   - **Internal Port**: `51820`
5. **Save** and **Apply**

**Verify Port Forwarding:**
```bash
# Check if external port is reachable (from outside your network)
# This command should be run from a different network or use online port checker
```

---

## Step 5: Start WireGuard Server

```bash
# Enable and start WireGuard
sudo systemctl enable wg-quick@wg0
sudo systemctl start wg-quick@wg0

# Check status
sudo systemctl status wg-quick@wg0

# Verify WireGuard is running
sudo wg show

# Check if port is listening
sudo ss -tulpn | grep 51820

# Check interface is created
ip addr show wg0
```

**Expected output for `sudo wg show`:**
```
interface: wg0
  public key: [your server public key]
  private key: (hidden)
  listening port: 51820
```

---

## Step 6: Generate Client Configuration

```bash
# Change to temporary directory
cd ~

# Generate client keys
wg genkey | tee client_private.key | wg pubkey > client_public.key

# Display client keys
echo "=== CLIENT KEYS ==="
echo "Client private key:"
cat client_private.key
echo ""
echo "Client public key:"
cat client_public.key
echo "==================="

# Get your external IP address
EXTERNAL_IP=$(curl -s ifconfig.me)
echo "Your external IP: $EXTERNAL_IP"
```

---

## Step 7: Add Client to Server Configuration

```bash
# Add client peer to server config
CLIENT_PUBLIC_KEY=$(cat client_public.key)

sudo tee -a /etc/wireguard/wg0.conf > /dev/null << EOF

[Peer]
PublicKey = $CLIENT_PUBLIC_KEY
AllowedIPs = 10.8.0.2/32
EOF

# Restart WireGuard to apply changes
sudo systemctl restart wg-quick@wg0

# Verify client was added
echo "=== UPDATED SERVER CONFIG ==="
sudo cat /etc/wireguard/wg0.conf
echo "============================="
```

---

## Step 8: Create Client Configuration File

```bash
# Get required information
EXTERNAL_IP=$(curl -s ifconfig.me)
SERVER_PUBLIC_KEY=$(sudo cat /etc/wireguard/public.key)
CLIENT_PRIVATE_KEY=$(cat client_private.key)

# Create client config
cat > poolio-client.conf << EOF
[Interface]
PrivateKey = $CLIENT_PRIVATE_KEY
Address = 10.8.0.2/24
DNS = 10.8.0.1

[Peer]
PublicKey = $SERVER_PUBLIC_KEY
Endpoint = $EXTERNAL_IP:51820
AllowedIPs = 192.168.68.0/24, 10.8.0.0/24
PersistentKeepalive = 25
EOF

echo "=== CLIENT CONFIGURATION ==="
echo "Client configuration saved to: poolio-client.conf"
echo ""
cat poolio-client.conf
echo "=========================="
```

**Configuration Explanation:**
- **Address**: Client's VPN IP (10.8.0.2)
- **DNS**: Use hub as DNS server (10.8.0.1)
- **AllowedIPs**: Routes home network (192.168.68.0/24) and VPN network (10.8.0.0/24) through VPN
- **PersistentKeepalive**: Keeps connection alive through NAT

---

## Step 9: Install WireGuard Client

### Mobile Device (iOS/Android)

**Install WireGuard App:**
1. Download **WireGuard** from App Store or Google Play
2. Open the app

**Add VPN Configuration:**

**Method 1: QR Code (Easiest)**
```bash
# Install QR code generator
sudo apt install qrencode

# Generate QR code for mobile
qrencode -t ansiutf8 < poolio-client.conf

# Or generate QR code image
qrencode -o poolio-client-qr.png < poolio-client.conf
echo "QR code saved as poolio-client-qr.png"
```

1. In WireGuard app, tap **+** → **Create from QR code**
2. Scan the QR code displayed in terminal
3. Name the tunnel: `PoolIO VPN`
4. Save the configuration

**Method 2: Manual Import**
1. Copy the `poolio-client.conf` content to your device
2. In WireGuard app, tap **+** → **Create from file or archive**
3. Select the file or paste the configuration
4. Save as `PoolIO VPN`

### Computer (Windows/Mac/Linux)

**Install WireGuard Client:**
- **Windows**: Download from [wireguard.com](https://www.wireguard.com/install/)
- **Mac**: Install from App Store or brew: `brew install wireguard-tools`
- **Linux**: `sudo apt install wireguard`

**Import Configuration:**
1. Open WireGuard client
2. Click **Add Tunnel** → **Add from file**
3. Select `poolio-client.conf`
4. Name it `PoolIO VPN`

---

## Step 10: Test VPN Connection

### On Server (poolio-hub)

```bash
# Check VPN status
sudo wg show

# Monitor connections in real-time
sudo watch wg show

# Check connected clients
sudo wg show wg0 peers

# Check VPN interface
ip addr show wg0

# Monitor VPN traffic
sudo tcpdump -i wg0
```

### On Client Device

**Connect to VPN:**
1. Open WireGuard app/client
2. Toggle **PoolIO VPN** to ON
3. Verify connection shows as active

**Test Access:**
```bash
# Test VPN IP connectivity
ping 10.8.0.1

# Test pool dashboard access
curl http://10.8.0.1

# Test InfluxDB access
curl http://10.8.0.1:8086/health
```

**Web Browser Tests:**
- Pool Dashboard: `http://10.8.0.1`
- InfluxDB Admin: `http://10.8.0.1:8086`
- API Health: `http://10.8.0.1:3000/health`

### Expected Results

**Successful Connection:**
- VPN shows "Connected" status
- Can ping 10.8.0.1
- Can access pool dashboard via browser
- Server shows peer connection in `sudo wg show`

**Connection Details:**
```bash
# Client should show:
interface: wg0
  public key: [client public key]
  private key: (hidden)
  listening port: [random port]

peer: [server public key]
  endpoint: [your external IP]:51820
  allowed ips: 192.168.68.0/24, 10.8.0.0/24
  latest handshake: [recent timestamp]
  transfer: [data sent/received]
```

---

## Adding Additional Clients

To add more devices (family members, additional computers):

```bash
# Generate new client keys
wg genkey | tee client2_private.key | wg pubkey > client2_public.key

# Add to server config (increment IP address)
CLIENT2_PUBLIC_KEY=$(cat client2_public.key)
sudo tee -a /etc/wireguard/wg0.conf > /dev/null << EOF

[Peer]
PublicKey = $CLIENT2_PUBLIC_KEY
AllowedIPs = 10.8.0.3/32
EOF

# Create client2 config (increment IP address)
cat > poolio-client2.conf << EOF
[Interface]
PrivateKey = $(cat client2_private.key)
Address = 10.8.0.3/24
DNS = 10.8.0.1

[Peer]
PublicKey = $(sudo cat /etc/wireguard/public.key)
Endpoint = $(curl -s ifconfig.me):51820
AllowedIPs = 192.168.68.0/24, 10.8.0.0/24
PersistentKeepalive = 25
EOF

# Restart WireGuard
sudo systemctl restart wg-quick@wg0
```

**IP Address Allocation:**
- Server: 10.8.0.1
- Client 1: 10.8.0.2  
- Client 2: 10.8.0.3
- Client 3: 10.8.0.4
- ... up to 10.8.0.254

---

## Troubleshooting

### VPN Won't Connect

**Check Server Status:**
```bash
# Verify WireGuard is running
sudo systemctl status wg-quick@wg0

# Check if port is open
sudo ss -tulpn | grep 51820

# Check firewall (if UFW is enabled)
sudo ufw status
sudo ufw allow 51820/udp

# Check logs
sudo journalctl -u wg-quick@wg0 -f
```

**Check Router Port Forwarding:**
```bash
# Test from external network or use online port checker
# Port 51820 UDP should be open and forwarding to YOUR_HUB_IP
```

**Verify External IP:**
```bash
# Make sure external IP in client config is correct
curl ifconfig.me
```

### Can Connect but Can't Access Services

**Check Routing:**
```bash
# On client, verify routes
ip route show

# Should see routes for 192.168.68.0/24 via VPN
```

**Check DNS:**
```bash
# On client, test DNS resolution
nslookup poolio-hub.local 10.8.0.1
```

**Test Direct IP Access:**
```bash
# Bypass DNS, test direct IP (poolio-hub)
curl http://YOUR_HUB_IP
curl http://YOUR_HUB_IP:8086/health
```

### Performance Issues

**Optimize MTU:**
```bash
# Add to client [Interface] section
MTU = 1420
```

**Check Bandwidth:**
```bash
# Monitor VPN traffic
sudo iftop -i wg0
```

---

## Security Notes

**Key Management:**
- Server private key: Keep secure on poolio-hub only
- Client private keys: Each device should have unique key
- Revoke access by removing [Peer] section and restarting WireGuard

**Firewall Configuration:**
```bash
# Optional: Restrict VPN access to specific services only
sudo iptables -A FORWARD -i wg0 -p tcp --dport 80 -j ACCEPT
sudo iptables -A FORWARD -i wg0 -p tcp --dport 8086 -j ACCEPT
sudo iptables -A FORWARD -i wg0 -j DROP
```

**Regular Maintenance:**
- Monitor connected clients: `sudo wg show`
- Review logs: `sudo journalctl -u wg-quick@wg0`
- Update WireGuard: `sudo apt update && sudo apt upgrade wireguard`

---

## Summary

You now have a secure WireGuard VPN providing encrypted remote access to your pool monitoring system. This allows you to:

- Monitor pool status from anywhere in the world
- Access the web dashboard securely
- Manage InfluxDB database remotely
- Maintain privacy with encrypted connections

**Quick Access URLs (when VPN connected):**
- Pool Dashboard: `http://10.8.0.1`
- InfluxDB Admin: `http://10.8.0.1:8086`
- API Health: `http://10.8.0.1:3000/health`