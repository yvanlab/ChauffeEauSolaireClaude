#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ESP32 Solar Spa Controller - Web Interface Simulator
Simulates the ESP32 web server to test the interface without hardware
"""

from http.server import HTTPServer, BaseHTTPRequestHandler
import json
import time
import random
import os
from urllib.parse import urlparse, parse_qs

# Simulated system state
class SystemState:
    def __init__(self):
        self.start_time = time.time()
        self.air_temp = 20.5
        self.spa_temp = 20.1
        self.panel_temp = 15.0
        self.pump_state = False
        self.manual_override = False
        self.temp_diff = 5.0
        self.hysteresis = 1.0
        self.min_external = 20.0
        self.max_spa = 38.0
        self.sample_interval = 60
        self.sample_duration = 30
        self.air_offset = 0.0
        self.spa_offset = 0.0
        self.panel_offset = 0.0
        self.wifi_ssid = "freebox"
        self.wifi_hostname = "chauffeSpa"
        self.total_pump_hours = 124.52
        self.logs = []
        self.history = []
        self.daily_history = []
        self.day_min_spa = 100.0
        self.day_max_spa = -100.0
        self.day_pump_hours = 0.0
        self.last_saved_day = -1
        # Simulated sensors with unique addresses
        self.sensors = [
            {"address": "28-FF-64-1E-03-15-01-9A", "role": "air", "temp": 20.5},
            {"address": "28-FF-64-1E-03-15-02-B3", "role": "spa", "temp": 30.1},
            {"address": "28-FF-64-1E-03-15-03-C7", "role": "panel", "temp": 25.0}
        ]
        self.use_sensor_mapping = False
        self.add_log("OK", "System simulator started")
        self._generate_initial_history()
        self._generate_daily_history()

    def add_log(self, level, message):
        entry = {
            "time": time.strftime("%H:%M:%S"),
            "level": level,
            "message": message,
            "icon": {"INFO": "[i]", "WARN": "[!]", "ERROR": "[X]", "OK": "[OK]"}[level]
        }
        self.logs.append(entry)
        if len(self.logs) > 100:
            self.logs.pop(0)

    def _generate_initial_history(self):
        """Generate realistic 24-hour temperature history for simulator"""
        current_time = int(time.time() * 1000)  # milliseconds

        # Generate 144 points (last 24 hours, every 10 minutes for demo)
        for i in range(144, 0, -1):
            timestamp = current_time - (i * 10 * 60 * 1000)  # 10 minutes apart
            hour_of_day = ((time.time() - (i * 10 * 60)) % 86400) / 3600

            # Simulate daily temperature cycle
            # Air: cooler at night (18-22°C), warmer during day (22-28°C)
            air_base = 22 + 4 * (0.5 - 0.5 * abs(hour_of_day - 14) / 14)
            air_temp = air_base + random.uniform(-1, 1)

            # Spa: slowly varying (28-35°C), heated during sunny hours
            spa_base = 30 + 3 * (0.5 - 0.5 * abs(hour_of_day - 15) / 15)
            spa_temp = spa_base + random.uniform(-0.5, 0.5)

            # Panel: follows sun strongly (15-50°C), peaks around noon
            if 8 <= hour_of_day <= 18:
                panel_base = 20 + 30 * (1 - abs(hour_of_day - 13) / 5)
            else:
                panel_base = 18 + random.uniform(-3, 2)
            panel_temp = panel_base + random.uniform(-2, 2)

            # Simulate pump activation: ON when panel > spa + 5°C
            pump_on = panel_temp > (spa_temp + self.temp_diff) and panel_temp > self.min_external

            self.history.append({
                "t": timestamp,
                "a": round(air_temp, 1),
                "s": round(spa_temp, 1),
                "p": round(panel_temp, 1),
                "pump": 1 if pump_on else 0
            })

        # Keep only last 1440 points (24 hours at 1-minute intervals)
        if len(self.history) > 1440:
            self.history = self.history[-1440:]

    def _generate_daily_history(self):
        """Generate 90 days of min/max stats for the new history page"""
        now = time.time()
        for i in range(90, 0, -1):
            timestamp = now - (i * 86400)
            date_str = time.strftime("%Y-%m-%d", time.localtime(timestamp))
            
            # Simulate seasonal variation (warmer 45 days ago)
            seasonal_factor = 1.0 - abs(45 - i) / 90.0
            base_temp = 25.0 + (10.0 * seasonal_factor)
            
            min_t = base_temp + random.uniform(-2, 0)
            max_t = base_temp + random.uniform(2, 6)
            hours = random.uniform(0.5, 8.0)
            
            self.daily_history.append({
                "d": date_str,
                "min": round(min_t, 1),
                "max": round(max_t, 1),
                "c": round(hours, 2)
            })

    def update_temps(self):
        prev_pump = self.pump_state
        # Simulate temperature variations
        self.air_temp += random.uniform(-0.2, 0.2)
        self.spa_temp += random.uniform(-0.1, 0.1)
        self.panel_temp += random.uniform(-0.5, 0.5)

        # Keep in realistic ranges
        self.air_temp = max(15, min(30, self.air_temp))
        self.spa_temp = max(15, min(45, self.spa_temp))
        self.panel_temp = max(10, min(60, self.panel_temp))

        # Update simulation pump state if in auto mode
        if not self.manual_override:
            self.pump_state = (self.panel_temp > (self.spa_temp + self.temp_diff) and
                              self.panel_temp > self.min_external and
                              self.spa_temp < self.max_spa)

        # Update pump runtime in simulator (approximate)
        if self.pump_state:
            self.total_pump_hours += (2.0 / 3600.0)
            self.day_pump_hours += (2.0 / 3600.0)

        # Update daily extremes
        if 0.1 < self.spa_temp < 90.0:
            if self.spa_temp < self.day_min_spa: self.day_min_spa = self.spa_temp
            if self.spa_temp > self.day_max_spa: self.day_max_spa = self.spa_temp

        # Simulate 11 PM save logic
        current_struct = time.localtime()
        if current_struct.tm_hour == 23 and current_struct.tm_mday != self.last_saved_day:
            date_str = time.strftime("%Y-%m-%d")
            self.daily_history.append({
                "d": date_str, 
                "min": round(self.day_min_spa, 1), 
                "max": round(self.day_max_spa, 1),
                "c": round(self.day_pump_hours, 2)
            })
            self.last_saved_day = current_struct.tm_mday
            self.day_min_spa = 100.0
            self.day_max_spa = -100.0
            self.day_pump_hours = 0.0
            self.add_log("OK", f"Daily extremes saved for {date_str}")

        # Update sensor temps
        for sensor in self.sensors:
            if sensor["role"] == "air":
                sensor["temp"] = round(self.air_temp - self.air_offset, 1)
            elif sensor["role"] == "spa":
                sensor["temp"] = round(self.spa_temp - self.spa_offset, 1)
            elif sensor["role"] == "panel":
                sensor["temp"] = round(self.panel_temp - self.panel_offset, 1)

        # Add to history (simulate 1-minute recording)
        current_time = int(time.time() * 1000)
        if not self.history or (current_time - self.history[-1]["t"]) >= 60000:
            # Determine pump state
            pump_on = (self.panel_temp > (self.spa_temp + self.temp_diff) and
                      self.panel_temp > self.min_external and
                      self.spa_temp < self.max_spa)

            self.history.append({
                "t": current_time,
                "a": round(self.air_temp, 1),
                "s": round(self.spa_temp, 1),
                "p": round(self.panel_temp, 1),
                "pump": 1 if pump_on else 0
            })
            # Keep only last 1440 points
            if len(self.history) > 1440:
                self.history.pop(0)

state = SystemState()

class SimulatorHandler(BaseHTTPRequestHandler):

    def log_message(self, format, *args):
        # Suppress default logging
        pass

    def do_GET(self):
        parsed_path = urlparse(self.path)

        # Serve index.html
        if parsed_path.path == '/' or parsed_path.path == '/index.html':
            self.serve_file('data/index.html', 'text/html')

        # Serve history3m.html
        elif parsed_path.path == '/history3m':
            self.serve_file('data/history3m.html', 'text/html')

        # Sensor data endpoint
        elif parsed_path.path == '/data':
            state.update_temps()
            data = {
                "airTemp": round(state.air_temp, 1),
                "spaTemp": round(state.spa_temp, 1),
                "panelTemp": round(state.panel_temp, 1),
                "pumpState": state.pump_state,
                "tempDiff": state.temp_diff,
                "hysteresis": state.hysteresis,
                "minExternal": state.min_external,
                "maxSpa": state.max_spa,
                "sampleInterval": state.sample_interval,
                "sampleDuration": state.sample_duration,
                "manualOverride": state.manual_override,
                "totalPumpHours": round(state.total_pump_hours, 2),
                "dayPumpHours": round(state.day_pump_hours, 2),
                "wifiSSID": state.wifi_ssid,
                "wifiPassword": "password123",
                "wifiHostname": state.wifi_hostname,
                "wifiIP": "192.168.1.50",
                "wifiRSSI": -63
            }
            self.send_json(data)

        # System info endpoint
        elif parsed_path.path == '/system/info':
            uptime = int(time.time() - state.start_time)

            # Simulate realistic ESP32 heap values
            totalHeap = 327680  # ~320 KB total RAM
            historyBufferSize = 1440 * 16  # 23,040 bytes for history
            baseUsed = 23000  # Base system usage
            usedHeap = baseUsed + historyBufferSize + random.randint(-1000, 1000)
            freeHeap = totalHeap - usedHeap
            heapPercent = (usedHeap / totalHeap) * 100
            minFreeHeap = freeHeap - random.randint(5000, 10000)  # Minimum ever seen
            maxAllocHeap = freeHeap - 4096  # Max single allocation

            info = {
                "version": "2.4",
                "buildDate": "Apr 20 2026",
                "buildTime": "12:00:00",
                "fsBuildDate": "Apr 20 2026",
                "fsBuildTime": "12:05:00",
                "chipModel": "ESP32-SIMULATOR",
                "cpuFreq": 240,
                "flashSize": 4194304,
                "freeHeap": freeHeap,
                "totalHeap": totalHeap,
                "usedHeap": usedHeap,
                "heapPercent": round(heapPercent, 1),
                "minFreeHeap": minFreeHeap,
                "maxAllocHeap": maxAllocHeap,
                "historyBufferSize": historyBufferSize,
                "uptime": uptime,
                "sketchSize": 902801,
                "sketchTotal": 1310720,
                "sketchPercent": 68.9,
                "fsUsed": 14336,
                "fsTotal": 1441792,
                "fsPercent": 0.99
            }
            self.send_json(info)

        # Logs endpoint
        elif parsed_path.path == '/logs':
            self.send_json(state.logs)

        # WiFi scan start
        elif parsed_path.path == '/wifi/scan/start':
            state.add_log("INFO", "Starting WiFi scan")
            self.send_text("Scan started")

        # WiFi scan results
        elif parsed_path.path == '/wifi/scan':
            networks = [
                {"ssid": "freebox", "rssi": -45, "encryption": 3},
                {"ssid": "Orange-WiFi", "rssi": -62, "encryption": 3},
                {"ssid": "SFR_Box", "rssi": -58, "encryption": 3},
                {"ssid": "Bouygues", "rssi": -75, "encryption": 3},
                {"ssid": "Public-WiFi", "rssi": -80, "encryption": 0},
                {"ssid": "Neighbor-2.4G", "rssi": -71, "encryption": 3}
            ]
            self.send_json(networks)

        # Temperature history endpoint
        elif parsed_path.path == '/history':
            history_data = {
                "points": state.history,
                "count": len(state.history)
            }
            self.send_json(history_data)

        # Daily history endpoint (JSON)
        elif parsed_path.path == '/history/daily':
            self.send_json({"points": state.daily_history})

        # Daily history CSV download
        elif parsed_path.path == '/history/daily/csv':
            csv_content = "Date,Min,Max,Heures_Pompe\n"
            for p in state.daily_history:
                csv_content += f"{p['d']},{p['min']},{p['max']},{p['c']}\n"
            self.send_response(200)
            self.send_header('Content-Type', 'text/csv')
            self.send_header('Content-Disposition', 'attachment; filename=daily_stats.csv')
            self.end_headers()
            self.wfile.write(csv_content.encode('utf-8'))

        # Sensors list endpoint
        elif parsed_path.path == '/sensors':
            state.update_temps()
            sensors_data = {
                "sensors": [
                    {
                        "index": i,
                        "address": sensor["address"],
                        "role": sensor["role"],
                        "temp": sensor["temp"],
                        "offset": state.air_offset if sensor["role"] == "air" else (state.spa_offset if sensor["role"] == "spa" else state.panel_offset)
                    }
                    for i, sensor in enumerate(state.sensors)
                ],
                "useMapping": state.use_sensor_mapping,
                "count": len(state.sensors)
            }
            self.send_json(sensors_data)

        else:
            self.send_404()

    def do_POST(self):
        parsed_path = urlparse(self.path)
        content_length = int(self.headers.get('Content-Length', 0))
        post_data = self.rfile.read(content_length).decode('utf-8')
        params = parse_qs(post_data)

        # Update temperature config
        if parsed_path.path == '/config':
            if 'tempDiff' in params:
                state.temp_diff = float(params['tempDiff'][0])
            if 'hysteresis' in params:
                state.hysteresis = float(params['hysteresis'][0])
            if 'minExternal' in params:
                state.min_external = float(params['minExternal'][0])
            if 'maxSpa' in params:
                state.max_spa = float(params['maxSpa'][0])
            if 'sampleInterval' in params:
                state.sample_interval = int(params['sampleInterval'][0])
            if 'sampleDuration' in params:
                state.sample_duration = int(params['sampleDuration'][0])
            state.add_log("OK", f"Temperature config updated: diff={state.temp_diff}")
            self.send_text("Configuration saved")

        # Pump control
        elif parsed_path.path == '/pump':
            if 'manual' in params:
                mode = params['manual'][0]
                if mode == 'on':
                    state.pump_state = True
                    state.manual_override = True
                    state.add_log("INFO", "Pump: MANUAL ON")
                elif mode == 'off':
                    state.pump_state = False
                    state.manual_override = True
                    state.add_log("INFO", "Pump: MANUAL OFF")
                elif mode == 'auto':
                    state.manual_override = False
                    state.add_log("INFO", "Pump: AUTO MODE")
            self.send_text("Pump state updated")

        # WiFi config
        elif parsed_path.path == '/wifi':
            if 'ssid' in params:
                state.wifi_ssid = params['ssid'][0]
                state.add_log("OK", f"WiFi SSID updated: {state.wifi_ssid}")
            self.send_text("WiFi configuration saved")

        # Clear logs
        elif parsed_path.path == '/logs/clear':
            state.logs.clear()
            state.add_log("INFO", "Logs cleared")
            self.send_text("Logs cleared")

        # Reset config
        elif parsed_path.path == '/reset':
            state.temp_diff = 5.0
            state.hysteresis = 1.0
            state.min_external = 20.0
            state.max_spa = 38.0
            state.sample_interval = 60
            state.sample_duration = 30
            state.add_log("WARN", "Configuration reset to defaults")
            self.send_text("Configuration reset")

        # Sensor mapping
        elif parsed_path.path == '/sensors/mapping':
            if 'airAddr' in params and 'spaAddr' in params and 'panelAddr' in params:
                air_addr = params['airAddr'][0]
                spa_addr = params['spaAddr'][0]
                panel_addr = params['panelAddr'][0]

                # Update sensor roles
                for sensor in state.sensors:
                    if sensor['address'] == air_addr:
                        sensor['role'] = 'air'
                    elif sensor['address'] == spa_addr:
                        sensor['role'] = 'spa'
                    elif sensor['address'] == panel_addr:
                        sensor['role'] = 'panel'

                state.use_sensor_mapping = True
                if 'airOffset' in params: state.air_offset = float(params['airOffset'][0])
                if 'spaOffset' in params: state.spa_offset = float(params['spaOffset'][0])
                if 'panelOffset' in params: state.panel_offset = float(params['panelOffset'][0])

                state.add_log("OK", f"Sensor mapping updated")
                self.send_text("Sensor mapping saved - restart required")
            else:
                self.send_text("Invalid sensor mapping parameters")

        # Restart endpoint
        elif parsed_path.path == '/restart':
            state.add_log("WARN", "Restart requested (simulated)")
            self.send_text("Restarting ESP32...")

        # OTA Update endpoints
        elif parsed_path.path == '/update/firmware' or parsed_path.path == '/update/filesystem':
            file_type = "firmware" if "firmware" in parsed_path.path else "filesystem"
            state.add_log("OK", f"Simulated {file_type} update successful")
            self.send_text(f"{file_type.capitalize()} updated")

        else:
            self.send_404()

    def serve_file(self, filepath, content_type):
        try:
            with open(filepath, 'rb') as f:
                content = f.read()
            self.send_response(200)
            self.send_header('Content-Type', content_type)
            self.send_header('Content-Length', len(content))
            self.end_headers()
            self.wfile.write(content)
        except FileNotFoundError:
            self.send_404()

    def send_json(self, data):
        content = json.dumps(data).encode('utf-8')
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', len(content))
        self.end_headers()
        self.wfile.write(content)

    def send_text(self, text):
        content = text.encode('utf-8')
        self.send_response(200)
        self.send_header('Content-Type', 'text/plain')
        self.send_header('Content-Length', len(content))
        self.end_headers()
        self.wfile.write(content)

    def send_404(self):
        self.send_response(404)
        self.send_header('Content-Type', 'text/plain')
        self.end_headers()
        self.wfile.write(b'404 Not Found')

def run_simulator(port=8080):
    server_address = ('', port)
    httpd = HTTPServer(server_address, SimulatorHandler)

    print("=" * 60)
    print("  ESP32 Solar Spa Controller - Web Simulator")
    print("=" * 60)
    print(f"\n  Server running at: http://localhost:{port}")
    print(f"  Or access via:     http://127.0.0.1:{port}")
    print("\n  Press Ctrl+C to stop the simulator\n")
    print("=" * 60)

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n\nSimulator stopped.")
        httpd.shutdown()

if __name__ == '__main__':
    # Change to script directory
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    run_simulator(8080)
