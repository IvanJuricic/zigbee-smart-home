import 'dart:developer';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

class ESP32State {
  BluetoothDevice? device;
  bool isBLEConnected = false;
  bool isWiFiConnected = false;
  List<BluetoothService> services = [];
  List<BluetoothCharacteristic> characteristics = [];

  ESP32State({this.device});

  Future<void> connectToDevice() async {
    try {
      await device?.connect();
      isBLEConnected = true;
      discoverServices();
    } catch (e) {
      log("Error connecting to device: $e");
    }
  }

  Future<void> writeLightCharacteristic(bool lightState) async {
    var characteristic = characteristics.firstWhere(
          (c) => c.uuid.toString().toUpperCase() == "6010",
    );

    log("Kar => $characteristic");

    if (characteristic.uuid.toString().toUpperCase() == "6010") {
      try {
        // Convert lightState to bytes and write to characteristic
        List<int> valueToWrite = lightState ? [1] : [0]; // For example, 1 for ON, 0 for OFF
        await characteristic.write(valueToWrite, withoutResponse: characteristic.properties.writeWithoutResponse);
        log("Light state written successfully");
      } catch (e) {
        log("Error writing to characteristic: $e");
      }
    } else {
      log("Characteristic not found");
    }
  }

  Future<void> discoverServices() async {
    try {
      services = (await device?.discoverServices())!;
      characteristics.clear();
      for (var service in services) {
        for (var characteristic in service.characteristics) {
          characteristics.add(characteristic);
        }
      }
    } catch (e) {
      log("Error discovering services: $e");
    }
  }

  // Function that updates characteristic values
  void updateCharacteristics(List<BluetoothCharacteristic> newCharacteristics) {
    log("tu smo updateamo karakteristike");
    characteristics = newCharacteristics;
    log("KARAKTERISTIKE => $characteristics");
  }

  // Add methods to update the state as needed
  void setBLEConnectionStatus(bool status, BluetoothDevice? d) {
    isBLEConnected = status;
    device = d;
    log("Uspili smo");
  }

  void setWiFiConnectionStatus(bool status) {
    isWiFiConnected = status;
  }

// ... other state management methods
}
