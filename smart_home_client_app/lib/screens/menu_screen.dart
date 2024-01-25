import 'package:flutter/material.dart';
import 'package:flutter_application_1/screens/main_control_screen.dart';
//import 'package:flutter_application_1/screens/scan_screen.dart';

import '../screens/scan_screen.dart';

class MenuScreen extends StatelessWidget {
  const MenuScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Menu Screen', textDirection: TextDirection.ltr),
        backgroundColor: Colors.grey[900], // Darker app bar
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            _buildButton(
              context,
              'Central Hub Configuration',
              Colors.blueGrey[600], // Subdued blue color
                  () => Navigator.of(context).push(
                MaterialPageRoute(builder: (context) => const ScanScreen()),
              ),
            ),
            const SizedBox(height: 20),
            _buildButton(
              context,
              'Home',
              Colors.green[600], // Subdued green color
                  () => Navigator.of(context).push(
                MaterialPageRoute(builder: (context) => MainControlScreen()),
              ),
            ),
            const SizedBox(height: 20),
            _buildButton(
              context,
              'Go to Screen Three',
              Colors.red[600], // Subdued red color
                  () {},
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildButton(BuildContext context, String text, Color? color, VoidCallback onPressed) {
    return ElevatedButton(
      style: ElevatedButton.styleFrom(
        primary: color, // Background color
        onPrimary: Colors.white, // Text color
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(10),
        ),
        padding: EdgeInsets.symmetric(horizontal: 30, vertical: 15),
        textStyle: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
        elevation: 5, // Adding some shadow
      ),
      onPressed: onPressed,
      child: Text(text, textDirection: TextDirection.ltr),
    );
  }
}
