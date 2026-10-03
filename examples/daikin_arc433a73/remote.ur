Filetype: Flipper Uni Remote
Version: 1
Id: daikin_arc433a73
Name: Daikin ARC433A73
ShortName: DAI
Transport: STATE_IR
Order: 31
RepeatEnabled: true
PageCount: 2
IrBurst: 1
SignalFile: signals.ir
ActionFile: actions.ur
BluetoothProfile:
StateAdapter: DAIKIN_ARC433A73
StateFile: state.urs
HardUpHold:
HardDownHold:
HardLeftHold:
HardRightHold:
HardOkHold:
ElementCount: 8
#
Element0Type: status
Element0Id: status
Element0Page: 0
Element0Rect: 0 0 3 1
#
Element1Type: screen
Element1Id: state
Element1Page: 0
Element1Rect: 0 1 3 2
Element1Label: LOCAL
#
Element2Type: hstep
Element2Id: temp
Element2Page: 0
Element2Rect: 0 3 3 1
Element2Label: TEMP
Element2Left: state:temp:-
Element2Right: state:temp:+
#
Element3Type: hstep
Element3Id: mode
Element3Page: 0
Element3Rect: 0 4 3 1
Element3Label: MODE
Element3Left: state:mode:-
Element3Right: state:mode:+
#
Element4Type: button
Element4Id: power
Element4Page: 0
Element4Rect: 1 5 1 1
Element4Label: PWR
Element4Icon: pwr
Element4Tap: state:power:toggle
#
Element5Type: hstep
Element5Id: fan
Element5Page: 1
Element5Rect: 0 0 3 1
Element5Label: FAN
Element5Left: state:fan:-
Element5Right: state:fan:+
#
Element6Type: button
Element6Id: swing
Element6Page: 1
Element6Rect: 0 1 1 2
Element6Label: SWING
Element6Tap: state:swing_v:toggle
#
Element7Type: button
Element7Id: powerful
Element7Page: 1
Element7Rect: 2 1 1 2
Element7Label: POWER
Element7Tap: state:powerful:toggle
