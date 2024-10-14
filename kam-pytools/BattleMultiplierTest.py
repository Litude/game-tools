'''
Created on 22.9.2018

@author: tommi
'''

DIRECTION_NORTH = 0
DIRECTION_NORTHEAST = 1
DIRECTION_EAST = 2
DIRECTION_SOUTHEAST = 3
DIRECTION_SOUTH = 4
DIRECTION_SOUTHWEST = 5
DIRECTION_WEST = 6
DIRECTION_NORTHWEST = 7

def main():
    for i in range(0, 8):
        for j in range(0, 8):
            print("Attacker: " + str(i) + ", defender: " + str(j) + ", modifier: " + str(modifier_calculator(i, j)))


def modifier_calculator(attacker, target):
    final_modifier = 1
    direction_modifier = abs(target - attacker)
    if direction_modifier > 4:
        direction_modifier = 8 - direction_modifier
    final_modifier *= (5 - direction_modifier)
            
    return final_modifier

main()