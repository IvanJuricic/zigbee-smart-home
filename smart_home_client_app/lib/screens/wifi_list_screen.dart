import 'dart:async';
import 'dart:convert';
import 'dart:developer';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../main.dart';
import '../utils/snackbar.dart';

class WifiListScreen extends StatefulWidget {
  final BluetoothDevice device;

  const WifiListScreen({Key? key, required this.device}) : super(key: key);

  @override
  State<WifiListScreen> createState() => _WifiListScreenState();
}

class _WifiListScreenState extends State<WifiListScreen> {
  List<String> wifiSSIDs = []; // List to store Wi-Fi SSIDs
  final List<BluetoothCharacteristic> _characteristics = []; // Initialize this list with relevant characteristics

  bool _isDiscoveringServices = false;
  bool _isConnected = false;
  late StreamSubscription<BluetoothConnectionState> _connectionStateSubscription;

  @override
  void initState() {
    super.initState();
    _setupConnectionListener();
    _connectToDevice();
  }

  void _promptForPassword(String ssid) {
    TextEditingController passwordController = TextEditingController();

    showDialog(
      context: context,
      builder: (context) {
        return AlertDialog(
          title: Text('Connect to $ssid'),
          content: TextField(
            controller: passwordController,
            decoration: InputDecoration(labelText: 'Password'),
            obscureText: true,
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(),
              child: Text('Cancel'),
            ),
            ElevatedButton(
              onPressed: () {
                // Implement your connection logic here
                // For example, send the SSID and password to the ESP32
                _sendCredentials(ssid, passwordController.text); // Send credentials to ESP32
                Navigator.of(context).pop();
              },
              child: Text('Connect'),
            ),
          ],
        );
      },
    );
  }

  void _setupConnectionListener() {
    _connectionStateSubscription = widget.device.connectionState.listen((state) {
      setState(() => _isConnected = state == BluetoothConnectionState.connected);
      if (state == BluetoothConnectionState.connected) {
        _discoverServices();
      }
    });
  }

  Future _sendCredentials(String ssid, String password) async {
    // Combine SSID and password, you might want a specific protocol or format
    String combinedCredentials = '$ssid:$password';
    // Convert the credentials to bytes and send them over the characteristic
    if (ssid.isNotEmpty && password.isNotEmpty) {
      try {
        for (BluetoothCharacteristic c in _characteristics) {
          // Check if the characteristic's UUID is for the SSID
          if (c.uuid.toString().toUpperCase() == "3B40") {
            List<int> ssidBytes = utf8.encode(ssid); // Convert SSID to bytes
            await c.write(ssidBytes, withoutResponse: c.properties.writeWithoutResponse);
            log("SSID sent successfully");
          }
          // Check if the characteristic's UUID is for the password
          else if (c.uuid.toString().toUpperCase() == "4240") {
            List<int> passwordBytes = utf8.encode(password); // Convert password to bytes
            await c.write(passwordBytes, withoutResponse: c.properties.writeWithoutResponse);
            log("Password sent successfully");
          }
        }

        Snackbar.show(ABC.c, "Credentials sent: Success", success: true);

      } catch (e) {
        Snackbar.show(ABC.c, prettyException("Write Error:", e),
            success: false);
      }
    }

    // Clear the text fields
    //_ssidController.clear();
    //_passwordController.clear();

    // Optionally, add logic to confirm the credentials were sent successfully
    await _confirmationListener();
  }

  Future _confirmationListener() async {
    try {
      for (var characteristic in _characteristics) {
        if (characteristic.uuid.toString().toUpperCase() == "4840") {
          await characteristic.setNotifyValue(true);
          characteristic.lastValueStream.listen((value) {
            if (value.isNotEmpty) {
              int receivedConfirmation = value[0];
              if (receivedConfirmation == 1) {
                debugPrint("Wi-Fi connected successfully!");
                navigatorKey.currentState?.popUntil((route) => route.isFirst);
              }
            }
          });
        }
      }
    } catch (e) {
      debugPrint("Error listening for confirmation: $e");
    }
  }


  Future _getWifiAPList() async {
    // Implementation to get the list of Wi-Fi APs
    try{
      for (var characteristic in _characteristics) {
        if (characteristic.uuid.toString().toUpperCase() == "5040") {
          // Clear previous SSIDs
          await characteristic.setNotifyValue(true);

          final subscription = characteristic.lastValueStream.listen((value) {
            // lastValueStream` is updated:
            //   - anytime read() is called
            //   - anytime write() is called
            //   - anytime a notification arrives (if subscribed)
            //   - also when first listened to, it re-emits the last value for convenience.
            log("Received data: $value");
            String ssidData = utf8.decode(value);
            List<String> ssids = ssidData.split('\n').where((s) => s.isNotEmpty).toList();
            setState(() {
              wifiSSIDs.addAll(ssids); // Update the list with new SSIDs
            });
          });

          widget.device.cancelWhenDisconnected(subscription);

          await characteristic.read();

          break;
        }
      }
    }catch(e){
      debugPrint("Error discovering services: $e");
    }finally{
      setState(() => _isDiscoveringServices = false);
    }
  }

  Future<void> _connectToDevice() async {
    try {
      await widget.device.connect();
    } catch (e) {
      debugPrint("Error connecting to device: $e");
    }
  }

  Future<void> _discoverServices() async {
    setState(() => _isDiscoveringServices = true);
    try {
      var services = await widget.device.discoverServices();
      _characteristics.clear();
      for (var service in services) {
        for (var characteristic in service.characteristics) {
          _characteristics.add(characteristic);
        }
      }

      await _getWifiAPList();

    } catch (e) {
      debugPrint("Error discovering services: $e");
    } finally {
      setState(() => _isDiscoveringServices = false);
    }
  }

  Future<void> _onRefresh() async {
    if (_isConnected) {
      wifiSSIDs.clear();
      await _discoverServices();
    }
  }

  @override
  void dispose() {
    _connectionStateSubscription.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text(widget.device.platformName)),
      body: RefreshIndicator(
        onRefresh: _onRefresh,
        child: _isDiscoveringServices
            ? const Center(child: CircularProgressIndicator())
            : wifiSSIDs.isEmpty
            ? const Center(child: Text('No Wi-Fi networks found. Pull down to refresh.'))
            : ListView.builder(
          itemCount: wifiSSIDs.length,
          itemBuilder: (context, index) {
            return Card(
              margin: EdgeInsets.all(8.0),
              child: ListTile(
                leading: Icon(Icons.wifi),
                title: Text(wifiSSIDs[index]),
                onTap: () => _promptForPassword(wifiSSIDs[index]), // Handle SSID tap
              ),
            );
          },
        ),
      ),
    );
  }

}
