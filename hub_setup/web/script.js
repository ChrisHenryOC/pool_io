class PoolIODashboard {
    constructor() {
        this.ws = null;
        this.reconnectInterval = null;
        this.heartbeatInterval = null;
        this.lastHeartbeat = Date.now();
        
        this.elements = {
            connectionStatus: document.getElementById('connectionStatus'),
            temperature: document.getElementById('temperature'),
            waterLevel: document.getElementById('waterLevel'),
            battery: document.getElementById('battery'),
            tempTimestamp: document.getElementById('tempTimestamp'),
            levelTimestamp: document.getElementById('levelTimestamp'),
            batteryTimestamp: document.getElementById('batteryTimestamp'),
            mqttStatus: document.getElementById('mqttStatus'),
            influxStatus: document.getElementById('influxStatus'),
            apiStatus: document.getElementById('apiStatus'),
            activityList: document.getElementById('activityList'),
            lastUpdate: document.getElementById('lastUpdate')
        };

        this.init();
    }

    init() {
        this.connectWebSocket();
        this.fetchInitialData();
        this.startHeartbeat();
        
        // Update last update timestamp every second
        setInterval(() => {
            this.elements.lastUpdate.textContent = new Date().toLocaleString();
        }, 1000);
    }

    connectWebSocket() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.hostname}:3000`;
        
        console.log('Connecting to WebSocket:', wsUrl);
        this.ws = new WebSocket(wsUrl);

        this.ws.onopen = () => {
            console.log('WebSocket connected');
            this.updateConnectionStatus(true);
            this.clearReconnectInterval();
        };

        this.ws.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                this.handleWebSocketMessage(data);
            } catch (error) {
                console.error('Error parsing WebSocket message:', error);
            }
        };

        this.ws.onclose = () => {
            console.log('WebSocket disconnected');
            this.updateConnectionStatus(false);
            this.scheduleReconnect();
        };

        this.ws.onerror = (error) => {
            console.error('WebSocket error:', error);
            this.updateConnectionStatus(false);
        };
    }

    handleWebSocketMessage(data) {
        console.log('Received data:', data);
        
        if (data.topic && data.data) {
            this.processSensorData(data.topic, data.data, data.timestamp);
        }
        
        this.lastHeartbeat = Date.now();
    }

    processSensorData(topic, data, timestamp) {
        try {
            const sensorData = typeof data === 'string' ? JSON.parse(data) : data;
            const formattedTime = new Date(timestamp).toLocaleTimeString();

            // Update metrics based on topic
            if (topic.includes('temperature')) {
                this.elements.temperature.textContent = sensorData.temperature || '--';
                this.elements.tempTimestamp.textContent = `Updated: ${formattedTime}`;
            }

            if (topic.includes('water_level')) {
                const level = sensorData.water_level ? 'Normal' : 'Low';
                this.elements.waterLevel.textContent = level;
                this.elements.waterLevel.className = `status ${sensorData.water_level ? 'normal' : 'warning'}`;
                this.elements.levelTimestamp.textContent = `Updated: ${formattedTime}`;
            }

            if (topic.includes('battery')) {
                this.elements.battery.textContent = sensorData.battery || '--';
                this.elements.batteryTimestamp.textContent = `Updated: ${formattedTime}`;
            }

            // Add to activity log
            this.addActivity(`${topic}: ${JSON.stringify(sensorData)}`, timestamp);

        } catch (error) {
            console.error('Error processing sensor data:', error);
        }
    }

    async fetchInitialData() {
        try {
            // Fetch API health
            const healthResponse = await fetch('/health');
            if (healthResponse.ok) {
                const health = await healthResponse.json();
                this.updateSystemStatus(health);
                this.elements.apiStatus.className = 'status-dot online';
            } else {
                this.elements.apiStatus.className = 'status-dot offline';
            }

            // Fetch initial sensor data
            const sensorsResponse = await fetch('/api/sensors');
            if (sensorsResponse.ok) {
                const sensors = await sensorsResponse.json();
                this.updateSensorDisplay(sensors);
            }

        } catch (error) {
            console.error('Error fetching initial data:', error);
            this.elements.apiStatus.className = 'status-dot offline';
        }
    }

    updateSystemStatus(health) {
        this.elements.mqttStatus.className = `status-dot ${health.services.mqtt ? 'online' : 'offline'}`;
        this.elements.influxStatus.className = `status-dot ${health.services.influx ? 'online' : 'offline'}`;
    }

    updateSensorDisplay(sensors) {
        if (sensors.temperature) {
            this.elements.temperature.textContent = sensors.temperature;
            this.elements.tempTimestamp.textContent = `Updated: ${new Date(sensors.lastUpdate).toLocaleTimeString()}`;
        }

        if (typeof sensors.waterLevel === 'boolean') {
            const level = sensors.waterLevel ? 'Normal' : 'Low';
            this.elements.waterLevel.textContent = level;
            this.elements.waterLevel.className = `status ${sensors.waterLevel ? 'normal' : 'warning'}`;
            this.elements.levelTimestamp.textContent = `Updated: ${new Date(sensors.lastUpdate).toLocaleTimeString()}`;
        }

        if (sensors.battery) {
            this.elements.battery.textContent = sensors.battery;
            this.elements.batteryTimestamp.textContent = `Updated: ${new Date(sensors.lastUpdate).toLocaleTimeString()}`;
        }
    }

    addActivity(message, timestamp) {
        const activityItem = document.createElement('div');
        activityItem.className = 'activity-item';
        
        const messageSpan = document.createElement('span');
        messageSpan.textContent = message;
        
        const timeSpan = document.createElement('span');
        timeSpan.textContent = new Date(timestamp).toLocaleTimeString();
        timeSpan.style.color = '#7f8c8d';
        timeSpan.style.fontSize = '0.9rem';
        
        activityItem.appendChild(messageSpan);
        activityItem.appendChild(timeSpan);

        // Clear "no data" message
        if (this.elements.activityList.querySelector('.no-data')) {
            this.elements.activityList.innerHTML = '';
        }

        // Add to top of list
        this.elements.activityList.insertBefore(activityItem, this.elements.activityList.firstChild);

        // Keep only last 10 items
        while (this.elements.activityList.children.length > 10) {
            this.elements.activityList.removeChild(this.elements.activityList.lastChild);
        }
    }

    updateConnectionStatus(connected) {
        const dot = this.elements.connectionStatus.querySelector('.dot');
        const text = this.elements.connectionStatus.querySelector('span:last-child');
        
        if (connected) {
            dot.className = 'dot online';
            text.textContent = 'Connected';
        } else {
            dot.className = 'dot offline';
            text.textContent = 'Disconnected';
        }
    }

    scheduleReconnect() {
        if (this.reconnectInterval) return;
        
        let attempts = 0;
        this.reconnectInterval = setInterval(() => {
            attempts++;
            console.log(`Reconnection attempt ${attempts}`);
            
            const dot = this.elements.connectionStatus.querySelector('.dot');
            const text = this.elements.connectionStatus.querySelector('span:last-child');
            dot.className = 'dot warning';
            text.textContent = `Reconnecting... (${attempts})`;
            
            this.connectWebSocket();
            
            // Stop trying after 10 attempts
            if (attempts >= 10) {
                this.clearReconnectInterval();
                dot.className = 'dot offline';
                text.textContent = 'Connection failed';
            }
        }, 5000);
    }

    clearReconnectInterval() {
        if (this.reconnectInterval) {
            clearInterval(this.reconnectInterval);
            this.reconnectInterval = null;
        }
    }

    startHeartbeat() {
        this.heartbeatInterval = setInterval(() => {
            // Check if we've received data recently
            if (Date.now() - this.lastHeartbeat > 30000) { // 30 seconds
                console.warn('No heartbeat received, connection may be stale');
                this.updateConnectionStatus(false);
            }
        }, 10000); // Check every 10 seconds
    }
}

// Initialize dashboard when page loads
document.addEventListener('DOMContentLoaded', () => {
    console.log('Initializing PoolIO Dashboard');
    new PoolIODashboard();
});