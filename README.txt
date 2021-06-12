========================================================
Cal Poly SLO
CSC 474 Computer animation
Prof. Christian Eckhardt
Quater Project - "First Person Ninja Game"
by Davin Johnson
6/11/2021
========================================================

	Goal of game:
	
		The goal of this game is to move around the map and collect all the crates spread around the map. If any of the enemy Ninjas roaming around detect you and touch you, you will die and have to restart the game. You can either run away from the Ninjas to make them stop chasing you or you can attack them with either a sword attack or throwing star attack to defeat them. Once you deal enough damage to a Ninja it will disappear.

	Game Controls:
	
		Movement
			w - walk forward
			a - walk left
			s - walk backward
			d - walk right
			lctrl - crouch
			
		Attacks
			mouse left click - sword attack
			e - throwing star attack
			
		Other
			r - restart game after game over (death or win)
			esc - quit game
			shift + mouse left click - focus/unfocus mouse cursor to game window
			
========================================================

Requriments: 
	-Windows 10
	-Visual Studio 2019 (or latest release)

To install and run, extract ext folder to the same directory as resources and src folders. Also extract OpenGL folder to the C: root directory. Then double click the .sln file to open in Visual Studio, right click "Quarter Project" in the solution explorer on the left and select "Set as Startup Project". Then press F5 to build and run the game.
