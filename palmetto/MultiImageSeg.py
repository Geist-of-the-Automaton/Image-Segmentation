import cv2
import numpy as np
import os
import argparse
import time
import sys

def canny_segment(data):
    path = data[0]
    file_name = path.split("/")[-1]
    image_info = data[1]
    nparr = np.frombuffer(image_info, np.uint8)
    img_np = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
    edges = cv2.Canny(img_np, 100, 200)
    return (f"new_{file_name}", edges)

def kmeans_segment(args):
    maxK = args.maxk
    kloops = args.kloops
    thresh = args.thresh
    image = cv2.imread(f"./input/{args.infile}")
    image_array = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    pixel_array = image_array.reshape((-1,3))
    pixel_array = np.float32(pixel_array)

    criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 10, 1.0)
    best_compactness = None
    best_labels = None
    best_center = None
    bestK = None

    start = time.time()
    for K in range(1,maxK+1):
        # return values are sum of squared distances (min obj), label array, and array of centroids
        compactness,labels,center = cv2.kmeans(pixel_array, K, None, criteria, kloops, cv2.KMEANS_RANDOM_CENTERS)
        if best_compactness is None or best_compactness-thresh > compactness:
            best_compactness = compactness
            best_labels = labels
            best_center = center
            bestK = K
    end = time.time()

    elapsed = end-start
    imgB = pixel_array.size * 4 * maxK * kloops
    imgGB = imgB / (1e9)

    best_center = np.uint8(best_center)
    segimg_array = best_center[best_labels.flatten()]
    segimg_array = segimg_array.reshape((image_array.shape))

    return (f"new_{args.infile}", segimg_array, elapsed, imgGB/elapsed, bestK)

def main(args):
    output = kmeans_segment(args)
    with open(f"{args.outdir}/diagnostics.txt", "a+") as f:
        cv2.imwrite(f"{args.outdir}/{output[0]}", output[1])
        f.write(f"{output[0]},{output[2]},{output[3]},{output[4]}\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--infile", required=True, help="Input file for processing.")
    parser.add_argument("--outdir", required=True, help="Output directory to store segmented image.")
    parser.add_argument("--maxk", type=int, nargs='?', default=10, help="Max number of clusters to attempt in segmentation.")
    parser.add_argument("--kloops", type=int, nargs='?', default=10, help="Number of internal iterations OpenCV kmeans will perform")
    parser.add_argument("--thresh", type=float, nargs='?', default=0.0, help="Minimum improvement for K-Clusters to be considered better.")
    args = parser.parse_args()
    main(args)
