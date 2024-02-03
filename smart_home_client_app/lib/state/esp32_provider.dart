import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'esp32_state.dart';

class ESP32Provider with ChangeNotifier {
  final ESP32State _state;

  ESP32Provider() : _state = ESP32State();

  ESP32State get state => _state;

  void updateBLEConnectionStatus(bool isConnected, [BluetoothDevice? device]) {
    _state.setBLEConnectionStatus(isConnected, device);
    notifyListeners();
  }

  void updateWiFiConnectionStatus(bool isConnected) {
    _state.setWiFiConnectionStatus(isConnected);
    notifyListeners();
  }

  void updateCharacteristics(List<BluetoothCharacteristic> newCharacteristics) {
    _state.updateCharacteristics(newCharacteristics);
    notifyListeners();
  }

// ... other methods to update the state
}
