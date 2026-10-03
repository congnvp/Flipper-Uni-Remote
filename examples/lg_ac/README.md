# LG AC stateful profile

Uses `Transport: STATE_IR` and adapter `LG_AC`.

The encoder and auxiliary command semantics are ported from the working `congnvp/LG_AC_ir_flipper-zero` project. Temperature is 18–30 C, fan is Auto/1–5, vertical swing is Off/S1..S6/Auto, and horizontal swing is Off/Left/LC/Center/RC/Right/ranges/All.

The UI deliberately exposes common daily controls over three sparse pages. Timer operations are implemented by the adapter but are not placed on the default layout.

`state.urs` contains remembered LOCAL state only; it is not confirmation from the air conditioner.
