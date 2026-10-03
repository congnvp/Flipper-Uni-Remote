Filetype: Flipper Uni Remote
Version: 1
Id: lg_ac
Name: LG AC
ShortName: LG
Transport: STATE_IR
Order: 30
Favourite: false
Folder: AC
RepeatEnabled: true
PageCount: 3
IrBurst: 1
SignalFile: signals.ir
ActionFile: actions.ur
BluetoothProfile:
StateAdapter: LG_AC
StateFile: state.urs
HardUpHold:
HardDownHold:
HardLeftHold:
HardRightHold:
HardOkHold:
ElementCount: 17
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
Element6Type: hstep
Element6Id: swing_v
Element6Page: 1
Element6Rect: 0 1 3 1
Element6Label: SW V
Element6Left: state:swing_v:-
Element6Right: state:swing_v:+
#
Element7Type: hstep
Element7Id: swing_h
Element7Page: 1
Element7Rect: 0 2 3 1
Element7Label: SW H
Element7Left: state:swing_h:-
Element7Right: state:swing_h:+
#
Element8Type: button
Element8Id: jet
Element8Page: 1
Element8Rect: 0 3 1 2
Element8Label: JET
Element8Tap: state:jet:toggle
#
Element9Type: button
Element9Id: eco
Element9Page: 1
Element9Rect: 1 3 1 2
Element9Label: ECO
Element9Icon: eco
Element9Tap: state:eco:toggle
#
Element10Type: button
Element10Id: comfort
Element10Page: 1
Element10Rect: 2 3 1 2
Element10Label: COMF
Element10Tap: state:comfort:toggle
#
Element11Type: button
Element11Id: display
Element11Page: 2
Element11Rect: 0 0 1 2
Element11Label: DISP
Element11Tap: state:display:toggle
#
Element12Type: button
Element12Id: auto_clean
Element12Page: 2
Element12Rect: 1 0 1 2
Element12Label: CLEAN
Element12Tap: state:auto_clean:toggle
#
Element13Type: button
Element13Id: purify
Element13Page: 2
Element13Rect: 2 0 1 2
Element13Label: AIR
Element13Tap: state:purify:toggle
#
Element14Type: button
Element14Id: jet_dry
Element14Page: 2
Element14Rect: 0 2 1 2
Element14Label: DRY+
Element14Tap: state:jet_dry:toggle
#
Element15Type: button
Element15Id: unit
Element15Page: 2
Element15Rect: 1 2 1 2
Element15Label: C/F
Element15Tap: state:unit:toggle
#
Element16Type: button
Element16Id: diagnosis
Element16Page: 2
Element16Rect: 2 2 1 2
Element16Label: DIAG
Element16Tap: state:diagnosis:send
