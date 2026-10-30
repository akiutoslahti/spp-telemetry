spp-telemetry

A small telemetry system built around the CCSDS Space Packet Protocol. Written to learn modern C++ and Buildroot by building something real instead of working through tutorials.

libspp: C++20 library for the Space Packet primary header (CCSDS 133.0-B-2). Parses from and serializes into spans. No allocation, no exceptions, errors as values. Tracks per-APID sequence counts and reports gaps.
tmd: Linux user space daemon. Receives Space Packets over UDP, validates them, keeps per-APID statistics and answers queries over a Unix domain socket. Single thread on epoll, logs through syslog.
spp-gen: Python packet generator. Sends valid streams or injects faults: bad version, length mismatch, truncation, sequence gaps, idle packets.
br2-external: Buildroot external tree that packages tmd with an init script into a QEMU aarch64 image.

All code is written by hand. An AI assistant is used for review and as a reference, see CLAUDE.md. The design and the schedule are in PLAN.md.

Status: in progress.
