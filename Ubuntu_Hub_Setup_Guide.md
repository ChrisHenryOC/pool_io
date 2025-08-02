# PoolIO Ubuntu Hub Setup Guide

## Prerequisites

- Ubuntu Server 20.04+ (your existing system)
- Root/sudo access
- Internet connection
- At least 2GB RAM, 20GB storage

## Step 1: System Preparation

### Update System and Set Hostname
```bash
sudo apt update && sudo apt upgrade -y
sudo apt install -y curl wget git nano htop

# Set hostname to poolio-hub
sudo hostnamectl set-hostname poolio-hub

# Verify hostname
hostname
hostname -f
```

### Configure .local Domain Resolution (avahi-daemon)
```bash
# Install avahi for .local domain broadcasting
sudo apt install avahi-daemon avahi-utils

# Enable and start the service
sudo systemctl enable avahi-daemon
sudo systemctl start avahi-daemon

# Check that it's running
sudo systemctl status avahi-daemon

# Verify it's broadcasting the hostname
avahi-browse -at

# Test local resolution
ping poolio-hub.local

# Optional: Add short hostname to /etc/hosts
echo "YOUR_HUB_IP    poolio-hub" | sudo tee -a /etc/hosts
```

### Verify Network Resolution
From any other computer on the network, test:
```bash
# These should work from other devices on your network:
ping poolio-hub.local
nslookup poolio-hub.local
ssh user@poolio-hub.local
```

### Network Configuration Complete
✅ Hostname set to `poolio-hub`
✅ mDNS/.local domain resolution configured
✅ TP-Link Deco router configured for network DNS

### Install Docker
```bash
# Install Docker (includes Compose plugin)
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker $USER

# Verify installation
docker --version
docker compose version

# IMPORTANT: Log out and back in (or restart SSH session) for docker group to take effect
echo "⚠️  You must log out and back in before proceeding to Step 8"
echo "⚠️  Run: exit (then reconnect via SSH)"
```

## Step 2: Create Project Structure

```bash
# Create project directory
mkdir -p ~/poolio-hub
cd ~/poolio-hub

# Create directory structure
mkdir -p {data/{influxdb,mosquitto/config,grafana},logs,config,scripts}

# Create initial config files
mkdir -p mosquitto/config
```

## Step 3: Create API Project Structure

```bash
cd ~/poolio-hub

# Create API project
mkdir -p api/src
mkdir -p web/dist

# Create API package.json
cat > api/package.json << 'EOF'
{
  "name": "poolio-api",
  "version": "1.0.0",
  "description": "PoolIO Hub API",
  "main": "dist/index.js",
  "scripts": {
    "build": "tsc",
    "start": "node dist/index.js",
    "dev": "ts-node src/index.ts"
  },
  "dependencies": {
    "express": "^4.18.2",
    "cors": "^2.8.5",
    "helmet": "^7.0.0",
    "mqtt": "^4.3.7",
    "@influxdata/influxdb-client": "^1.33.2",
    "ws": "^8.13.0",
    "jsonwebtoken": "^9.0.0",
    "bcryptjs": "^2.4.3"
  },
  "devDependencies": {
    "@types/node": "^20.4.5",
    "@types/express": "^4.17.17",
    "@types/cors": "^2.8.13",
    "@types/ws": "^8.5.5",
    "@types/bcryptjs": "^2.4.2",
    "@types/jsonwebtoken": "^9.0.2",
    "typescript": "^5.1.6",
    "ts-node": "^10.9.1"
  }
}
EOF

# Create API TypeScript config
cat > api/tsconfig.json << 'EOF'
{
  "compilerOptions": {
    "target": "ES2020",
    "module": "commonjs",
    "outDir": "./dist",
    "rootDir": "./src",
    "strict": true,
    "esModuleInterop": true,
    "skipLibCheck": true,
    "forceConsistentCasingInFileNames": true,
    "resolveJsonModule": true
  },
  "include": ["src/**/*"],
  "exclude": ["node_modules", "dist"]
}
EOF

# Create basic API source
cat > api/src/index.ts << 'EOF'
import express from 'express';
import cors from 'cors';
import helmet from 'helmet';
import { createServer } from 'http';
import { WebSocketServer } from 'ws';
import mqtt from 'mqtt';

const app = express();
const server = createServer(app);
const wss = new WebSocketServer({ server });
const port = process.env.PORT || 3000;

// Middleware
app.use(helmet());
app.use(cors());
app.use(express.json());

// MQTT Client
const mqttClient = mqtt.connect(process.env.MQTT_BROKER || 'mqtt://localhost:1883');

// WebSocket for real-time updates
wss.on('connection', (ws) => {
  console.log('Client connected');
  
  ws.on('close', () => {
    console.log('Client disconnected');
  });
});

// Basic health check
app.get('/health', (req, res) => {
  res.json({ 
    status: 'healthy', 
    timestamp: new Date().toISOString(),
    services: {
      mqtt: mqttClient.connected,
      influx: true // Will implement proper check later
    }
  });
});

// Basic sensor data endpoint
app.get('/api/sensors', (req, res) => {
  // Placeholder - will implement with InfluxDB
  res.json({
    temperature: 78.5,
    waterLevel: true,
    battery: 3.8,
    lastUpdate: new Date().toISOString()
  });
});

// MQTT message handling
mqttClient.on('connect', () => {
  console.log('Connected to MQTT broker');
  mqttClient.subscribe('poolio/+/data');
});

mqttClient.on('message', (topic, message) => {
  console.log(`Received: ${topic} - ${message.toString()}`);
  
  // Broadcast to WebSocket clients
  wss.clients.forEach(client => {
    if (client.readyState === 1) { // WebSocket.OPEN
      client.send(JSON.stringify({
        topic,
        data: message.toString(),
        timestamp: new Date().toISOString()
      }));
    }
  });
});

server.listen(port, () => {
  console.log(`PoolIO API running on port ${port}`);
});
EOF

# Create API Dockerfile
cat > api/Dockerfile << 'EOF'
FROM node:18-alpine

WORKDIR /app

# Copy package files
COPY package*.json ./

# Install all dependencies (including dev dependencies for build)
RUN npm install

# Copy source code
COPY src/ ./src/
COPY tsconfig.json ./

# Build the application
RUN npm run build

# Remove dev dependencies after build
RUN npm prune --omit=dev

EXPOSE 3000

CMD ["node", "dist/index.js"]
EOF
```

## Step 4: Create Web Interface

```bash
# Create basic web interface
cat > web/dist/index.html << 'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PoolIO Dashboard</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 20px; 
            background: #f5f5f5; 
        }
        .container { 
            max-width: 800px; 
            margin: 0 auto; 
            background: white; 
            padding: 20px; 
            border-radius: 8px; 
            box-shadow: 0 2px 4px rgba(0,0,0,0.1); 
        }
        .status { 
            display: grid; 
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); 
            gap: 20px; 
            margin: 20px 0; 
        }
        .card { 
            background: #f8f9fa; 
            padding: 15px; 
            border-radius: 6px; 
            border-left: 4px solid #007bff; 
        }
        .value { 
            font-size: 2em; 
            font-weight: bold; 
            color: #007bff; 
        }
        .label { 
            color: #666; 
            font-size: 0.9em; 
        }
        #connection-status { 
            padding: 10px; 
            border-radius: 4px; 
            margin-bottom: 20px; 
            font-weight: bold; 
        }
        .connected { 
            background: #d4edda; 
            color: #155724; 
        }
        .disconnected { 
            background: #f8d7da; 
            color: #721c24; 
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>PoolIO Dashboard</h1>
        
        <div id="connection-status" class="disconnected">
            🔴 Connecting...
        </div>
        
        <div class="status">
            <div class="card">
                <div class="label">Pool Temperature</div>
                <div class="value" id="temperature">--°F</div>
            </div>
            <div class="card">
                <div class="label">Water Level</div>
                <div class="value" id="water-level">--</div>
            </div>
            <div class="card">
                <div class="label">Battery</div>
                <div class="value" id="battery">--V</div>
            </div>
        </div>
        
        <div id="last-update" style="text-align: center; color: #666; margin-top: 20px;">
            Last update: --
        </div>
    </div>

    <script>
        // WebSocket connection for real-time updates
        let ws;
        
        function connect() {
            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const wsUrl = `${protocol}//${window.location.host}/ws`;
            
            // For now, use polling instead of WebSocket
            updateData();
            setInterval(updateData, 5000);
        }
        
        async function updateData() {
            try {
                const response = await fetch('/api/sensors');
                const data = await response.json();
                
                document.getElementById('temperature').textContent = `${data.temperature}°F`;
                document.getElementById('water-level').textContent = data.waterLevel ? 'OK' : 'LOW';
                document.getElementById('battery').textContent = `${data.battery}V`;
                document.getElementById('last-update').textContent = 
                    `Last update: ${new Date(data.lastUpdate).toLocaleTimeString()}`;
                
                // Update connection status
                const statusEl = document.getElementById('connection-status');
                statusEl.textContent = '🟢 Connected (Local Network)';
                statusEl.className = 'connected';
                
            } catch (error) {
                console.error('Failed to fetch data:', error);
                const statusEl = document.getElementById('connection-status');
                statusEl.textContent = '🔴 Connection Error';
                statusEl.className = 'disconnected';
            }
        }
        
        // Start connection when page loads
        connect();
    </script>
</body>
</html>
EOF

# Create web Dockerfile
cat > web/Dockerfile << 'EOF'
FROM nginx:alpine

# Copy built web assets
COPY dist/ /usr/share/nginx/html/

# Copy nginx config
COPY nginx.conf /etc/nginx/nginx.conf

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
EOF

# Create nginx config
cat > web/nginx.conf << 'EOF'
events {
    worker_connections 1024;
}

http {
    include /etc/nginx/mime.types;
    default_type application/octet-stream;

    server {
        listen 80;
        server_name localhost;
        root /usr/share/nginx/html;
        index index.html;

        # API proxy
        location /api/ {
            proxy_pass http://poolio-api:3000/api/;
            proxy_http_version 1.1;
            proxy_set_header Upgrade $http_upgrade;
            proxy_set_header Connection 'upgrade';
            proxy_set_header Host $host;
            proxy_cache_bypass $http_upgrade;
        }

        # Health check proxy
        location /health {
            proxy_pass http://poolio-api:3000/health;
        }

        # Serve static files
        location / {
            try_files $uri $uri/ /index.html;
        }
    }
}
EOF
```

## Step 5: Docker Compose Configuration

Create `docker-compose.yml`:
```yaml
services:
  mosquitto:
    image: eclipse-mosquitto:2.0
    container_name: poolio-mosquitto
    restart: unless-stopped
    ports:
      - "1883:1883"
      - "9001:9001"
    volumes:
      - ./mosquitto/config:/mosquitto/config
      - ./data/mosquitto:/mosquitto/data
      - ./logs/mosquitto:/mosquitto/log
    user: "${USER_ID}:${GROUP_ID}"

  influxdb:
    image: influxdb:2.7
    container_name: poolio-influxdb
    restart: unless-stopped
    ports:
      - "8086:8086"
    volumes:
      - ./data/influxdb:/var/lib/influxdb2
    environment:
      - DOCKER_INFLUXDB_INIT_MODE=setup
      - DOCKER_INFLUXDB_INIT_USERNAME=poolio
      - DOCKER_INFLUXDB_INIT_PASSWORD=${INFLUXDB_PASSWORD}
      - DOCKER_INFLUXDB_INIT_ORG=poolio
      - DOCKER_INFLUXDB_INIT_BUCKET=sensor-data

  api:
    build: ./api
    container_name: poolio-api
    restart: unless-stopped
    ports:
      - "3000:3000"
    volumes:
      - ./config:/app/config
      - ./logs:/app/logs
    environment:
      - NODE_ENV=production
      - INFLUXDB_URL=http://influxdb:8086
      - INFLUXDB_TOKEN=${INFLUXDB_TOKEN:-}
      - MQTT_BROKER=mosquitto://mosquitto:1883
    depends_on:
      - mosquitto
      - influxdb

  web:
    build: ./web
    container_name: poolio-web
    restart: unless-stopped
    ports:
      - "80:80"
    volumes:
      - ./web/dist:/usr/share/nginx/html
    depends_on:
      - api

networks:
  default:
    name: poolio-network
```

## Step 4: MQTT Broker Configuration

Create `mosquitto/config/mosquitto.conf`:
```conf
# Basic configuration
listener 1883
allow_anonymous true
persistence true
persistence_location /mosquitto/data/

# WebSocket support (for web interface)
listener 9001
protocol websockets

# Logging
log_dest file /mosquitto/log/mosquitto.log
log_type error
log_type warning
log_type notice
log_type information
log_timestamp true

# Security (basic - will enhance later)
password_file /mosquitto/config/passwd
```

Create password file (for future use):
```bash
# This will be used later when we add authentication
touch mosquitto/config/passwd
```

## Step 5: Basic API Service

Create `api/Dockerfile`:
```dockerfile
FROM node:18-alpine

WORKDIR /app

# Copy package files
COPY package*.json ./
RUN npm ci --only=production

# Copy source code
COPY src/ ./src/
COPY tsconfig.json ./

# Install TypeScript and build
RUN npm install -g typescript
RUN npm run build

EXPOSE 3000

CMD ["node", "dist/index.js"]
```

Create `api/package.json`:
```json
{
  "name": "poolio-api",
  "version": "1.0.0",
  "description": "PoolIO Hub API",
  "main": "dist/index.js",
  "scripts": {
    "build": "tsc",
    "start": "node dist/index.js",
    "dev": "ts-node src/index.ts"
  },
  "dependencies": {
    "express": "^4.18.2",
    "cors": "^2.8.5",
    "helmet": "^7.0.0",
    "mqtt": "^4.3.7",
    "@influxdata/influxdb-client": "^1.33.2",
    "ws": "^8.13.0",
    "jsonwebtoken": "^9.0.0",
    "bcryptjs": "^2.4.3"
  },
  "devDependencies": {
    "@types/node": "^20.4.5",
    "@types/express": "^4.17.17",
    "@types/ws": "^8.5.5",
    "typescript": "^5.1.6",
    "ts-node": "^10.9.1"
  }
}
```

Create basic API (`api/src/index.ts`):
```typescript
import express from 'express';
import cors from 'cors';
import helmet from 'helmet';
import { createServer } from 'http';
import { WebSocketServer } from 'ws';
import mqtt from 'mqtt';

const app = express();
const server = createServer(app);
const wss = new WebSocketServer({ server });
const port = process.env.PORT || 3000;

// Middleware
app.use(helmet());
app.use(cors());
app.use(express.json());

// MQTT Client
const mqttClient = mqtt.connect(process.env.MQTT_BROKER || 'mqtt://localhost:1883');

// WebSocket for real-time updates
wss.on('connection', (ws) => {
  console.log('Client connected');
  
  ws.on('close', () => {
    console.log('Client disconnected');
  });
});

// Basic health check
app.get('/health', (req, res) => {
  res.json({ 
    status: 'healthy', 
    timestamp: new Date().toISOString(),
    services: {
      mqtt: mqttClient.connected,
      influx: true // Will implement proper check later
    }
  });
});

// Basic sensor data endpoint
app.get('/api/sensors', (req, res) => {
  // Placeholder - will implement with InfluxDB
  res.json({
    temperature: 78.5,
    waterLevel: true,
    battery: 3.8,
    lastUpdate: new Date().toISOString()
  });
});

// MQTT message handling
mqttClient.on('connect', () => {
  console.log('Connected to MQTT broker');
  mqttClient.subscribe('poolio/+/data');
});

mqttClient.on('message', (topic, message) => {
  console.log(`Received: ${topic} - ${message.toString()}`);
  
  // Broadcast to WebSocket clients
  wss.clients.forEach(client => {
    if (client.readyState === 1) { // WebSocket.OPEN
      client.send(JSON.stringify({
        topic,
        data: message.toString(),
        timestamp: new Date().toISOString()
      }));
    }
  });
});

server.listen(port, () => {
  console.log(`PoolIO API running on port ${port}`);
});
```

Create `api/tsconfig.json`:
```json
{
  "compilerOptions": {
    "target": "ES2020",
    "module": "commonjs",
    "outDir": "./dist",
    "rootDir": "./src",
    "strict": true,
    "esModuleInterop": true,
    "skipLibCheck": true,
    "forceConsistentCasingInFileNames": true,
    "resolveJsonModule": true
  },
  "include": ["src/**/*"],
  "exclude": ["node_modules", "dist"]
}
```

## Step 6: Basic Web Interface

Create `web/Dockerfile`:
```dockerfile
FROM nginx:alpine

# Copy built web assets (will create these next)
COPY dist/ /usr/share/nginx/html/

# Copy nginx config
COPY nginx.conf /etc/nginx/nginx.conf

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
```

Create `web/nginx.conf`:
```nginx
events {
    worker_connections 1024;
}

http {
    include /etc/nginx/mime.types;
    default_type application/octet-stream;

    server {
        listen 80;
        server_name localhost;
        root /usr/share/nginx/html;
        index index.html;

        # API proxy
        location /api/ {
            proxy_pass http://poolio-api:3000/api/;
            proxy_http_version 1.1;
            proxy_set_header Upgrade $http_upgrade;
            proxy_set_header Connection 'upgrade';
            proxy_set_header Host $host;
            proxy_cache_bypass $http_upgrade;
        }

        # Health check proxy
        location /health {
            proxy_pass http://poolio-api:3000/health;
        }

        # Serve static files
        location / {
            try_files $uri $uri/ /index.html;
        }
    }
}
```

Create basic web interface (`web/dist/index.html`):
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PoolIO Dashboard</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 20px; 
            background: #f5f5f5; 
        }
        .container { 
            max-width: 800px; 
            margin: 0 auto; 
            background: white; 
            padding: 20px; 
            border-radius: 8px; 
            box-shadow: 0 2px 4px rgba(0,0,0,0.1); 
        }
        .status { 
            display: grid; 
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); 
            gap: 20px; 
            margin: 20px 0; 
        }
        .card { 
            background: #f8f9fa; 
            padding: 15px; 
            border-radius: 6px; 
            border-left: 4px solid #007bff; 
        }
        .value { 
            font-size: 2em; 
            font-weight: bold; 
            color: #007bff; 
        }
        .label { 
            color: #666; 
            font-size: 0.9em; 
        }
        #connection-status { 
            padding: 10px; 
            border-radius: 4px; 
            margin-bottom: 20px; 
            font-weight: bold; 
        }
        .connected { 
            background: #d4edda; 
            color: #155724; 
        }
        .disconnected { 
            background: #f8d7da; 
            color: #721c24; 
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>PoolIO Dashboard</h1>
        
        <div id="connection-status" class="disconnected">
            🔴 Connecting...
        </div>
        
        <div class="status">
            <div class="card">
                <div class="label">Pool Temperature</div>
                <div class="value" id="temperature">--°F</div>
            </div>
            <div class="card">
                <div class="label">Water Level</div>
                <div class="value" id="water-level">--</div>
            </div>
            <div class="card">
                <div class="label">Battery</div>
                <div class="value" id="battery">--V</div>
            </div>
        </div>
        
        <div id="last-update" style="text-align: center; color: #666; margin-top: 20px;">
            Last update: --
        </div>
    </div>

    <script>
        // WebSocket connection for real-time updates
        let ws;
        
        function connect() {
            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const wsUrl = `${protocol}//${window.location.host}/ws`;
            
            // For now, use polling instead of WebSocket
            updateData();
            setInterval(updateData, 5000);
        }
        
        async function updateData() {
            try {
                const response = await fetch('/api/sensors');
                const data = await response.json();
                
                document.getElementById('temperature').textContent = `${data.temperature}°F`;
                document.getElementById('water-level').textContent = data.waterLevel ? 'OK' : 'LOW';
                document.getElementById('battery').textContent = `${data.battery}V`;
                document.getElementById('last-update').textContent = 
                    `Last update: ${new Date(data.lastUpdate).toLocaleTimeString()}`;
                
                // Update connection status
                const statusEl = document.getElementById('connection-status');
                statusEl.textContent = '🟢 Connected (Local Network)';
                statusEl.className = 'connected';
                
            } catch (error) {
                console.error('Failed to fetch data:', error);
                const statusEl = document.getElementById('connection-status');
                statusEl.textContent = '🔴 Connection Error';
                statusEl.className = 'disconnected';
            }
        }
        
        // Start connection when page loads
        connect();
    </script>
</body>
</html>
```

## Step 7: Set User Permissions

```bash
# Set proper permissions
sudo chown -R $USER:$USER ~/poolio-hub
chmod -R 755 ~/poolio-hub
```

## Step 8: Initial Deployment

```bash
cd ~/poolio-hub

# Create environment file (use different variable names to avoid UID readonly issue)
echo "USER_ID=$(id -u)" > .env
echo "GROUP_ID=$(id -g)" >> .env

# Set secure InfluxDB password (CHANGE THIS!)
echo "INFLUXDB_PASSWORD=your-secure-password-here" >> .env

# Build and start services
docker compose up -d

# Check status
docker compose ps
docker compose logs -f
```

**If you get "permission denied" error:**
```bash
# You need to log out and back in for docker group permissions
exit
# Reconnect via SSH, then try again
ssh chenry@poolio-hub.local
cd ~/poolio-hub
docker compose up -d
```

## Step 9: Configure InfluxDB and Get Token

```bash
# Wait for InfluxDB to fully initialize (may take 30-60 seconds)
sleep 60

# Check InfluxDB is ready
curl http://localhost:8086/health

# Check if InfluxDB has started properly
docker compose logs influxdb | tail -10

# Get the token using the direct command
docker compose exec influxdb influx auth list --org poolio

# Copy the token from the output and add to environment file (replace with your actual token)
echo "INFLUXDB_TOKEN=your-token-from-above-command" >> .env

# Example: echo "INFLUXDB_TOKEN=fzVageqElwLNqduLY9uvzml4eS9hUXOjhhTZAvmW8iz7M80-wsKBDX_sXTCnUXzdRnCzAK3am5YEnjh1GVKfww==" >> .env

# Restart API service to pick up the token
docker compose restart api

# Verify .env file contents
cat .env
```

**Alternative: Web Interface Method**
If the command method doesn't work, visit `http://poolio-hub.local:8086`:
1. Login with username: `poolio`, password: `your-secure-password-here` (from Step 8)
2. Go to Data > API Tokens
3. Copy the existing token
4. Add it to `.env`: `echo "INFLUXDB_TOKEN=your-token-here" >> .env`
5. Restart API: `docker compose restart api`

## Step 10: Verify Installation

```bash
# Check if all services are running
docker compose ps

# Test API health check
curl http://localhost/health

# Install MQTT client tools for testing
sudo apt install mosquitto-clients

# Test MQTT broker
mosquitto_pub -h poolio-hub.local -t "test/topic" -m "Hello PoolIO"

# Optional: Listen for MQTT messages (Ctrl+C to stop)
mosquitto_sub -h poolio-hub.local -t "test/topic"

# Check API logs to see MQTT messages
docker compose logs api -f

# Check web dashboard
echo "Visit http://poolio-hub.local to see the dashboard"  
echo "Visit http://hub.local to see the dashboard"
echo "Visit http://poolio-hub.local:8086 for InfluxDB admin interface"
```

## Network Access

### ESP32 Configuration
Update your ESP32 code to use the hostname instead of IP addresses:
```cpp
// Old way:
// mqtt_client.setServer("YOUR_HUB_IP", 1883);

// New way:
mqtt_client.setServer("poolio-hub.local", 1883);
```

### Adding New Pool Devices
When you add new ESP32 devices, they will automatically get .local hostnames if they:
1. Set their hostname properly in the ESP32 code
2. Have avahi/mDNS support (available in most ESP32 libraries)

Example ESP32 hostname setup:
```cpp
#include <WiFi.h>
WiFi.setHostname("pool-sensor");
// Device will be available as pool-sensor.local
```

## Next Steps

1. **InfluxDB is automatically configured** - No manual setup needed! The Docker environment variables handle initialization.
2. **Test MQTT**: Use mosquitto_pub/sub to test messaging
3. **Update ESP32 nodes**: Modify to send data to local MQTT broker  
4. **Add authentication**: Secure the web interface
5. **Set up remote access**: Configure VPN and Cloudflare tunnel

### Optional: InfluxDB Web Interface Tour

If you want to explore InfluxDB's features, visit `http://poolio-hub.local:8086`:

**What's Already Configured:**
- ✅ **Organization**: `poolio` 
- ✅ **User**: `poolio` (with your secure password)
- ✅ **Bucket**: `sensor-data` (where pool data will be stored)
- ✅ **API Token**: Created automatically for API access

**Web Interface Features:**
1. **Data Explorer**: Query and visualize sensor data
2. **Dashboards**: Create custom pool monitoring dashboards  
3. **Tasks**: Set up automated data processing
4. **API Tokens**: Manage access tokens (already configured)
5. **Settings**: Manage users and organization settings

**Current Status**: Your InfluxDB is ready to receive data from the pool sensors. No additional configuration required!

## Troubleshooting

**If containers won't start:**
```bash
# Check logs
docker compose logs mosquitto
docker compose logs influxdb
docker compose logs api

# Restart services
docker compose restart

# Rebuild if needed
docker compose down
docker compose up --build -d
```

**If web interface shows connection errors:**
```bash
# Check API is responding
curl http://localhost:3000/health

# Check nginx is proxying correctly
docker compose logs web
```

This gets your basic Ubuntu hub running with MQTT, database, API, and web interface. Once this is working, we can enhance it with your ESP32 integration and remote access.