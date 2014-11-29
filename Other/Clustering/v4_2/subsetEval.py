import itertools
import sys

if len(sys.argv) < 4:
    print "%s: <labelledClassFile> <result> <classColofDatafile>" % sys.argv[0]
    sys.exit(0)

def getNatClusters():
    #f = open('disc_iris.csv', 'r')
    COL = int(sys.argv[3])
    f = open(sys.argv[1], 'r')
    natClusters = []
    for i, r in enumerate(f):
        #classLabel = r.strip(' \t\n').split(',')[-1]
        classLabel = r.strip(' \t\n').split(',')[COL]
        natClusters.append((i+1, classLabel))
    print natClusters
    return natClusters

def getMyClusters():
    #f = open('result1.txt', 'r')
    f = open(sys.argv[2], 'r')
    res = []
    for i in f:
        res.append([int(j) for j in i.strip(' \t\n').split(' ')])
    print res
    return res

def clustersMatch(natClusters, p, q):
    a = [i for i in natClusters if i[0] == p]
    b = [i for i in natClusters if i[0] == q]

    if a[0][1] == b[0][1]:
        return True
    else:
        return False

def evalDataset():
    natClusters = getNatClusters()
    myClusters = getMyClusters()
    print clustersMatch(natClusters, 144, 145)

    matchPair = 0
    totalPair = 0

    for cluster in myClusters:
        print cluster
        for p, q in itertools.combinations(cluster, 2):
            if clustersMatch(natClusters, p, q):
                matchPair += 1
            totalPair += 1
    print matchPair
    print totalPair
    print matchPair * 1.0 / totalPair

def printDict():
    natClusters = getNatClusters()
    d = {}
    for i in natClusters:
        if i[1] in d:
            d[i[1]].append(i[0])
        else:
            d[i[1]] = [i[0]]
    for i in d.keys():
        print ' '.join([str(ii) for ii in d[i]])

if __name__ == '__main__':
    evalDataset()
