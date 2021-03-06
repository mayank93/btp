import sys
import random
from apriori_clustering import *
from treeLib import *
from diverseRankLib import *

def getP(S,tid):
    for i in S:
        if i[0]==tid:
            return i
    return []

def getUniformlyRandom(S):
    pTid=0
    p=[]
    S=sorted(S,key=lambda x:x[0])
    x=S[0][0]
    y=S[-1][0]
    while 1:
        pTid=random.randint(x, y)
        p=getP(S,pTid)
        if p:
            return (pTid,p)

    return (pTid,p)

def getCluster(tb,minSup,beta,minEle):
    tid,i=getBest(tb,minSup,beta,minEle)
    #print "best",tid,i
    return (i,tid)


def getTable(S):
    table=[]
    for s in S:
        i=[s[0]]
        for j in range(1,len(s)):
                if s[j]==1:
                    i.append(j)
        table.append(i)
   
    return table

def getBest(tb,minSup,beta,minEle):
    #remove first element from each list in tb and get the frequent itemset
    #get the quality for best
    #print "test"
    return getFreqTransactions(tb, minSup,beta,minEle)

def getQuality(a,b,beta):
    # a is sup of i
    # b is dimmesion of i
    return a*((1.0/beta)**b)

def getNewTb(tb,tid):
    newTb=[]
    for i in tb:
        if i[0] not in tid:
            newTb.append(i)       
    return newTb


def removeOutlier1(vector,minVal):
    newVector=[]
    for i in vector:
        if i[1]<=minVal:
            newVector.append(i[0])
    return newVector


def distance(vector, element):
    sumArray = [(i-element)**2 for i in vector]
    res = reduce(lambda x, y: x + y, sumArray)**(0.5)
    return res 

def findOutlier2(vector,minVal):
    vector=sorted(vector,key=lambda x:x[1])
    #print vector
    minNum = vector[0]
    maxNum = vector[-1]
    if len(vector)>2:
        if (maxNum[1]-vector[-2][1])>minVal:
            #print 1
            return maxNum[0]
        elif (-minNum[1]+vector[1][1])>minVal:
            return minNum[0]
    return -1

def findOutlier3(vector,minVal):
    vector=sorted(vector,key=lambda x:x[1])
    newVector=[]
    #print vector
    temp=[vector[0]]
    for i in range(1,len(vector)):
        if abs(vector[i][1]-vector[i-1][1]) > minVal:
            newVector.append(temp)
            temp=[]
        temp.append(vector[i])
    if temp:
        newVector.append(temp)
    print newVector
    return newVector

def findOutlier1(vector,minVal):
    #for i in vector:
        #print i, distance(a, i)

    maxNum = max(vector)
    minNum = min(vector)

    distMax = distance(vector, maxNum)
    distMin = distance(vector, minNum)
    #print distMax,distMin
    if(distMax > distMin and distMax>=minVal):
        return (vector.index(maxNum), maxNum)
    elif (distMax < distMin and distMin>=minVal):
        return (vector.index(minNum), minNum)
    else:
        return -1,-1


def findOutlierTuple(tupleArray,minVal):
    nums = [int(i[1]*10000) for i in tupleArray]
    #print nums
    outIndex, outVal = findOutlier1(nums,minVal)
    #print "outlier",outIndex, outVal
    if outIndex==-1:
        return -1
    else:
        return tupleArray[outIndex][0]


def removeOutlier2(vector,minVal):
    outlier=0
    while outlier!=-1:
        outlier=findOutlierTuple(vector,minVal)
        vector=[i for i in vector if i[0]!=outlier]
    
    vector=[i[0] for i in vector]
    return vector

def removeOutlier3(vector,minVal):
    outlier=0
    while outlier!=-1:
        outlier=findOutlier2(vector,minVal)
        vector=[i for i in vector if i[0]!=outlier]
    
    vector=[i[0] for i in vector]
    return vector

def removeOutlier4(vector,minVal):
    outlier=findOutlier3(vector,minVal)
    maxVal=0;
    insider=[]
    for i in outlier:
        if len(i)>maxVal:
            maxVal=len(i)
            insider=i
    
    vector=[i[0] for i in insider]
    return vector


def refineCluster(tid,codeDictB,codeDictUb,height,tb):
    newTid=[]
    totalDiversity=0.0
    diversityVector=[]
    newTid=tid 
    #return newTid

    for j in tid:
        pattern=getP(tb,j)[1:]
        pattern=[str(i) for i in pattern]
        diversity=diverseRank(pattern, codeDictB, codeDictUb, height)
        diversityVector.append((j,diversity))
        #print "pattern:",pattern, diversity
    #newTid=removeOutlier1(diversityVector,0.2)
    #newTid=removeOutlier2(diversityVector,500)
    #print diversityVector
    #newTid=removeOutlier3(diversityVector,0.1)
    newTid=removeOutlier4(diversityVector,0.05)

    #print "new:",newTid
    #print totalDiversity
    return newTid

if __name__=="__main__":
    alpha=0.1
    S=[]
    beta=0.1
    w=0
   # create the tree
    tree = Tree()
    root=tree.root
    f=open(sys.argv[1],'r')
    for line in f:
        if len(line)>2:
            line=line.strip('\n\r\t ').split('|')
            root=tree.insert(root,line)
    codeDictUb={}
    codeDictB={}

    #tree.printTree(root)
    height=tree.treeHeight(root,0)
    tree.itemToCodeDict(root,codeDictUb)
    root=tree.unbalancedToBalanced(root,height)
    tree.itemToCodeDict(root,codeDictB)
  
    f=open(sys.argv[2])
    count=0
    for i in f:
        count+=1
        i=i.strip('\n\r\t ').split(',')
        i=[count]+i
        #print i
        S.append(i)

    #tb=getTable(S)
    tb=S
    #print"--------"
    #print "set",S
    #print"--------"
    #print "table",tb
    #print"--------"

    minEle=4
    minSup=.3
    tid=[]
    result=[]

    """
    #print getQuality(4,5,beta)
    #print getNewS(S,[1,4,6,9])
    #print getP(S,3)
    #print getUniformlyRandom(S) 
    #print getBest(tb,minSup,beta,minEle)
    """
    #print "Clusters"
    #print S
    count=0
    while len(tb)>=minEle:
        count+=1
#    while len(S):
        iBest,tid=getCluster(tb,minSup,beta,minEle)
        if not iBest:
            break
        #print iBest,tid
        tid=refineCluster(tid,codeDictB,codeDictUb,height,tb)
        #print "----------------------"
        #print "result",iBest,tid
        #print "----------------------"
        for i in tid:
            print i,
        print
        tb= getNewTb(tb,tid)
        #print S
        result.append((iBest,tid))
    #print "cluster:",count
