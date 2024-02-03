import 'dart:async';
import 'dart:convert';
import 'dart:developer';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../main.dart';
import '../utils/snackbar.dart';

import 'package:provider/provider.dart';
import '../state/esp32_provider.dart';

class WifiListScreen extends StatefulWidget {
  final BluetoothDevice device;

  const WifiListScreen({Key? key, required this.device}) : super(key: key);

  @override
  State<WifiListScreen> createState() => _WifiListScreenState();
}

class _WifiListScreenState extends State<WifiListScreen> {
  List<String> wifiSSIDs = []; // List to store Wi-Fi SSIDs
  final List<BluetoothCharacteristic> _characteristics = [
  ]; // Initialize this list with relevant characteristics
  final List<BluetoothService> _services = [
  ]; // Initialize this list with relevant services

  bool _isDiscoveringServices = false;
  bool _isConnected = false;
  static bool _isWifiConnected = false;
  bool _isScanningAPs = false;

  late StreamSubscription<BluetoothConnectionState> _connectionStateSubscription;

  @override
  void initState() {
    super.initState();
    _setupConnectionListener();
    if (_isConnected == false) _connectToDevice();
    _handleWiFiConnection(context, _isConnected);
    _getWifiConnectionStatus();
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
                _sendCredentials(
                    ssid, passwordController.text); // Send credentials to ESP32
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
    _connectionStateSubscription =
        widget.device.connectionState.listen((state) {
          setState(() =>
          _isConnected = state == BluetoothConnectionState.connected);
          if (state == BluetoothConnectionState.connected) {
            _discoverServices();
          }
        });
  }

  void _handleWiFiConnection(BuildContext context, bool isConnected) {
    Provider.of<ESP32Provider>(context, listen: false).updateWiFiConnectionStatus(isConnected);
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
            await c.write(
                ssidBytes, withoutResponse: c.properties.writeWithoutResponse);
            log("SSID sent successfully");
          }
          // Check if the characteristic's UUID is for the password
          else if (c.uuid.toString().toUpperCase() == "4240") {
            List<int> passwordBytes = utf8.encode(
                password); // Convert password to bytes
            await c.write(passwordBytes,
                withoutResponse: c.properties.writeWithoutResponse);
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
        if (["4840", "5210"].contains(characteristic.uuid.toString().toUpperCase())) {
          await characteristic.setNotifyValue(true);
          final subscription = characteristic.lastValueStream.listen(
                (value) => _handleCharacteristicValue(value),
          );
          widget.device.cancelWhenDisconnected(subscription);
        }
      }
    } catch (e) {
      debugPrint("Error listening for confirmation: $e");
    }
  }

  void _handleCharacteristicValue(List<int> value) {
    if (value.isNotEmpty) {
      String confirmation = utf8.decode(value);
      log("Received confirmation: $confirmation");
      if (["CONNECTED", "DISCONNECTED"].contains(confirmation)) {
        bool isConnected = confirmation == "CONNECTED";
        debugPrint("AAAAAAAAAAAAAAAAAA: $isConnected");
        _updateConnectionStatus(isConnected);
      } else {
        debugPrint("Unknown confirmation received: $confirmation");
      }
    }
  }

  void _updateConnectionStatus(bool isConnected) {
    if (mounted) {
      setState(() => _isWifiConnected = isConnected);

      _handleWiFiConnection(context, _isWifiConnected);

      String message = isConnected ? "Connected to Wi-Fi" : "Disconnected from Wi-Fi";
      Snackbar.show(ABC.c, message, success: true);

      if (isConnected) {
        // Navigate only if the widget is still mounted
        //Navigator.of(context).popUntil((route) => route.isFirst);
      }
    }
  }


  /*
  Future _confirmationListener() async {
    try {
      for (var characteristic in _characteristics) {
        if (characteristic.uuid.toString().toUpperCase() == "4840") {
          await characteristic.setNotifyValue(true);
          characteristic.lastValueStream.listen((value) {
            if (value.isNotEmpty) {
              String confirmation = utf8.decode(value);
              log("Received confirmation: $confirmation");
              if (confirmation == "CONNECTED" ||
                  confirmation == "DISCONNECTED") {
                debugPrint("Confirmation received: $confirmation");
                // Check if the widget is still in the widget tree
                if (mounted) {
                  log("Tu smo");
                  setState(() {
                    _isWifiConnected = confirmation == "CONNECTED";
                  });

                  if (_isWifiConnected) {
                    Snackbar.show(ABC.c, "Connected to Wi-Fi", success: true);
                    // Navigate only if the widget is still mounted after 2 seconds
                    /* Future.delayed(const Duration(seconds: 2), () =>
                        Navigator.of(context).popUntil((route) => route
                            .isFirst));*/
                    //Navigator.of(context).popUntil((route) => route.isFirst)
                  } else {
                    Snackbar.show(
                        ABC.c, "Disconnected from Wi-Fi", success: true);
                  }
                }
              } else {
                debugPrint("Unknown confirmation received: $confirmation");
              }
            }
          });
        }
        else if (characteristic.uuid.toString().toUpperCase() == "5210") {
          await characteristic.setNotifyValue(true);
          final subscription = characteristic.lastValueStream.listen((value) {
            if (value.isNotEmpty) {
              String confirmation = utf8.decode(value);
              log("Received confirmation: $confirmation");
              if (confirmation == "CONNECTED" ||
                  confirmation == "DISCONNECTED") {
                debugPrint("Confirmation received: $confirmation");

                // Check if the widget is still in the widget tree
                if (mounted) {
                  setState(() {
                    _isWifiConnected = confirmation == "CONNECTED";
                  });

                  if (_isWifiConnected) {
                    Snackbar.show(ABC.c, "Connected to Wi-Fi", success: true);
                    // Navigate only if the widget is still mounted
                    Navigator.of(context).popUntil((route) => route.isFirst);
                  } else {
                    Snackbar.show(
                        ABC.c, "Disconnected from Wi-Fi", success: true);
                  }
                }
              } else {
                debugPrint("Unknown confirmation received: $confirmation");
              }
            }
          });

          widget.device.cancelWhenDisconnected(subscription);

          break;
        }
      }
    } catch (e) {
      debugPrint("Error listening for confirmation: $e");
    }
  }

   */

  Future<void> _getWifiAPList() async {
    if (mounted) {
      setState(() {
        _isScanningAPs = true;
      });
    }
    try {
      for (var characteristic in _characteristics) {
        if (characteristic.uuid.toString().toUpperCase() == "5040") {
          await characteristic.setNotifyValue(true);
          final subscription = characteristic.lastValueStream.listen((value) {
            String ssidData = utf8.decode(value);
            List<String> ssids = ssidData.split('\n')
                .where((s) => s.isNotEmpty)
                .toList();
            if (mounted) { // Check if the widget is still mounted
              setState(() {
                wifiSSIDs.clear();
                wifiSSIDs.addAll(ssids);
              });
            }
          });

          widget.device.cancelWhenDisconnected(subscription);
          await characteristic.read();
        }
      }
    } catch (e) {
      debugPrint("Error getting WiFi AP list: $e");
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

      //if (!_isWifiConnected) _getWifiAPList();
    } catch (e) {
      debugPrint("Error discovering services: $e");
    } finally {
      setState(() => _isDiscoveringServices = false);
    }
  }

  Future<void> _onRefresh() async {
    if (_isConnected && !_isWifiConnected) {
      setState(() {
        //wifiSSIDs.clear();
        _isScanningAPs = true; // Show loading indicator while refreshing
      });
      await _getWifiAPList();
      setState(() {
        _isScanningAPs = false; // Hide loading indicator after refreshing
      });
    }
  }


  @override
  void dispose() {
    _connectionStateSubscription.cancel();
    super.dispose();
  }

  Widget _buildConnectionStatusBar() {
    return Container(
      padding: EdgeInsets.all(8.0),
      color: Colors.black26,
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: <Widget>[
          Row(
            children: [
              Icon(_isWifiConnected ? Icons.wifi : Icons.wifi_off,
                  color: _isWifiConnected ? Colors.green : Colors.red),
              SizedBox(width: 8),
              Text(
                _isWifiConnected
                    ? "Connected to Wi-Fi"
                    : "Not connected to Wi-Fi",
                style: TextStyle(color: Colors.white),
              ),
            ],
          ),
          if (_isScanningAPs)
            Text("Getting WiFi Status...",
                style: TextStyle(color: Colors.white)),
          _isWifiConnected ? _buildDisconnectWifiButton() : SizedBox(),
        ],
      ),
    );
  }

  Widget _buildDisconnectWifiButton() {
    return TextButton(
      onPressed: _disconnectWifi,
      child: Text("Disconnect", style: TextStyle(color: Colors.white)),
    );
  }

  // Function to get the WiFi connection status
  Future<int> _getWifiConnectionStatus() async {
    try {
      for (var characteristic in _characteristics) {
        if (characteristic.uuid.toString().toUpperCase() == "5210") {
          // Check if characteristic is readable
          if (characteristic.properties.read) {
            await characteristic.write(
                utf8.encode("GET_WIFI_STATUS"), withoutResponse: true);
            debugPrint("Sent GET_WIFI_STATUS command");
            return 1;
          } else {
            debugPrint("Characteristic 5040 not found");
            return 0;
          }
        }
      }
    } catch (e) {
      debugPrint("Error discovering services: $e");
    } finally {
      if (mounted) {
        setState(() {
          // Update state here if necessary
          //_isScanningAPs = false;
        });
      }
    }

    return 1;
  }

  Future<void> _disconnectWifi() async {
    try {
      log("Tu smo");
      for (var characteristic in _characteristics) {
        if (characteristic.uuid.toString().toUpperCase() == "5110") {
          // Clear previous SSIDs
          await characteristic.write(utf8.encode("DISCONNECT_WIFI"),
              withoutResponse: characteristic.properties.writeWithoutResponse);
          debugPrint("Sent DISCONNECT_WIFI command");
          break;
        }
      }
    } catch (e) {
      debugPrint("Error discovering services: $e");
    } finally {
      setState(() => _isWifiConnected = false
      );
      _getWifiAPList(); // Fetch the list again after disconnecting
    }
  }

  Widget _buildLoadingIndicator() {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          CircularProgressIndicator(),
          SizedBox(height: 10),
          Text(
            "Retrieving WiFi AP List",
            style: TextStyle(color: Colors.grey),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.device.platformName),
        backgroundColor: Colors.grey[900],
        // ... [app bar actions]
      ),
      body: _isWifiConnected ? _buildNonRefreshableList() : _buildRefreshableList(),
    );
  }

  Widget _buildRefreshableList() {
    return Column(
      children: [
        _buildConnectionStatusBar(),
        Expanded(
          child: RefreshIndicator(
            onRefresh: _onRefresh,
            child: _buildWifiListView(),
          ),
        ),
      ],
    );
  }

  Widget _buildNonRefreshableList() {
    return Column(
      children: [
        _buildConnectionStatusBar(),
        Expanded(
          child: _isScanningAPs
              ? _buildLoadingIndicator()
              : _buildWifiListView(disableTouch: true),
        ),
      ],
    );
  }

// Rest of your code...


  Widget _buildWifiListView({bool disableTouch = false}) {
    if (wifiSSIDs.isEmpty && !_isDiscoveringServices && !_isScanningAPs) {
      return ListView(
        physics: AlwaysScrollableScrollPhysics(),
        children: [
          ListTile(
            title: Center(
              child: Text(
                'Slide down to get list of available APs',
                style: TextStyle(color: Colors.grey),
              ),
            ),
          ),
        ],
      );
    } else {
      return ListView.builder(
        itemCount: wifiSSIDs.length,
        itemBuilder: (context, index) {
          return Opacity(
            opacity: disableTouch ? 0.5 : 1.0,
            child: Card(
              margin: EdgeInsets.all(8.0),
              child: ListTile(
                leading: Icon(Icons.wifi),
                title: Text(wifiSSIDs[index]),
                onTap: disableTouch ? null : () => _promptForPassword(wifiSSIDs[index]),
              ),
            ),
          );
        },
      );
    }
  }
}
