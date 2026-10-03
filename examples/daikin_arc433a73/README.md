# Daikin ARC433A73 stateful profile

Uses `Transport: STATE_IR` and adapter `DAIKIN_ARC433A73`.

The adapter is ported from `congnvp/daikin_ac_ir_flipper_zero`: 35-byte state frame, section checksums and 584 raw timings. Default temperature is 23 C. Supported default-layout controls are Power, Mode, Temperature, Fan, Swing and Powerful.

`state.urs` is local remembered state, not device feedback.
