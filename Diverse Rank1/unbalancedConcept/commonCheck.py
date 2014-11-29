import sys

a = sys.argv[1]
b = sys.argv[2]

def commonLines(a, b):
    f = open(a).read().split('\n')
    f = [i.strip('\n') for i in f]
    f = [i for i in f if len(i) > 2]

    g = open(b).read().split('\n')
    g = [i.strip('\n') for i in g]
    g = [i for i in g if len(i) > 2]

    return len([i for i in f if i in g])

print commonLines(a, b)
