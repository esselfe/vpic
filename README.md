# vpic

vpic is a program for viewing JPEG and PNG image files.  
It is written in C and uses X11, libpng and libjpeg-turbo.  

To compile just type 'make' from the extracted source directory  
then './vpic'. Currently there's no install procedure as the  
program is designed to be ran from the source tree and read  
file from the "images" directory.  

Note that this program is in alpha stage and only displays  
correctly RGB/RGBA PNG colorspace, not 8bit-palette/index-colors  
due to lack of documentation.  

