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


def compareTrans(i,j):
    """Check if the two trans have the first len-1 items in common"""
    i=list(i)
    j=list(j)
    l=len(i)
    for k in range(l-1):
        if i[k]!=j[k]:
            return False
    if i[-1]==j[-1]:
        return False
    elif i[-1]<j[-1]:
        return True
    elif i[-1]>j[-1]:
        return False
    
def joinSet(itemSet, length):
        """Join a set with itself and returns the n-element itemsets"""
        return set([frozenset(list(i)+[list(j)[-1]]) for i in itemSet for j in itemSet if compareTrans(i,j)])


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
        #print "\n\n", currentLSet, "\n"
        currentCSet = returnItemsWithMinSupport(currentLSet,
                                                transactionList,
                                                minSupport,
                                                freqSet)
        #print "\n", currentCSet, "\n\n"

        currentLSet = currentCSet
        k = k + 1

    def getSupport(item):
            """local function which Returns the support of an item"""
            return float(freqSet[item])/len(transactionList)
    toRetItems = []
    for key, value in largeSet.items():
        toRetItems.extend([tuple(item)
                           for item in value])

    return toRetItems


def printResults(items, rules):
    """prints the generated itemsets and the confidence rules"""
    for item, support in items:
        print "item: %s , %.3f" % (str(item), support)
    print "\n------------------------ RULES:"
    for rule, confidence in rules:
        pre, post = rule
        print "Rule: %s ==> %s , %.3f" % (str(pre), str(post), confidence)


def dataFromFile(fname):
        """Function which reads from the file and yields a generator"""
        file_iter = open(fname, 'rU')
        for line in file_iter:
#                print line
                line = line.strip().rstrip(' ')                         # Remove trailing comma
                record = frozenset(line.split(' '))
                yield record


if __name__ == "__main__":

    optparser = OptionParser()
    optparser.add_option('-f', '--inputFile',
                         dest='input',
                         help='filename containing csv',
                         default=None)
    optparser.add_option('-s', '--minSupport',
                         dest='minS',
                         help='minimum support value',
                         default=0.15,
                         type='float')
    optparser.add_option('-c', '--minConfidence',
                         dest='minC',
                         help='minimum confidence value',
                         default=0.6,
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

    #print options.input

    minSupport = options.minS
    minConfidence = options.minC

    items = runApriori(inFile, minSupport)

    print items
