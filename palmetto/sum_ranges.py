# quick data processing script to sum ranges of time that we produce from the MultiImgSeg script
# reads in all ranges of start and stop times from the specified file then 
# merges overlapped ranges and prints and final exclusive sum of those ranges

import sys

def sum_ranges(ranges):
    unique_ranges = []
    ranges = sorted(ranges, key=lambda x:x[0])
    for r in ranges:
        new_range = True
        for idx,u in enumerate(unique_ranges):
            if r[0] <= u[1]:
                if r[1] > u[1]:
                    unique_ranges[idx][1] = r[1]
                new_range = False
                break
        if new_range:
            unique_ranges.append(r)
    total = 0
    for r in unique_ranges:
        total += r[1]-r[0]
    print(total)

ranges = []
with open(sys.argv[1]) as fh:
    for line in fh:
        toks = line.split(",")
        if len(toks) > 3:
            ranges.append([float(toks[1]), float(toks[2])])
sum_ranges(ranges)
