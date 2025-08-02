# PoolIO Architecture Enhancement - Focused Implementation

## Executive Summary

Modernize your existing single-pool PoolIO system by adding a local hub (Ubuntu server), mobile web interface, and extensible plugin architecture for new sensors/controls while preserving your proven three-node design.

**Key Goals:**
- Add mobile web interface for remote monitoring/control
- Create extensible system for new sensors and control devices
- Improve reliability with local data storage and processing
- Maintain existing hardware where practical

---

## Current Hardware Assessment

### What You Have
- **Ubuntu Server**: Available for hub functionality
- **ESP32-S3 PoolNode**: Temperature, float switch, battery monitoring
- **ESP32-S3 ValveNode**: Outside temperature, valve control
- **DisplayNode**: Existing display hardware (keep for compatibility)

### What Needs Enhancement
- No mobile access capability
- Limited extensibility for new sensors/controls
- Cloud-dependent operation
- No local data persistence

---

## Recommended Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   PoolNode      │    │   Ubuntu Hub     │    │   ValveNode     │
│   (ESP32-S3)    │◄──►│   - MQTT Broker  │◄──►│   (ESP32-S3)    │
│   - Temp        │    │   - Web API      │    │   - Outside Temp│
│   - Float       │    │   - Data Storage │    │   - Valve Ctrl  │
│   - Battery     │    │   - Plugin Mgmt  │    │                 │
└─────────────────┘    └──────┬───────────┘    └─────────────────┘
                              │
                    ┌─────────┼─────────┐
                    │         │         │
           ┌────────┴──────┐  │  ┌──────┴──────────┐
           │ DisplayNode   │  │  │  Mobile Devices │
           │ (Current HW)  │  │  │  - Web Browser  │    
           │ - Local UI    │  │  │  - Direct HTTP  │    
           │ - Backup Ctrl │  │  │  - Real-time WS │    
           └───────────────┘  │  └─────────────────┘
                              │
                       ┌──────┴──────┐
                       │ AWS S3      │
                       │ - Backups   │
                       │ - Archives  │
                       └─────────────┘
```

---

## Implementation Plan

### Phase 1: Ubuntu Hub Setup (Week 1-2)

**Core Services (Docker Compose):**
```yaml
services:
  mosquitto:
    image: eclipse-mosquitto:2.0
    ports: ["1883:1883", "9001:9001"]
    
  influxdb:
    image: influxdb:2.7
    ports: ["8086:8086"]
    
  poolio-api:
    build: ./api
    ports: ["3000:3000"]
    depends_on: [mosquitto, influxdb]
    
  poolio-web:
    build: ./web
    ports: ["80:80"]
    depends_on: [poolio-api]
```

**Key Features:**
- Local MQTT broker for device communication
- Time-series database for sensor history
- REST API for mobile interface
- Web dashboard accessible from any device
- Plugin system for extensibility

### Phase 2: ESP32 Node Enhancement (Week 3-4)

**Keep CircuitPython** for existing functionality, add:
- MQTT connection to local hub (fallback to cloud)
- Plugin registration protocol
- OTA update capability
- Enhanced error handling

**Or Migrate to ESP-IDF/Arduino** for better performance:
- Native C++ for faster processing
- Better memory management
- More robust networking stack
- Easier integration with new sensor libraries

**Plugin Interface Example:**
```cpp
class PoolSensor {
public:
    virtual String getId() = 0;
    virtual String getType() = 0;
    virtual JsonObject readData() = 0;
    virtual bool initialize() = 0;
};

class PHSensor : public PoolSensor {
    String getId() { return "ph_sensor_01"; }
    String getType() { return "ph"; }
    JsonObject readData() {
        // Read pH sensor and return structured data
    }
};
```

### Phase 3: Mobile Web Interface (Week 3-5)

**Technology Stack:**
- **Frontend**: React/Vue.js with responsive design
- **Real-time**: WebSocket connection for live updates
- **Offline**: Service worker for cached operation
- **PWA**: Install as app on mobile devices

**Key Features:**
- Live sensor readings with historical charts
- Manual valve control with safety interlocks
- Alert notifications (browser push)
- System status and diagnostics
- Settings configuration

**Mobile Interface Layout:**
```
┌─────────────────────────────────┐
│ Pool Status          🔄 Settings│
├─────────────────────────────────┤
│ Temperature: 78.5°F   ⬆ Trending│
│ Water Level: OK       🟢 Normal  │
│ Battery: 3.8V         🔋 Good    │
├─────────────────────────────────┤
│ Valve Control                   │
│ [ Manual Fill ] [ Stop Fill ]   │
│ Next Auto: 2:00 PM              │
├─────────────────────────────────┤
│ 📊 Temperature Chart (24h)      │
│ [Chart showing temp over time]  │
└─────────────────────────────────┘
```

**Connection Architecture:**
- **Local Network**: Mobile devices connect directly to Ubuntu Hub via HTTP/WebSocket
- **Remote Access**: Hybrid VPN + Cloudflare tunnel approach (see Remote Access section)
- **DisplayNode**: Also connects directly to Ubuntu Hub (peer relationship)
- **ESP32 Nodes**: Connect to Ubuntu Hub via local MQTT
- **No daisy-chaining**: All devices communicate directly with the central hub

### Phase 4: Plugin System (Week 5-6)

**Sensor Plugin Architecture:**
```typescript
interface SensorPlugin {
  id: string;
  name: string;
  type: 'temperature' | 'ph' | 'chlorine' | 'flow' | 'pressure';
  units: string;
  initialize(): Promise<boolean>;
  read(): Promise<SensorReading>;
  calibrate?(params: CalibrationParams): Promise<void>;
}

interface ControlPlugin {
  id: string;
  name: string;
  type: 'valve' | 'pump' | 'heater' | 'doser';
  execute(command: ControlCommand): Promise<void>;
  getStatus(): Promise<DeviceStatus>;
}
```

**Example New Sensors:**
- pH/ORP sensor (analog input)
- Flow meter (pulse counter)
- Pressure sensor (I2C)
- Chemical tank level (ultrasonic)

**Example New Controls:**
- Pool pump variable speed control
- Heater on/off control
- Chemical doser pumps
- UV sterilizer control

---

## Remote Access Strategy

### Hybrid Approach (Recommended)

**Three-Tier Access System:**
```
1. Local Network: Direct HTTP connection (fastest)
2. Remote Primary: WireGuard VPN (secure, private)
3. Remote Backup: Cloudflare Tunnel (fallback)
```

**Connection Priority:**
1. **Home Network**: Direct connection to `http://192.168.1.100:3000`
2. **Away + VPN Available**: Connect via VPN, then use local IP
3. **Away + VPN Unavailable**: Fall back to Cloudflare tunnel

### Implementation Details

**WireGuard VPN Setup:**
```bash
# Install on Ubuntu Hub
sudo apt install wireguard

# Generate keys
wg genkey | sudo tee /etc/wireguard/private.key
sudo cat /etc/wireguard/private.key | wg pubkey | sudo tee /etc/wireguard/public.key

# Configure server (/etc/wireguard/wg0.conf)
[Interface]
PrivateKey = <server-private-key>
Address = 10.8.0.1/24
ListenPort = 51820
PostUp = iptables -A FORWARD -i wg0 -j ACCEPT
PostDown = iptables -D FORWARD -i wg0 -j ACCEPT

[Peer]
PublicKey = <mobile-device-public-key>
AllowedIPs = 10.8.0.2/32
```

**Cloudflare Tunnel Backup:**
```bash
# Install cloudflared on Ubuntu Hub
curl -L https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64.deb -o cloudflared.deb
sudo dpkg -i cloudflared.deb

# Create tunnel
cloudflared tunnel create poolio-backup
cloudflared tunnel route dns poolio-backup poolio-backup.yourdomain.com

# Configure tunnel (/etc/cloudflared/config.yml)
tunnel: poolio-backup
credentials-file: /etc/cloudflared/credentials.json
ingress:
  - hostname: poolio-backup.yourdomain.com
    service: http://localhost:3000
  - service: http_status:404
```

**Smart Client Logic:**
```typescript
class PoolIOConnection {
  private endpoints = {
    local: 'http://192.168.1.100:3000',
    vpn: 'http://10.8.0.1:3000',
    tunnel: 'https://poolio-backup.yourdomain.com'
  };

  async connect(): Promise<string> {
    // Try local network first
    if (await this.testConnection(this.endpoints.local)) {
      return this.endpoints.local;
    }

    // Try VPN connection
    if (await this.testConnection(this.endpoints.vpn)) {
      return this.endpoints.vpn;
    }

    // Fall back to tunnel
    if (await this.testConnection(this.endpoints.tunnel)) {
      return this.endpoints.tunnel;
    }

    throw new Error('No connection available');
  }

  private async testConnection(url: string): Promise<boolean> {
    try {
      const response = await fetch(`${url}/health`, { 
        timeout: 3000 
      });
      return response.ok;
    } catch {
      return false;
    }
  }
}
```

### Security Configuration

**VPN Security:**
- WireGuard modern cryptography (ChaCha20, Poly1305)
- Automatic key rotation (optional)
- Kill switch on mobile devices
- Split tunneling (only pool traffic through VPN)

**Tunnel Security:**
- Cloudflare's automatic HTTPS/TLS
- Optional HTTP authentication
- Rate limiting at edge
- DDoS protection included

**Hub Security:**
```typescript
// Authentication middleware
app.use('/api', (req, res, next) => {
  const token = req.headers.authorization?.split(' ')[1];
  if (!verifyJWT(token)) {
    return res.status(401).json({ error: 'Unauthorized' });
  }
  next();
});

// Rate limiting
app.use(rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 100 // limit each IP to 100 requests per windowMs
}));
```

### User Experience

**Mobile App Connection Status:**
```
🟢 Connected (Local Network)    - Best performance
🟡 Connected (VPN)             - Secure remote access  
🟠 Connected (Backup Tunnel)   - Limited functionality
🔴 Offline                     - Cached data only
```

**Automatic Failover:**
- Seamless switching between connection types
- Background connectivity monitoring
- Cached data available during outages
- Offline mode for viewing historical data

### Setup Complexity

**One-Time Setup:**
1. Configure WireGuard on Ubuntu Hub (30 minutes)
2. Set up Cloudflare tunnel (15 minutes)
3. Install VPN profile on mobile devices (5 minutes each)
4. Configure mobile app with all endpoints (included in app)

**Ongoing Maintenance:**
- VPN: Minimal (automatic)
- Tunnel: Automatic updates via Cloudflare
- Monitor connection health via hub dashboard

---

## Technical Specifications

### Ubuntu Hub Requirements
- **OS**: Ubuntu Server 20.04+ (your current setup)
- **Resources**: 2GB RAM, 20GB storage minimum
- **Network**: Ethernet preferred, WiFi acceptable
- **Ports**: 80 (web), 3000 (API), 1883 (MQTT), 8086 (InfluxDB)

### ESP32 Node Enhancements
- **Communication**: Local MQTT only (no cloud dependency)
- **Data Format**: JSON for structured sensor readings
- **OTA Updates**: Secure firmware updates via hub
- **Power Management**: Optimized sleep modes for battery nodes

### Web Interface
- **Responsive Design**: Works on phones, tablets, desktops
- **Real-time Updates**: WebSocket for live data
- **Offline Capability**: Cached data when network unavailable
- **Security**: HTTPS, authentication, CSRF protection

---

## Data Flow Architecture

### Sensor Data Flow
```
ESP32 Sensor → Local MQTT → Hub Processing → InfluxDB Storage
                    ↓                           ↓
            WebSocket (real-time)        AWS S3 Backup
                    ↓                    (daily export)
            Mobile Interface
```

### Control Command Flow
```
Mobile Interface → REST API → Hub Logic → MQTT Command → ESP32 Control
                                    ↓
                            Feedback Loop ← Status Confirmation
```

### Plugin Registration
```
ESP32 Boot → Plugin Discovery → Hub Registration → Web Interface Update
```

### AWS Backup Strategy
```
InfluxDB Data → Daily Export → Compressed Archive → AWS S3
Config Files → Git Repository → AWS CodeCommit  
System Logs → Rotated Logs → AWS S3 (weekly)
```

**Backup Components:**
- **Database Export**: Daily InfluxDB backup (compressed CSV/JSON)
- **Configuration**: Git repository with system configs, device settings
- **Logs**: System and application logs for troubleshooting
- **Recovery**: Automated restore scripts for disaster recovery

**AWS Services Used:**
- **S3**: Object storage for data backups (cost: ~$1-5/month)
- **CodeCommit** (optional): Git repository for configurations
- **IAM**: Service account with minimal required permissions
- **CloudWatch** (optional): Monitoring backup success/failure

**Sample Backup Script:**
```bash
#!/bin/bash
# Daily backup script (runs via cron)
DATE=$(date +%Y%m%d)
BACKUP_DIR="/tmp/poolio-backup-$DATE"

# Export InfluxDB data
influx backup --bucket poolio --org poolio $BACKUP_DIR/

# Compress and upload to S3
tar -czf poolio-backup-$DATE.tar.gz -C /tmp poolio-backup-$DATE/
aws s3 cp poolio-backup-$DATE.tar.gz s3://your-poolio-backups/daily/

# Cleanup old local backups
rm -rf $BACKUP_DIR poolio-backup-$DATE.tar.gz
```

**Advantages over Adafruit IO:**
- **Cost**: S3 storage ~$0.50/month vs Adafruit IO Pro $10/month
- **Control**: Full ownership of data and infrastructure
- **Reliability**: No third-party service dependencies
- **Flexibility**: Standard file formats, easy data migration
- **Privacy**: Data never leaves your infrastructure (except encrypted backups)

---

## Migration Strategy

### Phase 1: Local Hub Setup (Month 1)
- Set up Ubuntu hub with Docker services
- **Eliminate Adafruit IO dependency** - direct MQTT to local broker
- Add web interface with read-only access to current data
- Set up AWS S3 backup system
- Test local MQTT communication

### Phase 2: Enhanced Nodes (Month 2) 
- Upgrade ESP32 firmware to communicate directly with hub
- Remove all Adafruit IO code from ESP32 nodes
- Add new sensor/control plugins as needed
- Full mobile control capability through local hub

### Phase 3: Optimization (Month 3)
- Fine-tune performance and reliability
- Add advanced features (scheduling, alerts)
- Optimize power consumption
- Complete system integration testing

---

## Cost & Effort Estimate

### Development Time
- **Hub Setup**: 1-2 weeks
- **Mobile Interface**: 2-3 weeks  
- **ESP32 Enhancement**: 1-2 weeks
- **Plugin System**: 1-2 weeks
- **Testing & Integration**: 1 week
- **Total**: 6-10 weeks

### Hardware Costs
- **Ubuntu Server**: $0 (existing)
- **New Sensors**: $50-200 per sensor type
- **Development Hardware**: $100-200 (breadboards, test equipment)
- **Total**: $150-400

### Key Benefits
- Mobile monitoring/control capability
- Local operation during internet outages
- Easy addition of new sensors/controls
- Historical data analysis
- Improved system reliability
- Foundation for future enhancements

---

## Recommended Next Steps

1. **Set up Ubuntu hub** with Docker services
2. **Create basic web interface** for current sensor data
3. **Enhance one ESP32 node** as proof of concept
4. **Add first new sensor/control** to validate plugin system
5. **Expand gradually** with additional capabilities

This focused approach maintains your proven architecture while adding the extensibility and mobile access you need, without unnecessary complexity.