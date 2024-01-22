import 'package:flutter/material.dart';

class MainControlScreen extends StatefulWidget {
  @override
  _MainControlScreenState createState() => _MainControlScreenState();
}

class _MainControlScreenState extends State<MainControlScreen> {
  bool _lightIsOn = false; // This variable keeps track of the light's state

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Main Screen'),
      ),
      body: Column(
        children: [
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
