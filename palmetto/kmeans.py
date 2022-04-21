import numpy as np
import cv2 as cv
import sys
import matplotlib.pyplot as plt
import time
import argparse

def main(args):
    # read in image, transform to array of pixels instead of array of colors
    image_array = cv.imread(args.inf)
    image_array = cv.cvtColor(image_array, cv.COLOR_BGR2RGB)
    pixel_array = image_array.reshape((-1,3))
    pixel_array = np.float32(pixel_array)
    
    # criteria tells kmeans when to stop, will stop on iterations or convergence, 10 iterations, <= 1.0 epsilon value
    criteria = (cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_MAX_ITER, 10, 1.0)
    best_compactness = None
    best_labels = None
    best_center = None
    bestK = None
    maxK = args.maxk
    
    start = time.time()
    for K in range(1,maxK+1):
        # return values are sum of squared distances (min obj), label array, and array of centroids
        compactness,labels,center = cv.kmeans(pixel_array, K, None, criteria, args.kloops, cv.KMEANS_RANDOM_CENTERS)
        if best_compactness is None or best_compactness-args.thresh > compactness:
            best_compactness = compactness
            best_labels = labels
            best_center = center
            bestK = K
    end = time.time()
    
    # some performance metrics
    elapsed = end-start
    # total problem size is equal to the number of bytes in image * K clustering attempts * # of internal loops
    imgB = pixel_array.size * 4 * maxK * args.kloops
    imgGB = imgB / (1e9)
    print("Elapsed Time for Clustering: "+str(elapsed))
    print("Clustering Throughput (GB/S): "+str(imgGB/elapsed))
    print("Optimal # of Clusters: "+str(bestK))
    
    # post-processing of image to segmented centers
    best_center = np.uint8(best_center)
    segimg_array = best_center[best_labels.flatten()]
    segimg_array = segimg_array.reshape((image_array.shape))
    # svae segmented image to output file
    plt.imsave(args.outf, segimg_array)
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--inf", required=True, help="Input image for processing.")
    parser.add_argument("--outf", required=True, help="Output file for segmented image.")
    parser.add_argument("--maxk", type=int, nargs='?', default=10, help="Max number of clusters to attempt in segmentation.")
    parser.add_argument("--kloops", type=int, nargs='?', default=10, help="Number of internal iterations OpenCV kmeans will perform")
    parser.add_argument("--thresh", type=float, nargs='?', default=0.0, help="Minimum improvement for K-Clusters to be considered better.")
    args = parser.parse_args()
    main(args)
