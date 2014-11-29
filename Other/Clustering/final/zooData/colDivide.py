import sys
#labels = ["a", "p", "c", "l", "w", "ac", "g"]
labels = ["Q-E", "ZN-E", "PH-E", "DBO-E", "DQO-E", "SS-E", "SSV-E", "SED-E", "COND-E", "PH-P", "DBO-P", "SS-P", "SSV-P", "SED-P", "COND-P", "PH-D", "DBO-D", "DQO-D", "SS-D", "SSV-D", "SED-D", "COND-D", "PH-S", "DBO-S", "DQO-S", "SS-S", "SSV-S", "SED-S", "COND-S", "RD-DBO-P", "RD-SS-P", "RD-SED-P", "RD-DBO-S", "RD-DQO-S", "RD-DBO-G", "RD-DQO-G", "RD-SS-G", "RD-SED-G"]

def colStats(colData, colNum):
    #print colNum,
    #print labels[colNum-1]
    LBL = labels[colNum-1]
    #colData = [float(i) for i in colData if i is not '?' else i for i in colData]
    colData = [float(i) if i is not '?' else i for i in colData]
    colTup = [(colData[i], i) for i in range(len(colData))]
    colTup = sorted(colTup, key = lambda x: x[0])
    n = int(len(colTup)*1.0/3)
    twon = int(len(colTup)*2.0/3)
    colNew = []
    for i in range(len(colTup)):
        if i < n:
            #colNew.append((LBL+'low', colTup[i][1]))
            colNew.append((LBL+'-small', colTup[i][1]))
        elif i >= n and i < twon:
            #colNew.append((LBL+'mid', colTup[i][1]))
            colNew.append((LBL+'-medium', colTup[i][1]))
        else:
            #colNew.append((LBL+'hi', colTup[i][1]))
            colNew.append((LBL+'-large', colTup[i][1]))

    colNew = sorted(colNew, key = lambda x: x[1])
    return [i[0] for i in colNew]

def printMat(data):
    lenCol = len(data[0])
    for i in range(lenCol):
        l = [str(data[j][i]) for j in range(len(data))]
        print '\t'.join(l)


if __name__ == '__main__':
    f = open(sys.argv[1]).read().split('\n')
    numCols = len([i for i in f[0].split(',') if len(i) > 0])
    #numCols = len([i for i in f[0].split('\t')])
    data = [[] for i in range(numCols)]
    for i in f:
        k  = [j for j in i.split(',') if len(j) > 0]
        #k  = [j for j in i.split('\t')]
        #print i
        #print k
        for p, q in enumerate(k):
            data[p].append(q)

    convertCols = [1] * (numCols )# + [0]

    for i, j in enumerate(data):
        if convertCols[i] == 1:
            data[i] = colStats(j, i+1)

    #print data[-2]
    #print data

    printMat(data)
