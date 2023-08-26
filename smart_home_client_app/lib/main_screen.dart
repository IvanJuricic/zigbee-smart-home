import 'dart:io';
import 'dart:convert'; // For encoding/decoding messages
import 'package:flutter/material.dart';

class ConnectionStatusObserver with WidgetsBindingObserver {
  final _screenState;

  ConnectionStatusObserver(this._screenState);

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.resumed) {
      // Attempt to reconnect when the app is resumed.
      _screenState._connectToServer();
    }
  }
}

class MainScreen extends StatefulWidget {
  @override
  _MainScreenState createState() => _MainScreenState();
}

class _MainScreenState extends State<MainScreen> {
  bool lightsOn = false;
  late Socket socket;
  String connectionStatus = 'Connecting...';
  late ConnectionStatusObserver _connectionStatusObserver;

  @override
  void initState() {
    super.initState();

    _connectionStatusObserver = ConnectionStatusObserver(this);
    WidgetsBinding.instance!.addObserver(_connectionStatusObserver);

    // Attempt to establish the socket connection.
    _connectToServer();
  }

  // Function to establish the socket connection.
  Future<void> _connectToServer() async {
    try {
      // Initialize the socket connection.
      socket = await Socket.connect(SERVER_IP, 8000);

      // Send the "Auth" message upon connecting to authorize the client.
      socket.write(AUTH_TOKEN);

      // Update the connection status.
      setState(() {
        connectionStatus = 'Connected';
      });

      // Listen for messages from the server.
      socket.listen((List<int> data) {
        final serverResponse = utf8.decode(data);

        // Handle messages from the server here.
        print('Received message from server: $serverResponse');

        // Check if the server's response is "ON" or "OFF" and update the button color accordingly.
        setState(() {
          lightsOn = serverResponse == 'ON';
        });
      });
    } catch (error) {
      print('Error connecting to the server: $error');
      setState(() {
        connectionStatus = 'Disconnected';
      });
    }
  }

  void toggleLights() {
    // Check if the connection is stable before allowing button presses.
    if (connectionStatus == 'Connected') {
      // Toggle the lights and send a "Toggle!" message to the server.
      final message = 'Toggle!\n';
      socket.write(message);

      setState(() {
        // Update the UI immediately for local feedback.
        lightsOn = !lightsOn;
      });
    }
  }

  @override
  void dispose() {
    // Close the socket connection and remove the app lifecycle observer when the widget is disposed.
    socket?.destroy();
    WidgetsBinding.instance!.removeObserver(_connectionStatusObserver);
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text("IoT Smart Home"),
        bottom: PreferredSize(
          preferredSize: Size.fromHeight(20.0),
          child: Center(
            child: Text(
              'Connection Status: $connectionStatus',
              style: TextStyle(fontSize: 16.0, color: Colors.white),
            ),
          ),
        ),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            GestureDetector(
              onTap: toggleLights,
              child: AnimatedContainer(
                duration: Duration(milliseconds: 300),
                width: 200.0,
                height: 60.0,
                decoration: BoxDecoration(
                  color: lightsOn ? Colors.green : Colors.red,
                  borderRadius: BorderRadius.circular(30.0),
                ),
                child: Center(
                  child: Text(
                    lightsOn ? "Lights On" : "Lights Off",
                    style: TextStyle(color: Colors.white),
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
