# Uneven

Uneven will efficently enforce a constant microphone volume level, to counter annoying volume "leveling" adjustments performed by e.g. BlueJeans.

## Compile

    dnf install pulseaudio-libs-devel "@C Development Tools and Libraries"
    ./build.sh

## Usage

Call the binary with exactly two arguments:

    ./uneven SOURCE_NAME VOL_PERCENT

for example:

    ./uneven alsa_input.pci-0000_00_1b.0.analog-stereo 10

To find the Pulse Audio name of your device, use the following command to list all sources:

    pactl list sources

The "Description" field is often the name displayed in GUIs, and the "Name" field is what is required for SOURCE_NAME.
