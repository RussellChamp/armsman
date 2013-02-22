armsman
=======

An interactive DPS calculator for the Pathfinder system written in C++ and QT Creator 2.6.2
Written by Russell Champoux in February 2013

Purpose:
* I initially wrote this to be able to figure out whether Vital Strike or Deadly Stroke was able to output a higher damage per second under several situations
* Things evolved from there to support a whole lot more

To run:
* Compile this yourself using QT Creator
* Ok, I may upload some compiled binaries later
* I haven't yet figured out how to staticly link libraries in QT  so it'll look kinda ugly... and large

Current Functionality:
* Calculate the expected damage of 1H/2H melee attacks or ranged attacks based on weapon and attack type
* Supports many feats that increase damage, hit bonus, crit range, and crit confirm bonus
* Finally answers "Which is better for me: vital strike or deadly stroke?"

To do:
* Finish support for Manyshot and Rapidshot
* Add Paladin specific feats/abilities
* Add Rogue specific feats/abilities
* Add Monk specific feats/abilities
* Add Ranger specific feats/abilities
* Add Two Weapon Fighting
* Improve "warning" panel
