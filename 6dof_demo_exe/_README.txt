This is a demo implementing the approach described in the paper "“Motion parallax for 360 RGBD video”". 
This demo is provided "as is", without warranty of any kind. Permission is granted for non-commercial 
purposes. If you use our code or demo for your research we kindly ask you to cite our paper:

Serrano, A., Kim, I., Chen, Z., DiVerdi, S., Gutierrez, D., Hertzmann, A., y Masia, B.
“Motion parallax for 360 RGBD video”. 
IEEE transactions on visualization and computer graphics, 2019.


//////////////////////////////////////
///      TECHNICAL SPECS          ///
/////////////////////////////////////

This demo has been tested under the following environment:
- Windows 10 x64 equiped with a Nvidia Titan X (Pascal)
- Oculus App Last Version tested: 1.35.0.206993

//////////////////////////////////////
///    SETTINGS AND CONTROLS      ///
/////////////////////////////////////

Controls:
R 		- Recenter to center of projection
T 		- Activate our system(6-DoF), active by default
Y 		- DeActivate our system (3-DoF)  
SPACE 	- Pause/continue video playback
C 		- Activate/Deactivate coloring of the layers (refer to Fig. 16 in the main paper for more details)
ESC 	- Quit application

Use the settings.txt file to select the video. Options for videos:
- cafeteria
- pier
- shore

The video will loop automatically until exiting the application.