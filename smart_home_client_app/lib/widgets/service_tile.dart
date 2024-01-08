import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import "characteristic_tile.dart";

// Map for known services
Map<String, String> knownServices = {
  "1800": "Generic Access",
  "1801": "Generic Attribute",
  "180A": "Device Information",
  "2B36": "Wifi Credentials", // Added as per your request
  // Add more known services here
};

class ServiceTile extends StatelessWidget {
  final BluetoothService service;
  final List<CharacteristicTile> characteristicTiles;

  const ServiceTile(
      {Key? key, required this.service, required this.characteristicTiles})
      : super(key: key);

  String getServiceName(BluetoothService service) {
    var uuid = service.uuid.toString().toUpperCase();
    return knownServices[uuid] ?? "Unknown Service ($uuid)";
  }

  Widget buildUuid(BuildContext context) {
    String uuid = service.uuid.toString().toUpperCase();
    String name = getServiceName(service);
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text('Ivanovic UUID: $uuid', style: TextStyle(fontSize: 13)),
        Text('Name: $name', style: TextStyle(fontSize: 13, color: Colors.teal)),
      ],
    );
  }

  @override
  Widget build(BuildContext context) {
    return characteristicTiles.isNotEmpty
        ? ExpansionTile(
            title: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: <Widget>[
                const Text('Service', style: TextStyle(color: Colors.blue)),
                buildUuid(context),
              ],
            ),
            children: characteristicTiles,
          )
        : ListTile(
            title: const Text('Service'),
            subtitle: buildUuid(context),
          );
  }
}
