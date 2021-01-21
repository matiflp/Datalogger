# Datalogger

Datalogger made in Arduino with Qt takes data from two sensors, one of temperature and one of distance, since they are with the sensors that we had but can be adapted. The function it performs is to establish communication with the Arduino and then, as it is a master-slave topology, the PC tells the Arduino (which really works as a converter) that it requires it to send data every so often (with the possibility of configuring it) of the sensors you have connected, account also with a led that will turn on every time it is receiving data, so that later the PC is in charge of performing the decoding and the necessary calculations to obtain the data to store them and to be able to perform different operations with them in a graphical interface.

In the manual, you can see how the data logger works in detail and also images of the connection diagram, implemented software, interface made, etc.

### Developed in:
<p>
<img width="30" height="30" src="https://raw.githubusercontent.com/jesu95/datalogger-qt-arduino/master/img/arduino.svg">
<img width="30" height="30" src="https://raw.githubusercontent.com/jesu95/datalogger-qt-arduino/master/img/qt-logo.jpg">
</p>

Authors:

* [Matías López](https://github.com/matiflp/)
* [Jesús López](https://github.com/jesu95/)
