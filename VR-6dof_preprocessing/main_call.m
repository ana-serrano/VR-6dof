% This is the source code implementing the approach (preprocessing step) 
% described in the paper "Motion parallax for 360 RGBD video". 
%
% This code is provided "as is", without warranty of any kind. Permission is granted for non-commercial 
% purposes. If you use our code or demo for your research we kindly ask you to cite our paper:
% 
% Serrano, A., Kim, I., Chen, Z., DiVerdi, S., Gutierrez, D., Hertzmann, A., and Masia, B.
% “Motion parallax for 360 RGBD video”. 
% IEEE Transactions on Visualization and Computer Graphics, 2019.
%
% //////////////////////////////////////
% ///           DETAILS             ///
% /////////////////////////////////////
% Requirements: 
% - Oculus Rift CV1 and Oculus App
% - Matlab 2018a
% 
% This code has been tested under the following environment:
% - Windows 10 x64 equiped with a Nvidia Titan X (Pascal), and 32GB RAM.
% - Oculus App Last Version tested: 1.35.0.206993
%
% //////////////////////////////////////
% ///           HOW TO USE          ///
% /////////////////////////////////////
% Place in the _input_videos folder the desired video + depth following the
% convention (as in the example provided): 
% - name_of_video.mp4
% - name_of_video_depth.mp4
%
% Several folders and temporary files containing the output from different
% stages of the processing will be saved during the process, and deleted
% when finished.
%
% The output files, ready to use in our viewer, will be stored in a new
% folder called _vid2viewer
%
% //////////////////////////////////////
% ///      MAIN FILES INCLUDED      ///
% /////////////////////////////////////
% - main_call.m: This is the main script that will call to all the steps of the
%       preprocessing
%
% - main_depth_improving.m: Main script corresponding to the depth improving
%       as described in the paper
%
% - triangle_orientations.exe: Computes the initial opacity map. 
%       The depth is projected into a 3D mesh as described in the paper, and the 
%       orientation of each triangle of the mesh w.r.t. the center of projection 
%       is stored (0 meaning perpendicular orientation).
%   **TO-DO**: This .exe calls the Oculus software because it was
%       implemented under the same framework as our viewer. However, this
%       operation just performs simple mesh operationsdoes and does not depend 
%       on the Oculus, so it could be implemented as a standalone (without 
%       requiring the Oculus platform).
% 
% - main_alpha_processing.m: Computes the transparency values from the
%       triangle orientations as described in the paper
% - extrapolated_layer: Computes the extrapolated layer as described in the
%       paper.
% - inpainted_layer.m: Computes the inpainted layer as described in the
%       paper.

clear all
close all
warning('off')


%Filename (both RGB and depth must me inside _input_videos)
filename = 'bishop03_1';

%First we call the depth improving script
disp('STARTING DEPTH PROCESSING');
main_depth_improving;

%Take the filtered video and calculate triangle orientations
disp('COMPUTING TRIANGLE ORIENTATIONS');
arg1 = ['_improved_depth/' filename '/videos/'];
arg2 = ['_triangle_orientations/' filename];
mkdir(arg2);
system(sprintf('triangle_orientations.exe %s %s %s',arg1,filename,arg2));

%Stitch triangle orientations into panoramas and compute alpha values
disp('COMPUTING TRANSPARENCY VALUES');
folder = ['_triangle_orientations/' filename];
filename_in = filename;
main_alpha_processing;

%Compute extrapolated layer
disp('COMPUTING EXTRAPOLATED LAYER');
extrapolated_layer;

%Compute triangle orientations of extrapolated layer
disp('COMPUTING TRIANGLE ORIENTATIONS OF EXTRAPOLATED LAYER');
arg1 = ['_extrapolated_layer/' filename];
arg2 = [filename '_BG'];
arg3 = ['_extrapolated_layer/' filename '/_triangle_orientations'];
mkdir(arg3);
system(sprintf('triangle_orientations.exe %s %s %s',arg1,arg2, arg3));

%Stitch triangle orientations into panoramas and compute alpha values for
%inpainted layer
disp('COMPUTING TRANSPARENCY VALUES OF EXTRAPOLATED LAYER');
folder = ['_extrapolated_layer/' filename '/_triangle_orientations'];
filename_in = [filename '_BG'];
main_alpha_processing;
vv = VideoReader([folder '/' filename_in '_alphaproc.mp4']);
img = readFrame(vv);
imwrite(img, ['_extrapolated_layer/' filename '/' filename '_BGA.png'])

%Compute inpainted layer
disp('COMPUTING INPAINTED LAYER');
inpainted_layer;

%Arrange everything
disp('SAVING INTO _vid2viewer FOLDER');
mkdir(['_vid2viewer/' filename]);
copyfile(['_improved_depth/' filename '/videos/' filename '.mp4'],['_vid2viewer/' filename '/' filename '.mp4'])
copyfile(['_improved_depth/' filename '/videos/' filename '_depth.mp4'],['_vid2viewer/' filename '/' filename '_depth.mp4'])
copyfile(['_triangle_orientations/' filename '/' filename '_alphaproc.mp4'],['_vid2viewer/' filename '/' filename '_alphaproc.mp4'])
copyfile(['_extrapolated_layer/' filename '/' filename '_BG.png'],['_vid2viewer/' filename '/' filename '_BG.png'])
copyfile(['_extrapolated_layer/' filename '/' filename '_BG_depth.png'],['_vid2viewer/' filename '/' filename '_BGD.png'])
copyfile(['_extrapolated_layer/' filename '/' filename '_BGA.png'],['_vid2viewer/' filename '/' filename '_BGA.png'])
copyfile(['_inpainted_layer/' filename '/' filename '_BG_inp.png'],['_vid2viewer/' filename '/' filename '_BG_inp.png'])
copyfile(['_inpainted_layer/' filename '/' filename '_BGD_inp.png'],['_vid2viewer/' filename '/' filename '_BGD_inp.png'])


%Delete temporary files (if wanted)
disp('DELETING TEMPORARY FILES');
clear all
close all
rmdir _extrapolated_layer s
rmdir _improved_depth s
rmdir _inpainted_layer s
rmdir _triangle_orientations s