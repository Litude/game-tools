'''
Created on 3.11.2018

@author: tommi
'''

def main():
    
    with open("pattern.dat", "rb") as file:
        
        for i in range(0, 256):
            element_data = file.read(99)
            print("""Element %d:
field_0: %02x
field_1: %02x
field_2: %02x
field_3: %02x
field_4: %02x
field_5: %02x
""" % (i, element_data[0], element_data[1], element_data[2],
                   element_data[3], element_data[4], element_data[5]))

main()