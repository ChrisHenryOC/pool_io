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