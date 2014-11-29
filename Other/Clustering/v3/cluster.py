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

def getCluster(S,minSup,beta,w,minEle):
    pBest=[]
    iBest=set()
    tidBest=[]
    pBestTid=0

#    for j in range(1,(2*len(S))/minSup):
    for j in S :
        #from S choose p uniformly at random
        #pTid,p=getUniformlyRandom(S)
        #or check each and every element
        pTid,p=j[0],j
        #print "random", pTid,p
        #tb is the table of itemset constructed with tid of record as first element
        tb=getTable(S,p,w)
        #print tb
        tid,i=getBest(tb,minSup,beta,minEle)

        #if tid and i empty exit while loop

        #print "best ", i,tid
        #cmp if new i better than iBest
        if getQuality(len(tid),len(i),beta)>getQuality(len(tidBest),len(iBest),beta):
#            print "helo"
            tidBest=tid
            iBest=i
            pBest=p
            pBestTid=pTid
#        print "bb",pBest,iBest,tid
    return (pBest,iBest,tidBest,pBestTid)


def getTable(S,p,w):
    table=[]
    for s in S:
        i=[s[0]]
        for j in range(1,len(s)):
                if abs(p[j]-s[j])<=w:
                    i.append(j)
        table.append(i)
    return table

def getBest(tb,minSup,beta,minEle):
    #remove first element from each list in tb and get the frequent itemset
    #get the quality for best
    return getFreqTransactions(tb, minSup,beta,minEle)

def getQuality(a,b,beta):
    # a is sup of i
    # b is dimmesion of i
    return a*((1.0/beta)**b)

def getNewS(S,tid):
    newS=[]
    for i in S:
        if i[0] not in tid:
            newS.append(i)       
    return newS

def refineCluster(tid,codeDictB,codeDictUb,height):
    newTid=[]
    totalDiversity=0.0
    for j in tid:
        pattern=getP(S,j)[1:]
        pattern=[str(i) for i in pattern]
        print "pattern:",pattern, diverseRank(pattern, codeDictB, codeDictUb, height)
        newTid.append(j)
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
    f=open('concept.csv','r')
    for line in f:
        if len(line)>2:
            line=line.strip('\n\r\t ').split('|')
            root=tree.insert(root,line)
    codeDictUb={}
    codeDictB={}

    tree.printTree(root)
    height=tree.treeHeight(root,0)
    tree.itemToCodeDict(root,codeDictUb)
    root=tree.unbalancedToBalanced(root,height)
    tree.itemToCodeDict(root,codeDictB)
  
    f=open("1.txt")
    for i in f:
        i=i.strip('\n\r\t ').split()
        i=[int(j) for j in i]
        S.append(i)
    minEle=3
    minSup=.25
    tid=[]
    result=[]

    print S

    tb=getTable(S,[2, 1, 1, 1, 1, 0, 1, 9, 8, 9, 0, 4, 4],w)
    print getQuality(4,5,beta)
    print getNewS(S,[1,4,6,9])
    print getP(S,3)
    print getUniformlyRandom(S) 
    print getBest(tb,minSup,beta,minEle)

    #print "Clusters"
    #print S
    while len(S)>=minEle:
#    while len(S):
        pBest,iBest,tid,pBestTid=getCluster(S,minSup,beta,w,minEle)
        tid=refineCluster(tid,codeDictB,codeDictUb,height)
        print "----------------------"
        print "result",pBest,iBest,tid,pBestTid
        print "----------------------"
        S= getNewS(S,tid)
        #print S
        result.append((pBest,iBest,tid,pBestTid))
