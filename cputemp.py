#!/usr/bin/python3

"""Copyright (c) 2019, Douglas Otwell

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

import dbus
import json
import subprocess
from advertisement import Advertisement
from service import Application, Service, Characteristic, Descriptor
from gpiozero import CPUTemperature

GATT_CHRC_IFACE = "org.bluez.GattCharacteristic1"
NOTIFY_TIMEOUT = 5000

class ThermometerAdvertisement(Advertisement):
    def __init__(self, index):
        Advertisement.__init__(self, index, "peripheral")
        self.add_local_name("joeyblesae")
        self.include_tx_power = True

class ThermometerService(Service):
    THERMOMETER_SVC_UUID = "a854f4b4-8af9-4337-880d-745383350edc"

    def __init__(self, index):
        Service.__init__(self, index, self.THERMOMETER_SVC_UUID, True)
        self.add_characteristic(TempCharacteristic(self))


class TempCharacteristic(Characteristic):
    TEMP_CHARACTERISTIC_UUID = "e7a17637-0e41-4e41-a479-a333c824fc98"

    def __init__(self, service):
        self.notifying = False

        Characteristic.__init__(
                self, self.TEMP_CHARACTERISTIC_UUID,
                ["notify", "read"], service)
        self.add_descriptor(TempDescriptor(self))

    def get_temperature(self):
        value=[]
        command = "curl -u 6334446bbdcbd20e4daf92b4:3806e9d6cead4f45803df27eee00994a https://api.sigfox.com/v2/devices/B448DD/messages"
        output = subprocess.check_output(command, shell=True).decode("utf-8")

        idc = "B448DD"
        data = json.loads(output)

        result={'device': {'id': idc},'data': None}

        for entry in data["data"]:
            if entry["device"]["id"] == idc:
                result["data"] = entry["data"]
                break 

        result2=str(result)

        for c in result2:
            value.append(dbus.Byte(c.encode()))

        print(result)
        print(result2)
        return value

    def set_temperature_callback(self):
        if self.notifying:
            value = self.get_temperature()
            self.PropertiesChanged(GATT_CHRC_IFACE, {"Value": value}, [])

        return self.notifying

    def StartNotify(self):
        if self.notifying:
            return

        self.notifying = True

        value = self.get_temperature()
        self.PropertiesChanged(GATT_CHRC_IFACE, {"Value": value}, [])
        self.add_timeout(NOTIFY_TIMEOUT, self.set_temperature_callback)

    def StopNotify(self):
        self.notifying = False

    def ReadValue(self, options):
        value = self.get_temperature()

        return value

class TempDescriptor(Descriptor):
    TEMP_DESCRIPTOR_UUID = "2901"
    TEMP_DESCRIPTOR_VALUE = '{"data":"test"}'

    def __init__(self, characteristic):
        Descriptor.__init__(
                self, self.TEMP_DESCRIPTOR_UUID,
                ["read"],
                characteristic)

    def ReadValue(self, options):
        value = []
        desc = self.TEMP_DESCRIPTOR_VALUE

        for c in desc:
            value.append(dbus.Byte(c.encode()))

        return value

app = Application()
app.add_service(ThermometerService(0))
app.register()

adv = ThermometerAdvertisement(0)
adv.register()

try:
    app.run()
except KeyboardInterrupt:
    app.quit()
