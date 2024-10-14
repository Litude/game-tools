'''
Created on 26.7.2018

@author: tommi
'''

from struct import pack

class SoundInfo:
    def __init__(self, freq, vol, isRandFreq, isRandVol, randFreq, randVol, unused, index):
        self.frequency = freq
        self.volume = vol
        self.frequencyRandomized = isRandFreq
        self.volumeRandomized = isRandVol
        self.randomFrequencyAmplitude = randFreq
        self.randomSoundAmplitude = randVol
        self.padding = unused
        self.streamIndex = index
        
    def __str__(self):
        return "(%d, %d, %d, %d, %d, %d, %d, %d)" % (self.frequency, self.volume, self.frequencyRandomized, self.volumeRandomized, self.randomFrequencyAmplitude, self.randomSoundAmplitude, self.padding, self.streamIndex)
        
    def fixValues(self):
        if self.streamIndex == 65535:
            return
        
        if self.randomSoundAmplitude > 100:
            if self.randomFrequencyAmplitude == 0:
                self.randomFrequencyAmplitude = self.randomSoundAmplitude
                self.randomSoundAmplitude = 0
            else:
                temp = self.randomFrequencyAmplitude
                self.randomFrequencyAmplitude = self.randomSoundAmplitude
                self.randomSoundAmplitude = temp
                
        if self.randomFrequencyAmplitude < 100 and self.randomFrequencyAmplitude != 0:
            if self.randomSoundAmplitude == 0:
                self.randomSoundAmplitude = self.randomFrequencyAmplitude
                self.randomFrequencyAmplitude = 0
            else:
                temp = self.randomSoundAmplitude
                self.randomSoundAmplitude = self.randomFrequencyAmplitude
                self.randomFrequencyAmplitude = temp
                
        if self.volume + self.randomSoundAmplitude > 100:
            self.randomSoundAmplitude = 100 - self.volume
            
        if self.randomFrequencyAmplitude != 0:
            self.frequencyRandomized = 1
            
        if self.randomSoundAmplitude != 0:
            self.volumeRandomized = 1
            
    def pack(self):
        return pack("IIIIHHIH", self.frequency, self.volume, self.frequencyRandomized, self.volumeRandomized, self.randomFrequencyAmplitude, self.randomSoundAmplitude, self.padding, self.streamIndex)
    