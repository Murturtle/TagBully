# TagBully
This script allows you to detect nearby airtags using an ESP32C3 dev board and then play a sound. Optional screen capabilities with the 0.42in screen that comes with some small ESPs. 

This project is inspired by [AirGuard](https://github.com/seemoo-lab/AirGuard) and the ble uuids are taken from this repo.

## Process
The esp first scans for 10 seconds (when an airtag is disconnected from its owner's device for 15 minutes it sends out a ble beacon every 2 seconds). The esp then goes through each bluetooth device, checking if the manufacturer is Apple. It then checks for the bytes 0x12 and 0x19 to check if it is a find my device and if offline finding is enabled (offline finding is enabled after 15 minutes of owner's device being disconnected). It then connects to the device and checks for the sound service and characteristic. If it doesn't find it, its not an airtag and countinues. If it is an Airtag it sends the play sound code. It then disconnects and continues through the other bluetooth devices.
