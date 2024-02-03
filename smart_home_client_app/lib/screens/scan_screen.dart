import 'dart:async';
import 'dart:developer';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter_application_1/screens/device_screen.dart';
import 'package:flutter_application_1/screens/wifi_list_screen.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../screens/wifi_list_screen.dart';
import '../utils/snackbar.dart';
import '../widgets/system_device_tile.dart';
import '../widgets/scan_result_tile.dart';
import '../utils/extra.dart';

import 'package:provider/provider.dart';
import '../state/esp32_provider.dart';

class ScanScreen extends StatefulWidget {
  const ScanScreen({Key? key}) : super(key: key);

  @override
  State<ScanScreen> createState() => _ScanScreenState();
}

class _ScanScreenState extends State<ScanScreen> {
  List<BluetoothDevice> _systemDevices = [];
  List<ScanResult> _scanResults = [];
  bool _isScanning = false;
  bool _isDeviceConnected = false; // Changed to bool
  late StreamSubscription<List<ScanResult>> _scanResultsSubscription;
  late StreamSubscription<bool> _isScanningSubscription;

  @override
  void initState() {
    super.initState();

    _scanResultsSubscription = FlutterBluePlus.scanResults.listen((results) {
      _scanResults = results;
      if (mounted) {
        setState(() {});
      }
    }, onError: (e) {
      Snackbar.show(ABC.b, prettyException("Scan Error:", e), success: false);
    });

    _isScanningSubscription = FlutterBluePlus.isScanning.listen((state) {
      _isScanning = state;
      if (mounted) {
        setState(() {});
      }
    });

    final esp32Provider = Provider.of<ESP32Provider>(context, listen: false);
    _isDeviceConnected = esp32Provider.state.isBLEConnected; // Use bool value

  }

  @override
  void dispose() {
    _scanResultsSubscription.cancel();
    _isScanningSubscription.cancel();
    super.dispose();
  }

  void _handleBLEConnection(BuildContext context, bool isConnected, BluetoothDevice? device) {
    log("Pisem da je spojen bt");
    Provider.of<ESP32Provider>(context, listen: false).updateBLEConnectionStatus(isConnected, device);
  }

  Future onScanPressed() async {
    if (_isDeviceConnected) return;  // Do not scan if a device is connected

    try {
      _systemDevices = await FlutterBluePlus.systemDevices;
      int divisor = Platform.isAndroid ? 8 : 1;
      await FlutterBluePlus.startScan(
          timeout: const Duration(seconds: 15),
          continuousUpdates: true,
          continuousDivisor: divisor);
    } catch (e) {
      Snackbar.show(ABC.b, prettyException("Start Scan Error:", e), success: false);
    }
    if (mounted) {
      setState(() {});
    }
  }

  Future onStopPressed() async {
    try {
      FlutterBluePlus.stopScan();
    } catch (e) {
      Snackbar.show(ABC.b, prettyException("Stop Scan Error:", e),
          success: false);
    }
  }

  void onConnectPressed(BluetoothDevice device) {
    device.connectAndUpdateStream().catchError((e) {
      Snackbar.show(ABC.c, prettyException("Connect Error:", e), success: false);
    }).then((_) {
      setState(() => _isDeviceConnected = true); // Update bool value
      _handleBLEConnection(context, true, device);
    });
    MaterialPageRoute route = MaterialPageRoute(
        builder: (context) => WifiListScreen(device: device),
        settings: const RouteSettings(name: '/WifiListScreen'));
    Navigator.of(context).push(route);
  }

  Future onRefresh() async {
    if (_isScanning == false) {
      FlutterBluePlus.startScan(timeout: const Duration(seconds: 15));
    }
    if (mounted) {
      setState(() {});
    }
    return Future.delayed(Duration(milliseconds: 500));
  }

  Widget buildScanButton(BuildContext context) {
    if (_isDeviceConnected) return SizedBox(); // Changed condition to use bool

    if (FlutterBluePlus.isScanningNow) {
      return FloatingActionButton(
        child: const Icon(Icons.stop),
        onPressed: onStopPressed,
        backgroundColor: Colors.red,
      );
    } else {
      return FloatingActionButton(
          child: const Text("SCAN"), onPressed: onScanPressed);
    }
  }

  List<Widget> _buildSystemDeviceTiles(BuildContext context) {
    return _systemDevices
        .map(
          (d) => SystemDeviceTile(
        device: d,
        onOpen: () => Navigator.of(context).push(
          MaterialPageRoute(
            builder: (context) => WifiListScreen(device: d),
            settings: const RouteSettings(name: '/WifiListScreen'),
          ),
        ),
        onConnect: () => onConnectPressed(d),
      ),
    )
        .toList();
  }

  // Update _buildConnectedDeviceTile to reflect the boolean status
  // and show the connected device if it's available from the provider
  List<Widget> _buildConnectedDeviceTile(BuildContext context) {
    final esp32Provider = Provider.of<ESP32Provider>(context);
    if (_isDeviceConnected && esp32Provider.state.device != null) {
      return [
        SystemDeviceTile(
          device: esp32Provider.state.device!,
          onOpen: () => Navigator.of(context).push(
            MaterialPageRoute(
              builder: (context) => WifiListScreen(device: esp32Provider.state.device!),
              settings: const RouteSettings(name: '/WifiListScreen'),
            ),
          ),
          onConnect: () => onConnectPressed(esp32Provider.state.device!),
        )
      ];
    }
    return [];
  }

  List<Widget> _buildScanResultTiles(BuildContext context) {
    return _scanResults
        .map(
          (r) => ScanResultTile(
        result: r,
        onTap: () => onConnectPressed(r.device),
      ),
    )
        .toList();
  }

  @override
  Widget build(BuildContext context) {
    return ScaffoldMessenger(
      key: Snackbar.snackBarKeyB,
      child: Scaffold(
        appBar: AppBar(
          title: const Text('Find Devices'),
          backgroundColor: Colors.grey[900],
        ),
        body: RefreshIndicator(
          onRefresh: onRefresh,
          child: ListView(
            children: <Widget>[
              ..._buildConnectedDeviceTile(context),
              ..._buildSystemDeviceTiles(context),
              ..._buildScanResultTiles(context),
            ],
          ),
        ),
        floatingActionButton: buildScanButton(context),
      ),
    );
  }
}