import 'package:flutter_blue_plus/flutter_blue_plus.dart';

class ESP32State {
  bool isBLEConnected, isWiFiConnected;
  // Add other relevant fields here, e.g., connection type, data from ESP32, etc.
  BluetoothDevice? connectedDevice;

  ESP32State({this.isBLEConnected = false, this.isWiFiConnected = false, this.connectedDevice});

  // Add methods to update the state as needed
  void setBLEConnectionStatus(bool status, BluetoothDevice? device) {
    isBLEConnected = status;
    connectedDevice = device;
  }

  void setWiFiConnectionStatus(bool status) {
    isWiFiConnected = status;
  }

// ... other state management methods
}
