// SplashScreen (lib/screens/splash_screen.dart)

import 'package:flutter/material.dart';
import 'home_screen.dart';

class SplashScreen extends StatefulWidget {
  @override
  _SplashScreenState createState() => _SplashScreenState();
}

class _SplashScreenState extends State<SplashScreen> {
  @override
  void initState() {
    super.initState();
    navigateToHome();
  }

  navigateToHome() async {
    await Future.delayed(Duration(seconds: 3), () {});
    Navigator.pushReplacement(
        context,
        PageRouteBuilder(
          pageBuilder: (context, animation1, animation2) => HomeScreen(),
          transitionDuration: Duration(seconds: 1),
          transitionsBuilder: (context, animation, secondaryAnimation, child) {
            var begin = 0.0;
            var end = 1.0;
            var tween = Tween(begin: begin, end: end);
            var fadeAnimation = animation.drive(tween);
            return FadeTransition(
              opacity: fadeAnimation,
              child: child,
            );
          },
        ));
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      color: Colors.deepPurpleAccent,
      child: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            CircularProgressIndicator(
              valueColor: AlwaysStoppedAnimation<Color>(Colors.white),
            ),
            SizedBox(height: 20),
            Text(
              'Loading...',
              style: TextStyle(
                fontSize: 24,
                fontWeight: FontWeight.bold,
                color: Colors.white,
              ),
            ),
          ],
        ),
      ),
    );
  }
}
