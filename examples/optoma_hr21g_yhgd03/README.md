# Optoma HR21G-YHGD03 profile

Portable Universal Remote package derived from the working `congnvp/Optoma_Projector_HR21G-YHGD03` project and its captured `Optoma_HR21G-YHGD03.ir` file.

All 18 NEC commands remain in `signals.ir`. The layout represents them with 13 UI elements over three sparse pages:

- Page 1: Power, Aspect, Source, Mode, Settings, Menu.
- Page 2: D-pad/Enter plus Return.
- Page 3: Volume stepper, Mute, Freeze, Keystone and AV Mute.

The standalone app sends one normal NEC frame for a short press, so this package uses `IrBurst: 1`. Repeat-capable controls may use NEC repeat semantics through the common IR transport.

Hardware TX remains pending until tested with the physical projector.
