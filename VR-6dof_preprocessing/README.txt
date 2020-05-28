This is the source code implementing the approach (preprocessing step) 
described in the paper "Motion parallax for 360 RGBD video". 

This code is provided "as is", without warranty of any kind. Permission is granted for non-commercial 
purposes. If you use our code or demo for your research we kindly ask you to cite our paper:

Serrano, A., Kim, I., Chen, Z., DiVerdi, S., Gutierrez, D., Hertzmann, A., and Masia, B.
“Motion parallax for 360 RGBD video”. 
IEEE Transactions on Visualization and Computer Graphics, 2019.

//////////////////////////////////////
///           DETAILS             ///
/////////////////////////////////////
Requirements: 
- Oculus Rift CV1 and Oculus App
- Matlab 2018a

This code has been tested under the following environment:
- Windows 10 x64 equiped with a Nvidia Titan X (Pascal), and 32GB RAM.
- Oculus App Last Version tested: 1.35.0.206993

//////////////////////////////////////
///           HOW TO USE          ///
/////////////////////////////////////
Place in the _input_videos folder the desired video + depth following the
convention (as in the example provided): 
- name_of_video.mp4
- name_of_video_depth.mp4

Several folders and temporary files containing the output from different
stages of the processing will be saved during the process, and (optionally)
deleted when finished.

The output files, ready to use in our viewer, will be stored in a new
folder called _vid2viewer

//////////////////////////////////////
///      MAIN FILES INCLUDED      ///
/////////////////////////////////////
- main_call.m: This is the main script that will call to all the steps of the
      preprocessing

- main_depth_improving.m: Main script corresponding to the depth improving
      as described in the paper.
- triangle_orientations.exe: Computes the initial opacity map. 
      The depth is projected into a 3D mesh as described in the paper, and the 
      orientation of each triangle of the mesh w.r.t. the center of projection 
      is stored (0 meaning perpendicular orientation).  

- main_alpha_processing.m: Computes the transparency values from the
      triangle orientations as described in the paper.
- extrapolated_layer: Computes the extrapolated layer as described in the
      paper.
- inpainted_layer.m: Computes the inpainted layer as described in the
      paper.
