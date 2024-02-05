import 'dart:async';
import 'dart:io';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

class MQTTManager {
  final String broker = 'ivanproxy.ddns.net'; // Replace with your broker's address
  final String clientIdentifier = '';
  MqttServerClient? _client;

  Future<void> initializeMQTTClient() async {
    _client = MqttServerClient(broker, clientIdentifier);
    _client!.logging(on: true);
    _client!.setProtocolV311();
    _client!.keepAlivePeriod = 20;
    _client!.onDisconnected = _onDisconnected;
    _client!.onConnected = _onConnected;
    _client!.onSubscribed = _onSubscribed;

    try {
      await _client!.connect();
    } on NoConnectionException catch (e) {
      print('MQTT client exception - $e');
      _client!.disconnect();
    } on SocketException catch (e) {
      print('MQTT socket exception - $e');
      _client!.disconnect();
    }

    if (_client!.connectionStatus!.state == MqttConnectionState.connected) {
      print('MQTT client connected');
    } else {
      print('MQTT client connection failed - disconnecting');
      _client!.disconnect();
      //exit(-1);
    }
  }

  void disposeMQTTClient() {
    _client?.disconnect();
  }

  void subscribeToTopic(String topic) {
    _client!.subscribe(topic, MqttQos.atMostOnce);
  }

  void publishMessage(String topic, String message) {
    final builder = MqttClientPayloadBuilder();
    builder.addString(message);
    _client!.publishMessage(topic, MqttQos.exactlyOnce, builder.payload!);
  }

  void _onSubscribed(String topic) {
    print('Subscription confirmed for topic $topic');
  }

  void _onDisconnected() {
    print('MQTT client disconnected');
  }

  void _onConnected() {
    print('MQTT client connected');
  }

}