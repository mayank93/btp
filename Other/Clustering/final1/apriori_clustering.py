import sys, os
from itertools import chain, combinations
from collections import defaultdict
from optparse import OptionParser

def getQuality(a, b, beta = 0.25):
    support = a
    length = b
    # a is sup of i
    # b is dimmesion of i
    return support*((1.0/beta)**length)

def item_sortKey(item):
    return getQuality(item[1], len(item[0]))

def keyFunc(betaVal):
    def qualityFunc(item, beta = betaVal):
        support = item[1]
        length = len(item[0])
        return support*((1.0/beta)**length)
    return qualityFunc

def printResults(items, rules):
    """prints the generated itemsets and the confidence rules"""
    print items
    for item, support in sorted(items, key = item_sortKey):
        print "item: %s , %.3f  - %.3f" % (str(item), support, item_sortKey((item, support)))

    print max(items, key = item_sortKey)
    return

def getMax(items):
    return max(items, key = item_sortKey)

def getMatching(transaction, dataIter):
    res = []
    transaction = frozenset(transaction)
    #print transaction
    for i in dataIter:
        #print frozenset(i[1:])
        if transaction.issubset(frozenset(i[1:])):
            res.append(i[0])
    return res

def runApriori_bin(data_frozenset, minSup):
    #print data_frozenset[:10]
    binInput = 'binInput'
    binOutput = 'binOutput'
    #print "Opening file... "
    f = open(binInput, 'w+')
    #print "Opened file... "
    for i in data_frozenset:
        f.write(' '.join(i) + '\n')
    f.close()

    os.system("./apriori_bin -s%0.1f %s %s > /dev/null 2>&1" % (minSup * 100, binInput, binOutput))

    f = open(binOutput, 'r')

    items = []

    for i in f:
        i = i.strip(' \t\n').split(' ')
        support = float(i[-1].strip('()'))/100
        i = i[:-1]
        i = [j for j in i if len(j) > 0]
        items.append((tuple(i), support))

    #print items
    #items = runApriori(data_frozenset, minSup)
    #print items
    #print items
    #print type(items[0][0])
    return items

def getFreqTransactions(data, minSup, betaVal):
    data_frozenset = []
    for i in data:
        data_frozenset.append(frozenset(i[1:]))
    #print "Starting Apriori"
    items = runApriori_bin(data_frozenset, minSup)
    #print "Apriori finished"
    #print items
    #items=[i for i in items if i[1]*len(data)>=minEle]
    if len(items)==0:
        return ([],set())
    #print items
    #for i in sorted(items, key = keyFunc(betaVal)):
    #        print i,keyFunc(betaVal)(i)
    #maxItem = getMax(items, key = keyFunc(betaVal))
    maxItem = max(items, key = keyFunc(betaVal))
    #print maxItem
    #for i in items:
        #print i, keyFunc(betaVal)(i)
    #print '-----------------'
    matchingTrans = getMatching(maxItem[0], data)
    pattern = maxItem[0]
    #print maxItem, keyFunc(betaVal)(maxItem)
    return (matchingTrans, pattern)


if __name__ == "__main__":

    print "Its a lib"
