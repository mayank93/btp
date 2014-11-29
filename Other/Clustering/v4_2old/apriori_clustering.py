"""
Description     : Simple Python implementation of the Apriori Algorithm

Usage:
    $python apriori.py -f DATASET.csv -s minSupport  -c minConfidence

    $python apriori.py -f DATASET.csv -s 0.15 -c 0.6
"""

import sys

from itertools import chain, combinations
from collections import defaultdict
from optparse import OptionParser


def subsets(arr):
    """ Returns non empty subsets of arr"""
    return chain(*[combinations(arr, i + 1) for i, a in enumerate(arr)])


def returnItemsWithMinSupport(itemSet, transactionList, minSupport, freqSet):
        """calculates the support for items in the itemSet and returns a subset
       of the itemSet each of whose elements satisfies the minimum support"""
        _itemSet = set()
        localSet = defaultdict(int)

        for item in itemSet:
                for transaction in transactionList:
                        if item.issubset(transaction):
                                freqSet[item] += 1
                                localSet[item] += 1

        for item, count in localSet.items():
                support = float(count)/len(transactionList)

                if support >= minSupport:
                        _itemSet.add(item)

        return _itemSet


def joinSet(itemSet, length):
        """Join a set with itself and returns the n-element itemsets"""
        return set([i.union(j) for i in itemSet for j in itemSet if len(i.union(j)) == length])


def getItemSetTransactionList(data_iterator):
    transactionList = list()
    itemSet = set()
    for record in data_iterator:
        transaction = frozenset(record)
        transactionList.append(transaction)
        for item in transaction:
            itemSet.add(frozenset([item]))              # Generate 1-itemSets
    return itemSet, transactionList


def runApriori(data_iter, minSupport):
    """
    run the apriori algorithm. data_iter is a record iterator
    Return both:
     - items (tuple, support)
     - rules ((pretuple, posttuple), confidence)
    """
    itemSet, transactionList = getItemSetTransactionList(data_iter)

    freqSet = defaultdict(int)
    largeSet = dict()
    # Global dictionary which stores (key=n-itemSets,value=support)
    # which satisfy minSupport

    assocRules = dict()
    # Dictionary which stores Association Rules

    oneCSet = returnItemsWithMinSupport(itemSet,
                                        transactionList,
                                        minSupport,
                                        freqSet)

    currentLSet = oneCSet
    k = 2
    while(currentLSet != set([])):
        largeSet[k-1] = currentLSet
        currentLSet = joinSet(currentLSet, k)
        currentCSet = returnItemsWithMinSupport(currentLSet,
                                                transactionList,
                                                minSupport,
                                                freqSet)
        currentLSet = currentCSet
        k = k + 1

    def getSupport(item):
            """local function which Returns the support of an item"""
            return float(freqSet[item])/len(transactionList)

    toRetItems = []
    for key, value in largeSet.items():
        toRetItems.extend([(tuple(item), getSupport(item))
                           for item in value])
    return toRetItems

def getQuality(a, b, beta = 0.1):
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
    for i in dataIter:
        if transaction.issubset(frozenset(i[1:])):
            res.append(i[0])
    return res


def dataFromFile(fname):
        """Function which reads from the file and yields a generator"""
        file_iter = open(fname, 'rU')
        for line in file_iter:
                line = line.strip().rstrip(' ')                         # Remove trailing comma
                record = frozenset(line.split(' ')[1:])
                yield record

def dataFromFileAsList(fname):
        """Function which reads from the file and yields a generator"""
        file_iter = open(fname, 'rU')
        res = []
        for line in file_iter:
                line = line.strip().rstrip(' ')                         # Remove trailing comma
                line = line.split(' ')
                res.append(line)
        return res

def dataFromFileWithIndex(fname):
        """Function which reads from the file and yields a generator"""
        file_iter = open(fname, 'rU')
        for line in file_iter:
                line = line.strip().rstrip(' ')                         # Remove trailing comma
                line = line.split(' ')
                record = (line[0], frozenset(line[1:]))
                yield record


def getFreqTransactions(data, minSup, betaVal,minEle):
    data_frozenset = []
    for i in data:
        data_frozenset.append(frozenset(i[1:]))
    items = runApriori(data_frozenset, minSup)
    #print items
    items=[i for i in items if i[1]*len(data)>=minEle]
    if len(items)==0:
        return ([],set())
    #print items
    #for i in sorted(items, key = keyFunc(betaVal)):
    #        print i,keyFunc(betaVal)(i)
    #maxItem = getMax(items, key = keyFunc(betaVal))
    maxItem = max(items, key = keyFunc(betaVal))
    #for i in items:
        #print i, keyFunc(betaVal)(i)
    #print '-----------------'
    matchingTrans = getMatching(maxItem[0], data)
    pattern = maxItem[0]
    #print maxItem, keyFunc(betaVal)(maxItem)
    return (matchingTrans, pattern)


if __name__ == "__main__":

    optparser = OptionParser()
    optparser.add_option('-f', '--inputFile',
                         dest='input',
                         help='filename containing csv',
                         default=None)
    optparser.add_option('-s', '--minSupport',
                         dest='minS',
                         help='minimum support value',
                         #default=0.15,
                         default=0.05,
                         type='float')
    optparser.add_option('-c', '--minConfidence',
                         dest='minC',
                         help='minimum confidence value',
                         #default=0.6,
                         default=0.8,
                         type='float')

    (options, args) = optparser.parse_args()

    inFile = None
    if options.input is None:
            inFile = sys.stdin
    elif options.input is not None:
            inFile = dataFromFile(options.input)
    else:
            print 'No dataset filename specified, system with exit\n'
            sys.exit('System will exit')

    minSupport = options.minS
    minConfidence = options.minC

    data = dataFromFileAsList(options.input)
    betaVal = 0.10
    print getFreqTransactions(data, minSupport, betaVal)
