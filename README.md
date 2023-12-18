**Summary of the Game**

In the game, a spaceship will appear at the bottom of the screen. and shapes will start falling on her from the top of the screen

There will be 3 types of shapes with a length of 3,4,5 units

The units will drop at a rate of one unit per second. Every 3 seconds an additional unit of random length will fall from another place on the x-axis along the screen at maximum height.

The goal of the game is not to get such a unit to fall to the floor.
As soon as a unit touches the floor - the screen will go to game over and it will be possible to start a new game by pressing the S1 button. By moving the board left and right, the spaceship will glide to the right and left (by reading the accelerometer). Pressing one of the buttons S1 or S2 will create a "shot" marked in red that will climb at a rate of one unit per second upwards.
Hitting a shot in the middle of the formation will cause the unit to blink and disappear.

The LED light will be colored in one of three colors - if there are up to 2 units in the air in green, 2-4 in yellow more than 4 in red.
 
The entire game will be written using the change timer and button interfaces, no polling is used.
