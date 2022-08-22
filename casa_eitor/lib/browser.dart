import 'dart:async';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_browser_client.dart';

Future<MqttClient> setup(String serverAddress, String uniqueID, int port) {
  return connect();
}

Future<MqttBrowserClient> connect() async {
  MqttBrowserClient client =
  MqttBrowserClient('ws://iot-mqtt-ifms-2022.herokuapp.com', '10');
  client.port=80;
  client.logging(on: false);
  client.keepAlivePeriod=20;
  //client.setProtocolV311();
  client.onConnected = onConnected;
  client.onDisconnected = onDisconnected;
  client.onSubscribed = onSubscribed;
  client.onSubscribeFail = onSubscribeFail;
  client.pongCallback = pong;

  await client.connect();

  if (client.connectionStatus!.state == MqttConnectionState.connected) {
    print('EMQX client connected');
    client.updates!.listen((List<MqttReceivedMessage<MqttMessage>>? c) {
      final message = c![0].payload as MqttPublishMessage;
      final payload =
      MqttPublishPayload.bytesToStringAsString(message.payload.message);

      print('Received message:$payload from topic: ${c[0].topic}>');
    });

    client.published!.listen((MqttPublishMessage message) {
      print('published');
      final payload =
      MqttPublishPayload.bytesToStringAsString(message.payload.message);
      print(
          'Published message: $payload to topic: ${message.variableHeader!.topicName}');
    });
  } else {
    print(
        'EMQX client connection failed - disconnecting, status is ${client.connectionStatus}');
    client.disconnect();
  }

  return client;
}

void onConnected() {
  print('Connected');
}

void onDisconnected() {
  print('Disconnected');
}

void onSubscribed(String topic) {
  print('Subscribed topic: $topic');
}

void onSubscribeFail(String topic) {
  print('Failed to subscribe topic: $topic');
}

void pong() {
  print('Ping response client callback invoked');
}
