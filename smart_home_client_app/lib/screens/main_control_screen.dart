import 'package:flutter/material.dart';
import 'package:flutter_application_1/state/esp32_provider.dart';
import 'package:provider/provider.dart';

class MainControlScreen extends StatefulWidget {
  @override
  _MainControlScreenState createState() => _MainControlScreenState();
}

class _MainControlScreenState extends State<MainControlScreen> {
  bool _lightIsOn = false; // This variable keeps track of the light's state

  @override
  Widget build(BuildContext context) {
    final esp32State = Provider.of<ESP32Provider>(context).state;

    return Scaffold(
      appBar: AppBar(
        title: Text('Main Screen'),
        backgroundColor: Colors.grey[900], // Darker app bar
      ),
      body: Column(
        children: [
          // Connection buttons
          // Connection Buttons
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              IconButton(
                icon: Icon(Icons.wifi,
                    color: esp32State.isWiFiConnected ? Colors.green : Colors.grey),
                onPressed: () {
                  // Logic to handle WiFi connection
                },
                iconSize: 40.0,
              ),
              SizedBox(width: 20),
              IconButton(
                icon: Icon(Icons.bluetooth,
                    color: esp32State.isBLEConnected ? Colors.green : Colors.grey),
                onPressed: () {
                  // Logic to handle Bluetooth connection
                },
                iconSize: 40.0,
              ),
            ],
          ),
          SizedBox(height: 20),
          // Sensor Readings Section
          Expanded(
            flex: 1, // Adjust flex ratio to change the size of sections
            child: Container(
              padding: EdgeInsets.all(10),
              decoration: BoxDecoration(
                color: Colors.grey.shade200, // Shaded background
                borderRadius: BorderRadius.circular(10),
              ),
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Text(
                    'Reading from sensor',
                    style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
                  ),
                  SizedBox(height: 20),
                  Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Text('Degrees [°C]: ', style: TextStyle(fontSize: 20)),
                      // Replace with actual sensor value
                      Text('0.0', style: TextStyle(fontSize: 20)),
                    ],
                  ),
                  SizedBox(height: 10),
                  Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Text('Moisture: ', style: TextStyle(fontSize: 20)),
                      // Replace with actual sensor value
                      Text('0%', style: TextStyle(fontSize: 20)),
                    ],
                  ),
                ],
              ),
            ),
          ),

          // Light Toggle Section
          Expanded(
            flex: 1, // Adjust flex ratio to change the size of sections
            child: Center(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Text(
                    'Toggle Lights',
                    style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
                  ),
                  Switch(
                    value: _lightIsOn,
                    onChanged: (bool value) {
                      setState(() {
                        _lightIsOn = value;
                        // Add logic to send command to ESP32 to toggle lights
                      });
                    },
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}
