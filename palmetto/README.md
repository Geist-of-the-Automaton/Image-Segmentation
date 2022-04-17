# Kmeans Cluster Algorithm for use on Palmetto in Python with OpenCV library

## Running Instructions:
For running in general, the following Python modules are used:
- OpenCV
- Numpy
- Matplotlib
- argparse

On palmetto, the software stack is less flexible and limited in your ability to install these outside of virtual environments. In order to install the need modules on palmetto the best approach is to use anaconda. The following details the steps to set up the virtual environment. 
1. load anaconda module
2. create a directory to store the virtual environment i.e. ~/software/venv/imgseg
3. create the virtual environment from system packages i.e. python3 -m venv --system-site-packages ~/software/venv/imgseg
4. activate the virtual environment i.e. source ~/software/venv/imgseg/bin/activate
5. use pip to install the packages as normal
6. (on repeat use) simply load the anaconda module and activate the virtual environment
