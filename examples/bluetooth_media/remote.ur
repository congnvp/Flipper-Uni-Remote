Filetype: Flipper Uni Remote
Version: 1
Id: bluetooth_media
Name: Bluetooth Media
ShortName: BTM
Transport: BT
Order: 40
Favourite: false
Folder: BLUETOOTH
RepeatEnabled: true
PageCount: 2
IrBurst: 1
SignalFile: signals.ir
ActionFile: actions.ur
BluetoothProfile: UniMedia
StateAdapter:
StateFile: state.urs
HardUpHold:
HardDownHold:
HardLeftHold:
HardRightHold:
HardOkHold:
ElementCount: 11
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
Element1Label: PAIRING
#
Element2Type: hstep
Element2Id: track
Element2Page: 0
Element2Rect: 0 3 3 1
Element2Label: TRACK
Element2Left: bt:media:prev
Element2Right: bt:media:next
#
Element3Type: vstep
Element3Id: volume
Element3Page: 0
Element3Rect: 0 4 1 2
Element3Label: VOL
Element3Up: bt:media:vol_up
Element3Down: bt:media:vol_down
#
Element4Type: button
Element4Id: play
Element4Page: 0
Element4Rect: 1 4 1 2
Element4Label: PLAY
Element4Tap: bt:media:play_pause
#
Element5Type: button
Element5Id: mute
Element5Page: 0
Element5Rect: 2 4 1 2
Element5Label: MUTE
Element5Icon: mut
Element5Tap: bt:media:mute
#
Element6Type: dpad
Element6Id: nav
Element6Page: 1
Element6Rect: 0 1 3 3
Element6Up: bt:key:up
Element6Down: bt:key:down
Element6Left: bt:key:left
Element6Right: bt:key:right
Element6Ok: bt:key:enter
Element6AltSticky: false
#
Element7Type: button
Element7Id: home
Element7Page: 1
Element7Rect: 0 1 1 1
Element7Label: HOME
Element7Tap: bt:media:home
#
Element8Type: button
Element8Id: back
Element8Page: 1
Element8Rect: 2 1 1 1
Element8Label: BACK
Element8Icon: back
Element8Tap: bt:media:back
#
Element9Type: button
Element9Id: escape
Element9Page: 1
Element9Rect: 1 4 1 2
Element9Label: ESC
Element9Tap: bt:key:escape
#
Element10Type: button
Element10Id: space
Element10Page: 1
Element10Rect: 2 4 1 2
Element10Label: SPACE
Element10Tap: bt:key:space
