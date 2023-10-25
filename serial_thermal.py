import os
import serial
from datetime import datetime

filename = 'serial_thermal_data_v1_20231025_1630.txt'
filepath = os.path.join(os.getcwd(), filename)

if __name__ == '__main__':
    file = open(filepath, "a")
    ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)
    ser.reset_input_buffer()

    try:
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').rstrip()
                line = line.replace('CLEARDATA,', '')
                line = line.replace('CLEARSHEET,', '')
                line = line.replace('RESETTIMER,', '')
                line = line.replace('LABEL, ', '')
                line = line.replace('Date, Time', 'DateTime')
                line = line.replace('DATA,', '')
                line = line.replace('TIME,', '')
                line = line.replace('DATE', str(datetime.now()))
                line = line + '\n'
                file.write(line)
                print(line)
    except KeyboardInterrupt:
        file.close()


