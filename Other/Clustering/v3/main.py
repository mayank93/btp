from treeLib import *
from diverseRankLib import *

if __name__ == "__main__":

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

#	tree.printTree(root)

	print diverseRank(['1','4','7','10'],codeDictB,codeDictUb,height)
