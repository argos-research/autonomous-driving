---
title: Team speedDreams
author:
- Alexander Reisner
- Alexander Weidinger
- David Werner
institute: Technical University of Munich (TUM)
abstract: This repository contains the source code of 'Team speedDreams', which implements an autonomous parking scenario for the lil4 practical course.
---

# Repository structure
```
.
+-- ecu                    // (contains code for the ecus)
|   +-- autonomousParking  // (our autonomous parking ecu)
+-- genode
|   +-- ...
+-- simulators             // (the collection of simulators in use)
|   +-- speed-dreams
|   +-- sumo
+-- tools                  // (additional tools or utility software)
|   +-- QEMU-SA-VM
|   +-- SimCoupler
```

# Build instructions
- The repository consists of multiple submodules, which first need to be fetched via `git submodule update --init`.
- ...
