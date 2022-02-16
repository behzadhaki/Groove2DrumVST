![MIDI Fx Plugin](https://user-images.githubusercontent.com/35939495/153415111-1811ea40-ffb0-4c51-b0c2-f27d96cc3dd9.jpg)


# Info
This is repo has been generated from https://github.com/eyalamirmusic/JUCECmakeRepoPrototype
The only difference with the template above is that cmakelists and the xx_components.cpp have been updated so as to be able to use pytorch within the apps/plugins 

# Installation
1. Install libtorch (or if you have torch skip) - https://pytorch.org/cppdocs/installing.html
	
	
	1. go to https://pytorch.org/get-started/locally/ and get the C++ LibTorch Source
		In my case:	https://download.pytorch.org/libtorch/cpu/libtorch-macos-1.10.2.zip
	3. unzip the file 
	4. format the  $TORCH_PATH path as follows:
		/Users/behzadhaki/Downloads/libtorch/share/cmake
		This is your {$TORCH_PATH}
	
2. run cmake with {$TORCH_PATH} (in clion can be added in cmake settings)

    
	cmake -DCMAKE_PREFIX_PATH={$TORCH_PATH}
	example


# JUCE CMake Repo Prototype
A prototype to model a way to create an entire repo using JUCE 6 and CMake.

This is inspired by a desire to keep the environment setting of my projects to minimum,
making sure the the environment is identical for every developer/machine.

The main concept is to set all the different variables (where JUCE is, custom modules, etc) 
in the top CMakeLists.txt, then add all your projects with very little setup time.

Another important concept is to share all 'related' projects under the same configuration,
which I prefer, since it encourages code-sharing and build system settings sharing.
In some of the examples I added minimal usages of juce-style modules to illustrate how that
can be done.

To build, all you have to do is load this project in your favorite IDE 
(CLion/Visual Studio/VSCode/etc) 
and click 'build' for one of the targets (templates, JUCE examples, Projucer, etc).

You can also generate a project for an IDE by using (Mac):
```
cmake -G Xcode -B build
```
Windows:
```
cmake -G "Visual Studio 16 2019" -B build
```
For package management, I'm using the amazing CPM.cmake:
#https://github.com/TheLartians/CPM.cmake
It automatically fetches JUCE from git, but you can also set the variable:
CPM_JUCE_SOURCE to point it to a local folder, by using:
``-DCPM_JUCE_SOURCE="Path_To_JUCE"``
when invoking CMake

JUCE can be found here:
#https://github.com/juce-framework/JUCE

License:
Anything from me in this repo is completely free to use for any purpose. 
However, please check the licenses for CPM and JUCE as described in their repo. 
