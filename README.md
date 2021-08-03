# piphys

Piphys is available as a preprint on bioarXiv: 

[Light-weight electrophysiology hardware and software platform for cloud-based neural recording experiments](https://www.biorxiv.org/content/10.1101/2021.05.18.444685v2)

--------
## Overview

Piphys is an inexpensive open source neurophysiological recording platform based on Raspberry Pi. It is easily accessed and controlled via a standard web interface through Internet of Things (IoT) protocols. 

Piphys modules can be used in a fleet to enable long-term observations of neural development, organization, and activity at scale. 

## Design Files

### Hardware
- Raspberry Pi expansion circuit board for and Intan RHD2000 bioamplifier chip.
- Electrode adapter circuit board for Axion CytoView MEA Plate.

Source files, documentation, and files for manufacturing are in [`./Hardware`](https://github.com/braingeneers/piphys/Hardware).

### Mechanical
CAD files in for the electrode plate holder are in [`./Mechanical/AxionPlateHolder`](https://github.com/braingeneers/piphys/AxionPlateHolder).

```
AxionPlateHolder_CNC_ClampTop_Aluminum
---------------------------------------
[Axion CytoView MEA Plate]
---------------------------------------
AxionPlateHolder_CNC_ClampBottom_Plastic
AxionPlateHolder_CNC_ClampBottom_Aluminum
```

### Software
Software to enable voltage sampling and user interaction, with accopmanying documentation are in [`./Software`](https://github.com/braingeneers/piphys/Software).

See [`braingeneerpy`](https://github.com/braingeneers/braingeneerspy) Python package for IoT communication library.

----
© 2021 Braingeneers
