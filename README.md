# Uneven - Microphone volume unleveler

Uneven will efficently enforce a constant microphone volume level, to counter annoying volume "leveling" adjustments performed by e.g. BlueJeans.

## Usage

Call the binary with exactly two arguments:

    ./uneven SOURCE_NAME VOL_PERCENT

for example:

    ./uneven alsa_input.pci-0000_00_1b.0.analog-stereo 10

To find the Pulse Audio name of your device, use the following command to list all sources:

    pactl list sources

The "Description" field is often the name displayed in GUIs, and the "Name" field is what is required for SOURCE_NAME.

## Install

### Fedora

    dnf copr enable aszczucz/uneven
    dnf install -y uneven

## CentOS

    curl -o /etc/yum.repos.d/_copr_aszczucz-uneven.repo https://copr.fedorainfracloud.org/coprs/aszczucz/uneven/repo/epel-7/aszczucz-uneven-epel-7.repo
    yum install -y uneven

## Compile

    dnf install pulseaudio-libs-devel "@C Development Tools and Libraries"
    ./build.sh

