# Image-Segmentation

This repository built of two components, each located in a different directory. Each directory has its own ReadME file to assist testing. These projects have been designed to work on different platforms:

Windows-C++ Version (imgSeg directory): This project works by using Qt and C++ to multi-thread image preprocessing and K-Means Image Segmentation. Program was written with Qt 5.12 but anything newer than 4 should allow the program to function. The project file is included so that easy building is possible.

Palmetto-Python Version (palmetto directory): This project works by using Python's OpenCV library to perform K-Means Image Segmentation. It uses GNU-Parallel to distribute image across multiple nodes in the Palmetto Cluster, and Python's Multiprocessing Library to run different iterations of K-Means on multiple CPU cores.
