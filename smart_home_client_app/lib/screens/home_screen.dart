import 'package:flutter/material.dart';
import 'bluetooth_off_screen.dart';

class HomeScreen extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Center(
        child: ElevatedButton(
          onPressed: () {
            // Navigator.of(context).push(MaterialPageRoute(
            //   builder: (context) => BluetoothScanScreen(),
            // ));
          },
          child: Text('Go to Bluetooth Scan'),
        ),
      ),
    );
  }
}
