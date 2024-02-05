import 'dart:developer';

import 'package:flutter/material.dart';
import 'package:flutter_application_1/state/esp32_provider.dart';
import 'package:provider/provider.dart';

import '../state/MQTTManager.dart';
import '../state/esp32_state.dart';

class MainControlScreen extends StatefulWidget {
  @override
  _MainControlScreenState createState() => _MainControlScreenState();
}

class _MainControlScreenState extends State<MainControlScreen> {
  bool _lightIsOn = false; // This variable keeps track of the light's state
  final MQTTManager _mqttManager = MQTTManager();

  List<bool> _selections = [true, false]; // Initial selection - Bluetooth by default

  @override
  void initState() {
    super.initState();
    _mqttManager.initializeMQTTClient();
  }

  @override
  void dispose() {
    _mqttManager.disposeMQTTClient();
    super.dispose();
  }

  void _toggleLight(bool value) {
    log("Toggle light: $value");
    if (_selections[0]) { // If Bluetooth is selected
      if (mounted) {
        Provider.of<ESP32Provider>(context, listen: false)
            .state
            .writeLightCharacteristic(value)
            .then((_) {
          setState(() {
            _lightIsOn = value;
          });
        });
      }
    } else if (_selections[1]) { // If WiFi (MQTT) is selected
      String message = value ? "ON" : "OFF";
      log("Publishing message: $message");
      _mqttManager.publishMessage("client/toggle", message);
      setState(() {
        _lightIsOn = value;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    bool isBluetoothAvailable = Provider.of<ESP32Provider>(context, listen: false).state.isBLEConnected;
    bool isWifiAvailable = Provider.of<ESP32Provider>(context, listen: false).state.isWiFiConnected;
    // Determine which toggle button should be enabled and which should be selected
    // If both are available, but neither is selected, default to selecting Bluetooth
    if (isBluetoothAvailable && isWifiAvailable && !_selections[0] && !_selections[1]) {
      _selections = [true, false];
    } else {
      _selections = [isBluetoothAvailable && _selections[0], isWifiAvailable && _selections[1]];
    }
    return Scaffold(
      appBar: AppBar(
        title: const Text('Main Screen'),
        backgroundColor: Colors.grey[900], // Darker app bar
      ),
      body: Column(
        children: [
          // Connection buttons
          // Connection Buttons
          // Toggle between Bluetooth and WiFi
          Padding(
            padding: const EdgeInsets.all(8.0),
            child: ToggleButtons(
              children: <Widget>[
                Icon(Icons.bluetooth, color: isBluetoothAvailable ? (_selections[0] ? Colors.green : null) : Colors.grey),
                Icon(Icons.wifi, color: isWifiAvailable ? (_selections[1] ? Colors.green : null) : Colors.grey),
              ],
              isSelected: _selections,
              onPressed: (int index) {
                setState(() {
                  if ((index == 0 && isBluetoothAvailable) || (index == 1 && isWifiAvailable)) {
                    for (int buttonIndex = 0; buttonIndex < _selections.length; buttonIndex++) {
                      _selections[buttonIndex] = buttonIndex == index;
                    }
                  }
                });
                // Add your logic to handle Bluetooth or WiFi selection
              },
              fillColor: Colors.lightGreenAccent,
              selectedBorderColor: Colors.green,
              borderColor: Colors.grey,
              constraints: BoxConstraints(minWidth: 56, minHeight: 56), // Making buttons larger
            ),
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
                    onChanged: _toggleLight,
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
