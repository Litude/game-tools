# Age of Empires SHP Tool

Convert Age of Empires .SLP files into .SHP files.

## Usage

``Usage: shptool filename.slp [--bounds widthxheight] [--origin x,y] [--colormap colormap.col]``

Where:<br>
filename.slp - SLP file to be converted<br>
bounds - Optional bounds value written as the bounds of the original FLC file. If omitted, the tool guesses the original bounds. Valid values are e.g. --bounds 800x600, --bounds 1024x768<br>
origin - Origin/anchor of the image in the coordinate system of the original FLC file. If omitted, the tool guesses the original origin. Valid values are e.g. --origin 320x200, --origin 400x300<br>
colormap - Colormap that is applied to each pixel value during the conversion. This is a text file with 256 lines where each line contains a palette index. ``slp2shp.col`` is an example for reversing the official colormapping.
