Filetype: Flipper Uni Remote
Version: 1
Id: living_tv
Name: Living TV
ShortName: TV
Transport: IR
Order: 10
RepeatEnabled: true
SignalFile: signals.ir
ActionFile: actions.ur
BluetoothProfile: living_tv
HardUpHold:
HardDownHold:
HardLeftHold:
HardRightHold:
HardOkHold: act:quiet
ElementCount: 6
#
Element0Type: status
Element0Id: status
Element0Rect: 0 0 3 1
#
Element1Type: screen
Element1Id: main
Element1Rect: 0 1 3 2
Element1Label: READY
#
Element2Type: hstep
Element2Id: channel
Element2Rect: 0 3 3 1
Element2Label: CH
Element2Left: sig:ChDown
Element2Right: sig:ChUp
#
Element3Type: vstep
Element3Id: volume
Element3Rect: 0 4 1 2
Element3Label: VOL
Element3Up: sig:VolUp
Element3Down: sig:VolDown
#
Element4Type: button
Element4Id: power
Element4Rect: 1 4 1 2
Element4Label: PWR
Element4Icon: pwr
Element4Tap: sig:Power
Element4Hold: sig:Mute
#
Element5Type: vstep
Element5Id: source
Element5Rect: 2 4 1 2
Element5Label: SRC
Element5Up:
Element5Down: act:mute_alias
