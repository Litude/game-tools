'''
Created on 2.11.2018

@author: tommi
'''

from struct import unpack

UNITS = [
    "Serf",
    "Woodcutter",
    "Miner",
    "Animal Breeder",
    "Farmer",
    "Carpenter",
    "Baker",
    "Butcher",
    "Fisherman",
    "Laborer",
    "Stonemason",
    "Blacksmith",
    "Metallurgist",
    "Recruit",
    "Militia",
    "Axe Fighter",
    "Sword Fighter",
    "Bowman",
    "Crossbowman",
    "Lance Carrier",
    "Pikeman",
    "Scout",
    "Knight",
    "Barbarian",
    "Rebel",
    "Rogue",
    "Warrior",
    "Vagabond",
    "Catapult",
    "Ballista",
    "Wolf",
    "Fish",
    "Sea Snake",
    "Starfish",
    "Crab",
    "Water Lily",
    "Lily Pad",
    "Duck",
    "Unused 1",
    "Unused 2",
    "Unused 3"  
    ]

def main():
    
    with open("unit.dat", "rb") as file:
        skip_animations(file)
        
        for i in range(0, 41):
            unit_data = read_unit_data(file)
            (health, attack, cavBonus, unknown, defense, speed, chargeSpeed, los, habitat, attFrame, split) = unpack("=HHHHHHHHBBI", unit_data)
            
            print(
                f"Unit {UNITS[i]} ({i})\n" +
                f"Health: {health}\n" +
                f"Attack: {attack}\n" +
                f"Cavalry Bonus: {cavBonus}\n" +
                f"Unknown: {unknown}\n" +
                f"Defense: {defense}\n" +
                f"Speed: {speed}\n" +
                f"Charge speed+: {chargeSpeed}\n" +
                f"Line of Sight: {los}\n" +
                f"Habitat: {habitat}\n" +
                f"Attack frame: {attFrame}\n" +
                f"Split graphics: {split}\n"
                )
            
            goto_next_unit(file)
        
        
def skip_animations(file_handle):
    file_handle.seek(15680)

def goto_build_area(file_handle):
    file_handle.seek(1428, 1)
    
def goto_next_unit(file_handle):
    file_handle.seek(7876, 1)
    
def read_unit_data(file_handle):
    return file_handle.read(22)

    
main()
