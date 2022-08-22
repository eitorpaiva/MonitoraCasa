
import 'package:flutter/material.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'server.dart' if (dart.library.html) 'browser.dart' as mqttsetup;

void main() => runApp(const MyApp());

class MyApp extends StatelessWidget {
  const MyApp({Key? key}) : super(key: key);
  static const String _title = 'TSI - IoT 2022 (Casa do Eitor)';

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(
      title: _title,
      home: MQTTCliente(),
    );
  }
}

class MQTTCliente extends StatefulWidget {
  const MQTTCliente({Key? key}) : super(key: key);

  @override
  _MQTTClienteState createState() => _MQTTClienteState();

}

class _MQTTClienteState extends State<MQTTCliente>{
  bool estaLigado = false;
  var topicoTemperatura = "eitor/casa/sala/temperatura";
  var topicoUmidade = "eitor/casa/sala/umidade";
  var topicoLampada = "eitor/casa/sala/lampada";
  var lampada = "desli";
  String temp = "--";
  String umi = "--";

  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return FutureBuilder(
        future: mqttsetup.connect(),
        builder:(context, snapshot){
          if(snapshot.hasData){
            MqttClient client = snapshot.data as MqttClient;
            client.subscribe(topicoTemperatura, MqttQos.atMostOnce);
            client.subscribe(topicoUmidade, MqttQos.atMostOnce);
            client.updates!.listen((List<MqttReceivedMessage<MqttMessage?>>? c) {
              final recMess = c![0].payload as MqttPublishMessage;
              final pt =
              MqttPublishPayload.bytesToStringAsString(recMess.payload.message);

              if(c[0].topic == topicoTemperatura) {
                setState(() {
                  temp = pt.toString();
                });
              }
              if(c[0].topic == topicoUmidade) {
                setState(() {
                  umi = pt.toString();
                });
              }
            });

            return MaterialApp(
              home: DefaultTabController(
                length: 3,
                child: Scaffold(
                  appBar: AppBar(
                    bottom: const TabBar(
                      tabs: [
                        Tab(icon: Icon(Icons.thermostat)),
                        Tab(icon: Icon(Icons.invert_colors)),
                        Tab(icon: Icon(Icons.lightbulb_sharp)),
                      ],
                    ),
                    title: const Text("TSI - IoT 2022 (Casa do Eitor)"),
                  ),
                  body: TabBarView(
                    children: [
                      _buildTexto(temp+"ºC", "Temperatura"),
                      _buildTexto(umi+"%", "Umidade"),

                      Container(
                        width: double.infinity,
                        padding: EdgeInsets.all(20),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.center,
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Switch(
                              value: estaLigado,
                              onChanged: (value) {
                                setState(() {
                                  estaLigado = value;
                                  final builder = MqttClientPayloadBuilder();
                                  if(estaLigado == true)
                                    lampada = "ligar";
                                  else
                                    lampada = "desli";
                                  builder.addString(lampada);
                                  print(lampada);
                                  client.publishMessage(topicoLampada, MqttQos.atLeastOnce, builder.payload!);
                                });
                              },
                            ),
                            Text(
                              "Lâmpada",
                              style: TextStyle(
                                color: Colors.black,
                                fontSize: 25,
                                fontWeight: FontWeight.normal,
                              ),
                            ),
                          ],
                        ),
                      )
                    ],
                  ),
                ),
              ),
            );
          }else{
            return Center( child: CircularProgressIndicator());
          }
        });
  }

  Widget _buildTexto(String titulo, String subtitulo){
    return Container(
      width: double.infinity,
      padding: EdgeInsets.all(20),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.center,
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Text(
            titulo,
            style: TextStyle(
              color: Colors.black,
              fontSize: 65,
              fontWeight: FontWeight.normal,
            ),
          ),
          Text(
            subtitulo,
            style: TextStyle(
              color: Colors.black,
              fontSize: 25,
              fontWeight: FontWeight.normal,
            ),
          ),
        ],
      ),
    );
  }
}
