import 'dart:async';
import 'dart:convert';

import 'package:flutter/material.dart';
import 'dart:developer';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../widgets/service_tile.dart';
import '../widgets/characteristic_tile.dart';
import '../widgets/descriptor_tile.dart';
import '../utils/snackbar.dart';
import '../utils/extra.dart';

class DeviceScreen extends StatefulWidget {
  final BluetoothDevice device;

  const DeviceScreen({Key? key, required this.device}) : super(key: key);

  @override
  State<DeviceScreen> createState() => _DeviceScreenState();
}

class _DeviceScreenState extends State<DeviceScreen> {
  int? _rssi;
  int? _mtuSize;
  BluetoothConnectionState _connectionState =
      BluetoothConnectionState.disconnected;
  List<BluetoothService> _services = [];
  List<BluetoothCharacteristic> _characteristics = [];
  bool _isDiscoveringServices = false;
  bool _isConnecting = false;
  bool _isDisconnecting = false;

  List<String> wifiSSIDs = []; // List to store Wi-Fi SSIDs

  late StreamSubscription<BluetoothConnectionState>
  _connectionStateSubscription;
  late StreamSubscription<bool> _isConnectingSubscription;
  late StreamSubscription<bool> _isDisconnectingSubscription;
  late StreamSubscription<int> _mtuSubscription;

  final _ssidController = TextEditingController();
  final _passwordController = TextEditingController();

  @override
  void initState()  {
    super.initState();

    _connectionStateSubscription =
        widget.device.connectionState.listen((state) async {
          _connectionState = state;
          if (state == BluetoothConnectionState.connected) {
            _services = []; // must rediscover services
            await onDiscoverServicesPressed();
            log("Ivan: Do ovde smo dosli");
            await readWifiSSIDCharacteristic(); // Call the method to read Wi-Fi SSID characteristic
            log("Ivan: ajde");
          }
          if (state == BluetoothConnectionState.connected && _rssi == null) {
            _rssi = await widget.device.readRssi();
          }
          if (mounted) {
            setState(() {});
          }
        });

    _mtuSubscription = widget.device.mtu.listen((value) {
      _mtuSize = value;
      if (mounted) {
        setState(() {});
      }
    });

    _isConnectingSubscription = widget.device.isConnecting.listen((value) {
      _isConnecting = value;
      if (mounted) {
        setState(() {});
      }
    });

    _isDisconnectingSubscription =
        widget.device.isDisconnecting.listen((value) {
          _isDisconnecting = value;
          if (mounted) {
            setState(() {});
          }
        });


  }

  @override
  void dispose() {
    _connectionStateSubscription.cancel();
    _mtuSubscription.cancel();
    _isConnectingSubscription.cancel();
    _isDisconnectingSubscription.cancel();
    super.dispose();
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
  }

  bool get isConnected {
    return _connectionState == BluetoothConnectionState.connected;
  }

  Future<void> readWifiSSIDCharacteristic() async {
    if (_services.isNotEmpty) {
      for (var service in _services) {
        for (var characteristic in service.characteristics) {
          if (characteristic.uuid.toString().toUpperCase() == "5040") {
            try {
              List<int> value = await characteristic.read();
              var tmp = value[0];
              log("TU smo $tmp");
              value.map((int val) {
                log("vrijednost => $val");
              });

              // Assuming each byte in 'value' is a unique SSID identifier
              setState(() {
                wifiSSIDs = value.map((e) => "SSID_$e").toList(); // Placeholder conversion
              });
            } catch (e) {
              log("Error reading SSID characteristic: $e");
              Snackbar.show(ABC.c, "Error reading SSID characteristic: $e", success: false);
            }
            return; // Exit after reading the characteristic
          }
        }
      }
      Snackbar.show(ABC.c, "SSID characteristic not found", success: false);
    } else {
      Snackbar.show(ABC.c, "No services discovered", success: false);
    }
  }

  // Add a method to handle SSID selection
  void onSSIDSelected(String ssid) {
    log("ale");
    TextEditingController passwordController = TextEditingController();
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text("Enter Password for $ssid"),
          content: TextField(
            controller: passwordController,
            decoration: InputDecoration(labelText: 'Password'),
            obscureText: true,
          ),
          actions: <Widget>[
            ElevatedButton(
              child: Text('Connect'),
              onPressed: () {
                Navigator.of(context).pop(); // Close the dialog
                _sendCredentials(ssid, passwordController.text);
              },
            ),
          ],
        );
      },
    );
  }


  Future onConnectPressed() async {
    try {
      await widget.device.connectAndUpdateStream();
      Snackbar.show(ABC.c, "Connect: Success", success: true);
    } catch (e) {
      if (e is FlutterBluePlusException &&
          e.code == FbpErrorCode.connectionCanceled.index) {
        // ignore connections canceled by the user
      } else {
        Snackbar.show(ABC.c, prettyException("Connect Error:", e),
            success: false);
      }
    }
  }

  Future onCancelPressed() async {
    try {
      await widget.device.disconnectAndUpdateStream(queue: false);
      Snackbar.show(ABC.c, "Cancel: Success", success: true);
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Cancel Error:", e), success: false);
    }
  }

  Future onDisconnectPressed() async {
    try {
      await widget.device.disconnectAndUpdateStream();
      Snackbar.show(ABC.c, "Disconnect: Success", success: true);
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Disconnect Error:", e),
          success: false);
    }
  }

  Future onDiscoverServicesPressed() async {
    if (mounted) {
      setState(() {
        _isDiscoveringServices = true;
      });
    }
    try {
      log("TUUUU SMOOOOO");
      _services = await widget.device.discoverServices();
      // Define the UUID you are interested in
      String targetUuid = "2b36";

      // Filter services to only include the ones with the target UUID
      List<BluetoothService> filteredServices = _services
          .where((service) => service.uuid.toString().toLowerCase().contains(targetUuid))
          .toList();

      // Initialize an empty list to hold all characteristics from the filtered services
      List<BluetoothCharacteristic> allCharacteristics = [];

      // Iterate through all filtered services to extract characteristics
      for (BluetoothService service in filteredServices) {
        // Add all characteristics from the current service to the list
        allCharacteristics.addAll(service.characteristics);
      }

      // Update the _characteristics variable with the extracted characteristics
      setState(() {
        _services = filteredServices;
        _characteristics = allCharacteristics;
      });

      log("Services: $_services");
      log("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAservices: $widget");
      //await readWifiSSIDCharacteristic();
      Snackbar.show(ABC.c, "Discover Services: Success", success: true);
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Discover Services Error:", e),
          success: false);
    }
    if (mounted) {
      setState(() {
        _isDiscoveringServices = false;
      });
    }
  }

  Future onRequestMtuPressed() async {
    try {
      await widget.device.requestMtu(223, predelay: 0);
      Snackbar.show(ABC.c, "Request Mtu: Success", success: true);
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Change Mtu Error:", e),
          success: false);
    }
  }

  List<Widget> _buildServiceTiles(BuildContext context, BluetoothDevice d) {
    return _services
        .map(
          (s) => ServiceTile(
        service: s,
        characteristicTiles: s.characteristics
            .map((c) => _buildCharacteristicTile(c))
            .toList(),
      ),
    )
        .toList();
  }

  CharacteristicTile _buildCharacteristicTile(BluetoothCharacteristic c) {
    return CharacteristicTile(
      characteristic: c,
      descriptorTiles:
      c.descriptors.map((d) => DescriptorTile(descriptor: d)).toList(),
    );
  }

  Widget buildSpinner(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(14.0),
      child: AspectRatio(
        aspectRatio: 1.0,
        child: CircularProgressIndicator(
          backgroundColor: Colors.black12,
          color: Colors.black26,
        ),
      ),
    );
  }

  Widget buildRemoteId(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(8.0),
      child: Text('${widget.device.remoteId}'),
    );
  }

  Widget buildRssiTile(BuildContext context) {
    return Column(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        isConnected
            ? const Icon(Icons.bluetooth_connected)
            : const Icon(Icons.bluetooth_disabled),
        Text(((isConnected && _rssi != null) ? '${_rssi!} dBm' : ''),
            style: Theme.of(context).textTheme.bodySmall)
      ],
    );
  }

  Widget buildGetServices(BuildContext context) {
    return IndexedStack(
      index: (_isDiscoveringServices) ? 1 : 0,
      children: <Widget>[
        TextButton(
          child: const Text("Get Services"),
          onPressed: onDiscoverServicesPressed,
        ),
        const IconButton(
          icon: SizedBox(
            child: CircularProgressIndicator(
              valueColor: AlwaysStoppedAnimation(Colors.grey),
            ),
            width: 18.0,
            height: 18.0,
          ),
          onPressed: null,
        )
      ],
    );
  }

  Widget buildMtuTile(BuildContext context) {
    return ListTile(
        title: const Text('MTU Size'),
        subtitle: Text('$_mtuSize bytes'),
        trailing: IconButton(
          icon: const Icon(Icons.edit),
          onPressed: onRequestMtuPressed,
        ));
  }

  Widget buildConnectButton(BuildContext context) {
    return Row(children: [
      if (_isConnecting || _isDisconnecting) buildSpinner(context),
      TextButton(
          onPressed: _isConnecting
              ? onCancelPressed
              : (isConnected ? onDisconnectPressed : onConnectPressed),
          child: Text(
            _isConnecting ? "CANCEL" : (isConnected ? "DISCONNECT" : "CONNECT"),
            style: Theme.of(context)
                .primaryTextTheme
                .labelLarge
                ?.copyWith(color: Colors.white),
          ))
    ]);
  }

  Widget buildWifiCredentials(BuildContext context) {
    return Column(
      children: <Widget>[
        TextFormField(
          controller: _ssidController,
          decoration: InputDecoration(labelText: 'WiFi SSID'),
        ),
        TextFormField(
          controller: _passwordController,
          decoration: InputDecoration(labelText: 'Password'),
          obscureText: true, // Use this to obscure password input
        ),
        ElevatedButton(
          onPressed: () {
            if (_ssidController.text.isNotEmpty && _passwordController.text.isNotEmpty) {
              //_sendCredentials();
            }
          },
          child: Text('Send'),
        ),
      ],
    );
  }

  @override
  Widget build(BuildContext context) {
    return ScaffoldMessenger(
      key: Snackbar.snackBarKeyC,
      child: Scaffold(
        appBar: AppBar(
          title: Text(widget.device.platformName),
          actions: [buildConnectButton(context)],
        ),
        body: SingleChildScrollView(
          child: Column(
            children: <Widget>[
              //buildRemoteId(context),
              // ListTile(
              //   leading: buildRssiTile(context),
              //   title: Text(
              //       'Device is ${_connectionState.toString().split('.')[1]}.'),
              //   trailing: buildGetServices(context),
              // ),
              // buildMtuTile(context),
              //buildGetServices(context),
              //..._buildServiceTiles(context, widget.device),
              buildWifiCredentials(context),
            ],
          ),
        ),
      ),
    );
  }
}