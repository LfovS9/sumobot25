# SumoBot 2025

## Overview

This project documents the design and implementation of a two-wheel autonomous sumo robot. This repo contains the control software used during the competition and the CAD models used to manufacture the robot chassis and components through 3D printing.

The robot detects opponents using an ultrasonic distance sensor and avoids exiting the ring using a downward-facing infrared edge sensor. The control logic manages several operating states including searching, approaching, charging, and edge recovery.

## Hardware

Main components used in the robot:

* Microcontroller: Arduino Nano
* Motor driver: TB6612FNG
* Motors: two DC gear motors (500 RPM, 6V)
* Sensors:
  * HC-SR04 ultrasonic distance sensor (opponent detection)
  * downward-facing IR reflectance sensor (edge detection)
* Power system: onboard battery pack
* Chassis: custom 3D printed frame

## Software

The control system is written in C++ for the Arduino environment.

Key behaviors implemented in the code include:

* **Search mode** – forward movement with alternating curvature to scan for opponents
* **Approach mode** – faster movement when an opponent is detected within a specified distance
* **Charge mode** – full-speed attack when the opponent is within close range
* **Edge recovery** – immediate retreat and pivot when the edge of the ring is detected
* **Target confirmation logic** – multiple sensor readings are required before committing to an attack
* **Attack lock** – briefly maintains an attack if the opponent is temporarily lost from the sensor

The code structure is organized around a simple state machine controlling these behaviors.

## Repository Structure

```
sumobot25/
│
├── Code/
│   Arduino control programs for the robot
│
├── IPTs/
│   CAD files used for designing and printing the chassis
│
└── README.md
```

## Contributors

* Anson L, Leo L, William C, Edwin L
