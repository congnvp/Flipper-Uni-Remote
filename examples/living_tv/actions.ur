Filetype: Flipper Uni Remote Actions
Version: 1
ActionCount: 3
#
Action0Id: quiet
Action0Type: sequence
Action0StepCount: 2
Action0Step0: VolDown
Action0Delay0: 120
Action0Step1: VolDown
Action0Delay1: 0
#
Action1Id: mute_alias
Action1Type: signal
Action1Signal: Mute
#
Action2Id: quiet_mute
Action2Type: sequence
Action2StepCount: 2
Action2Step0: act:quiet
Action2Delay0: 150
Action2Step1: sig:Mute
Action2Delay1: 0
