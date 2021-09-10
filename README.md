# NeuroLab

Device Connection:
-> plug in usb cable into the computer
-> connect battery pack to the device by placing the red cable to it
-> attach the electrodes to channels 1-32 since that is all we are dealing with
-> Open Visual Studio
-> Connection will be established once the code is running

Main Branch:
 ->Clean code before editing from the Brain Vision Amplifier website.
 -> Click 0 to connect to the device

7/16/2021 Branch:
 -> got multiple files to read automatically and save all data into one file
 ->Once we start, we have to manually adjust the sample rate to option 0 and number 3 for a sample rate of 1000
 -> Always make sure to click 1 to verify that you are connected to channels 1-32 when prompted

9/10/2021 Branch:
->Once we start, we have to manually adjust the sample rate to option 0 and number 3 for a sample rate of 1000
-> Always make sure to click 1 to verify that you are connected to channels 1-32 when prompted
-> Click enter and then we manually enter a file name
-> Recording begins and exits once finished


****Known Errors:
-> If visual studio is not connecting to the device at runtime, restart the computer, just restarting Visual Studio will not reset the device connection
-> Files will store into one big EEG file instead of seperating into different files when we want to run multiple VHDR files to collect data when we run the code
