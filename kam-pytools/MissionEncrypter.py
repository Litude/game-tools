
import os
inputdir = 'input'
outputdir = 'output'
EXTENSIONS = ('.dat')


def main():

    for subdir, _, files in os.walk(inputdir):
        for file in files:
            if file.endswith(EXTENSIONS):
                with open(os.path.join(subdir, file), "rb") as stream:
                    contents = stream.read()
                    (_, filename) = os.path.split(file)
                    encrypted = bytearray([x^0xEF for x in contents])
                    
                    with open(os.path.join(outputdir, filename), "wb") as output:
                        print("Writing " + filename)
                        output.write(encrypted)
                        
main()
