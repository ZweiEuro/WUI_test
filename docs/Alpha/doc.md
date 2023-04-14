# Magic Eight

Dominik Völkel 11811035 d.voelkel@student.tugraz.at

Phillip Stranger 11807773 phillip.stranger@student.tugraz.at

Notes from ass2:

Angular momentum visualization: we have no angular momentum.

Seperated Update rate and time delta multiplier.

Implemented FPS updater (do not switch this around to much the internal timer might not like it).

Notes for final release:

Force Field checkbox added.

Object path is shown in debug mode.

Toggle box for euler / RK4

Step size h slider for euler/RK4

Voronoi - currently only 15 voronoi cells, as more cells need to much calculation time to be calculated dynamically (they stall the game)

It can happen that a ball bugs into or through a rectangle. This happens when the ball is faster than the Physics Engine can handle the collision i.e. the ball is already fully into or though the rectangle when the Physics Engine handles the collision. This happens most commonly when the ball gets pushed through the rectangle i.e. it cannot escape the acceleration, so it bugs into/through the rectangle.

It can happen sometimes, that the visualization of the voronoi diagram fails for cells, as we have to generate the visualization via calculating the convex hull of the points residing in one cell. As points in the cells, especially in rotated rectangles, can have offsets to the actual pixel location (floating point offsets) this can lead to difficulties when generating the convex hull.

## 0. Needed libraries:

Firstly on ubuntu community repository (universe) need to be enabled. (Tutorial: https://www.linuxshelltips.com/enable-universe-repository-ubuntu/)

Allegro needs to be installed:
https://github.com/liballeg/allegro_wiki/wiki/Quickstart

under Linux -> Ubuntu

```
sudo apt update

sudo add-apt-repository ppa:allegro/5.2
sudo apt-get install liballegro*5.2 liballegro*5-dev

#and then we install spdlog
sudo apt install libspdlog-dev

```

## 1. Excecution

Next to this file in this filder is a shell script "start.sh" that links ./res for the game and starts it.
With a terminal inside the extracted folder you can execute the game with:

```
./start.sh
```

## 2. Manual

Clicking with the mouse onto the table will fire the white ball to the mouse with increasing speed the further the click is away from the white ball.
Esc -> shutdown and exit
P -> Pause physics and collision
R -> reset game objects
S -> show/hide settings

The settings can change a time delta premultiplier to slow down the physics simulation.
A debug view may be enabled to show collision bodies, past collision points and velocity vectors.
Additionaly the view shows uninteractable Catmull-rom splines. These do not have a function yet.

Settings:

The FPS counter also affects the GUI rendering.
The PPS counter (phyics) also handles the GUI logic. (lowering the PPS too much will make the system very unstable and increase the likelyhood of moving 2 objects into each other (as we calculate with delta time since last phyics update. This means that at sub 30 the system might crash as some objects are flung out of the playspace into nan)).

As our physics manager uses delta time, h needs to be very small or else the calculation will have no impact.

If PPS are too low objects clip into each other before the collision can catch which causes a numerical error that the collision normal doesn't exist.

## 3. Tech

Rigid Body collision:
GameObject/RigidBody.cpp implements rigid body collisions resolution.
GameObject/GameObject.cpp implements collision detection.

The physics are simulated by the Gamecore/PhysicsManager.cpp

Path interpolation:
WIP arc length parametrization not done yet.
math/PathInterpol.cpp

## Note

If you want to compile this yourself you will need
allegro5 and spdlog as libraries in your system.
