from treeLib import *
from diverseRankLib import *


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
	print diverseRank(['low_buying','med_maint','5more_doors','4_persons','med_lug_boot','med_safety'],codeDictB,codeDictUb,height-1);
