from treeLib import *
from diverseRankLib import *
from apriori_v2 import *
from functions import *
from classifier import *

def sortFunc(key):
    if '_buying' in key:
        return 0
    elif '_maint' in key:
        return 1
    elif '_doors' in key:
        return 2
    elif '_persons' in key:
        return 3
    elif '_lug_boot' in key:
        return 4
    elif '_safety' in key:
        return 5
    elif '_class' in key:
        return 6
    else: return 7

def writeToFile(data, file):
    for dataItem in data:
        l = list(dataItem[0])
        l = sorted(l, key = sortFunc)
        print ','.join(l) + ',' + dataItem[1]
    return

if __name__ == "__main__":

	# create the tree
	tree = Tree()
	root=tree.root
	f=open('tagged_cht.txt','r')
	for line in f:
        	if len(line)>2:
			line=line.strip('\n\r\t ').split('|')
			root=tree.insert(root,line)
	codeDictUb={}
	codeDictB={}

#	tree.printTree(root)
	height=tree.treeHeight(root,0)

	tree.itemToCodeDict(root,codeDictUb)

	root=tree.unbalancedToBalanced(root,height)

	tree.itemToCodeDict(root,codeDictB)
#	tree.printTree(root)
        freqPat=runApriori(dataFromFile('trainData_stripped'),0.1)

#        print "Frequent Patterns: ", freqPat
#        print "Size of data: ", len(freqPat)
        classLabels=['acc_class','good_class','unacc_class','vgood_class']
        rules=ruleMiner(freqPat, classLabels)
#        print "Rules: ",rules

        train=open('trainData','r')
        trainData=[]
        for i in train:
            i=i.strip('\n\r\t ').split(',')
            trainData.append(set(i))

        rankedRules=ranker(rules, highorder(diverseRank,codeDictB,codeDictUb,height-1), highorder(confidence,trainData,classLabels), diverseWt = 0, confidenceWt = 1.0, cutoff = 0.2)
#        print "Ranked Rules: ",rankedRules
        rankedRules=convertRulesToFS(rankedRules)
        print "Frozec Rules: ",rankedRules
        
        test=open('testData_stripped','r')
        testData=[]
        for i in test:
            i=i.strip('\n\r\t ').split(',')
            testData.append(set(i))

        dataSet = [frozenset(i) for i in testData]
        #print "DataSet : ",dataSet

        classifiedData = classify(rankedRules, dataSet, 'unacc_class')
        #print "Classified data : ",classifiedData
        writeToFile(classifiedData, "testData_labelled")
