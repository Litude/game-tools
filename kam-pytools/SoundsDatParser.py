'''
Created on 23.7.2018

@author: tommi
'''

from struct import unpack
from SoundInfo import SoundInfo

def main():
    read("sounds.dat")
    #readAndWrite()
    
def readAndWrite():
    with open("sounds.dat", "rb") as file:
        with open("soundsfix.dat", "wb") as output:
            buffer = file.read(774965)
            output.write(buffer)
            for i in range(0, 400):
                buffer = file.read(26)
                values = unpack("IIIIHHIH", buffer)
                entry = SoundInfo(*values)
                print(entry)
                entry.fixValues()
                print(entry)
                output.write(entry.pack())
                
def read(filename):
    with open(filename, "rb") as file:
        file.seek(774965)
        for i in range(0, 400):
            buffer = file.read(26)
            values = unpack("IIIIHHIH", buffer)
            entry = SoundInfo(*values)
            print(entry)
            
main()
