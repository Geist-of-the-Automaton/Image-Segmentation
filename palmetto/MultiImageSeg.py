import cv2
import numpy as np
import matplotlib.pyplot as plt
import argparse
import time
from multiprocessing import Pool

def kmeans_cluster(pixel_array, K, criteria, kloops):
    compactness,labels,center = cv2.kmeans(pixel_array, K, None, criteria, kloops, cv2.KMEANS_RANDOM_CENTERS)
    return (compactness, labels, center, K)

def kmeans_segment(args):
    maxK = args.maxk
    kloops = args.kloops
    thresh = args.thresh
    image = cv2.imread(f"./input/{args.infile}")
    image_array = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    pixel_array = image_array.reshape((-1,3))
    pixel_array = np.float32(pixel_array)

    criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 10, 1.0)

    pool = Pool()
    job_list = []
    for K in range(1,maxK+1):
        # return values are sum of squared distances (min obj), label array, and array of centroids
        job_list.append((pixel_array, K, criteria, kloops))
    
    start = time.time()
    data_output = pool.starmap(kmeans_cluster, job_list)
    pool.close()
    pool.join()
    best_cluster = sorted(data_output)[0]
    end = time.time()
    elapsed = end - start

    best_compactness = best_cluster[0]
    best_labels = best_cluster[1]
    best_center = best_cluster[2]
    bestK = best_cluster[3]

    imgB = pixel_array.size * 4 * maxK * kloops
    imgGB = imgB / (1e9)

    best_center = np.uint8(best_center)
    segimg_array = best_center[best_labels.flatten()]
    segimg_array = segimg_array.reshape((image_array.shape))
    return (segimg_array, elapsed, imgGB/elapsed, bestK)

def main(args):
    output = kmeans_segment(args)
    with open(f"{args.outdir}/diagnostics.txt", "a+") as f:
        plt.imsave(f"{args.outdir}/new_{args.infile}", output[0])
        f.write(f"{args.infile},{output[1]},{output[2]},{output[3]}\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--infile", required=True, help="Input file for processing.")
    parser.add_argument("--outdir", required=True, help="Output directory to store segmented image.")
    parser.add_argument("--maxk", type=int, nargs='?', default=10, help="Max number of clusters to attempt in segmentation.")
    parser.add_argument("--kloops", type=int, nargs='?', default=10, help="Number of internal iterations OpenCV kmeans will perform")
    parser.add_argument("--thresh", type=float, nargs='?', default=0.0, help="Minimum improvement for K-Clusters to be considered better.")
    args = parser.parse_args()
    main(args)
