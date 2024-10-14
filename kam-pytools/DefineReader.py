'''
Created on 2.11.2018

@author: tommi
'''

from struct import unpack

BUILDINGS = [
    "Sawmill",
    "Iron Smithy",
    "Weapon Smithy",
    "Coal Mine",
    "Iron Mine",
    "Gold Mine",
    "Fisherman's",
    "Bakery",
    "Farm",
    "Woodcutter's",
    "Armor Smithy",
    "Storehouse",
    "Stables",
    "Schoolhouse",
    "Quarry",
    "Metallurgist's",
    "Swine Farm",
    "Watch Tower",
    "Town Hall",
    "Weapons Workshop",
    "Armory Workshop",
    "Barracks",
    "Mill",
    "Siege Workshop",
    "Butcher's",
    "Tannery",
    "Training Hall",
    "Inn",
    "Vineyard"
    ]

def main():
    
    with open("houses.dat", "rb") as file:
        skip_animations(file)
        #goto_build_area(file)
        
        for i in range(0, 29):
            goto_construction_steps(file)
            print(f"{BUILDINGS[i]} ({i})" )
            build_data = read_prearea_data(file)
            (stepsWood, stepsStone, hitsPerStep, entranceX, entranceY, eXOffset, eYOffset) = unpack("=HHHbbbb", build_data)
            build_data = read_build_area(file)
            build_area = [build_data[i:i+10] for i in range(0, len(build_data), 10)]
            
            skip_buildRes(file)
            build_data = read_postarea_data(file)
            (buildSpotAmount, sizeArea, sizeX, sizeY, originX, originY, workTime, restTime) = unpack("=HHBBBBHH", build_data)
            
            print(
            f"Steps: Wood x{stepsWood}, Stone x{stepsStone}\n" +
            f"Hits per step: {hitsPerStep}\n" +
            f"Entrance: {entranceX}x, {entranceY}y\n" +
            f"Entrance offset: {eXOffset}x, {eYOffset}y\n" +
            f"Build spot amount: {buildSpotAmount}\n" +
            f"Area size: {sizeArea}\n" +
            f"Dimensions: {sizeX}x{sizeY}\n" +
            f"Origin: {originX}x, {originY}y\n" +
            f"Work time: {workTime}\n" +
            f"Rest time: {restTime}\n"
            )
            
            
            for y in range(0, 10):
                for x in range(0, 10):
                    print("%02x " %(build_area[y][x]), end='')
                print("")
            print("")
            #back_build_area_size(file)
            skip_to_next_building(file)
        
        
        
def skip_animations(file_handle):
    file_handle.seek(2100)
    
def skip_buildRes(file_handle):
    file_handle.seek(98, 1)
    
def goto_construction_steps(file_handle):
    file_handle.seek(1418, 1)

def goto_build_area(file_handle):
    file_handle.seek(1428, 1)
    
def goto_next_building(file_handle):
    #file_handle.seek(1688, 1)
    file_handle.seek(1688, 1)
    
def read_build_area(file_handle):
    return file_handle.read(100)

def read_prearea_data(file_handle):
    return file_handle.read(10)

def read_postarea_data(file_handle):
    return file_handle.read(12)

def back_build_area_size(file_handle):
    file_handle.seek(-100, 1)
    
def skip_to_next_building(file_handle):
    file_handle.seek(50, 1)
    
main()
