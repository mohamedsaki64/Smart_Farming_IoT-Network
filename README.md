# Smart_Farming_IoT-Network
Designed and implemented a smart farming IoT network that monitors environmental conditions and automates irrigation, while enabling remote monitoring and control. 
The project encompasses local sensor interfacing, microcontroller programming, wireless communication, and dashboard development. 
Experience gained through project: practical experience in sensor networks, MQTT communication, Node-RED dashboard creation, and long-range LoRa communication for Internet 
of Things applications.
 1. Installation, integration and calibration of sensors for the parameters such as humidity, temperature , light intensity, soil moisture and detect rainfall.
 2. Installed electric valves controlled by servo motors for the automated irrigation system.
 3. Used ESP32 microcontroller whihc serves as the central node. Read sensor data and control automated irrigation.
 4. The ESP32 contains OLED Display for the on-site readings immediate feedbacks of parameters.
 5. The sensor reading are obtained from remote farm and main farm.
 6. To obtain sensor data from remote farm, project integrates LoRa wireless communication. LoRa modules enable low-power devices to send data across distances in the order of kilometers.
 7. Implented a user friendly node-red dashboard for the remote monitoring and control.
 8. Communication between the ESP32 and the Node RED server is achieved via the MQTT protocol. This allows sensor data to be published to the cloud and control commands to be sent back to the field device.
