'''
Created on 22.12.2020

@author: tommi
'''

from struct import unpack

anim_frames = set()

def skip_animation(file):
    file.seek(70, 1)
    
    
def read_animation(file):
    frames = []
    for i in range(0, 30):
        (frame, ) = unpack("=H", file.read(2))
        #print(frame)
        frames.append(frame)
        anim_frames.add(frame)
    (frameCount, ) = unpack("=H", file.read(2))
    #print(frameCount)
    frames = frames[0:frameCount]
    (x, y) = unpack("=II", file.read(8))
    return frames, x, y

def main():
    
    with open("mapelem.dat", "rb") as file:
        
        for i in range(0, 254):
            frames, x, y = read_animation(file)
            element_data = file.read(29)
            (choppable, uncrossable, unwalkable, tiled, growable, tree, treeStump, buildable) = unpack("=IIIIIIBI", element_data)
            
            print(
            f"Map Object {i}\n"
            f"Frames: {frames}\n"
            f"Position: {x}x {y}y\n"
            f"Choppable: {choppable}\n" +
            f"Uncrossable: {uncrossable}\n" +
            f"Unwalkable: {unwalkable}\n" +
            f"Tiled: {tiled}\n" +
            f"Growable: {growable}\n" +
            f"Tree: {tree}\n" +
            f"Tree stump: {treeStump}\n" +
            f"Buildable: {buildable}\n"
            )
    for i in range(0, 255):
        if not i in anim_frames:
            print(f"Graphic {i} is unused")

main()