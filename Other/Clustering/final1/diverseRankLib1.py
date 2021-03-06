from treeLib import * 
def mergingFactor(pattern1,pattern2):
	"""return the merging factor at level l"""
	"""pattern1 is pattern at level l pattern2 is pattern at level l+1"""

	count1=len(pattern1)*1.0
	count2=len(pattern2)*1.0
	mf=(count1-1.0)/(count2-1.0)
	return mf


def levelFactor(h,l):
	"""return the level factor at level l"""
	
	lf=(2.0*(h-l))/( (h-1)*h)
	return lf


def adjustmentFactor(eub,eb):
	"""eub is number of edges for this pattern in unbalanced concept tree from l+1 to l  """
	"""eb is number of edges for this pattern in balanced concept tree from l+1 to l  """
	"""return the adjustment factor at level l"""
	
	af=(eub*1.0)/(eb*1.0)
	return af


def diverseRank(pattern,codeDictB,codeDictUb,height):
        """pattern in a list of items whose diverserank is to be calculated"""
        """codeDictB,codeDictUb are the dict which stores the unique code for each item in Balanced and Unbalanced Concept Hierarchy Tree"""
        """height is the height of the Balanced  Concept Hierarchy Tree"""

	patternUb=[]
	patternB=[]
	for i in pattern:
		patternUb.append(codeDictUb[i])
		patternB.append(codeDictB[i])

	patternB=set(patternB)
	patternUb=set(patternUb)

	return dr

def diverseRank1(pattern,codeDictB,codeDictUb,height):
        """pattern in a list of items whose diverserank is to be calculated"""
        """codeDictB,codeDictUb are the dict which stores the unique code for each item in Balanced and Unbalanced Concept Hierarchy Tree"""
        """height is the height of the Balanced  Concept Hierarchy Tree"""

	patternUb=[]
	patternB=[]
	for i in pattern:
		patternUb.append(codeDictUb[i])
		patternB.append(codeDictB[i])

	patternB=set(patternB)
	patternUb=set(patternUb)

	dr=0
	level=height
	while len(patternB)>1:
		newPatternB=[]
		newPatternUb=[]
		level-=1
		maxPlus=0
		for i in patternUb:
			if maxPlus<i.count('+'):
				maxPlus=i.count('+')
		eub=0
        	for i in patternUb:
			index=i.rfind('+')
			if maxPlus==i.count('+'):
            			newPatternUb.append(i[:index])  
				eub+=1
			else:
            			newPatternUb.append(i)

        	for i in patternB:
			index=i.rfind('+')
            		newPatternB.append(i[:index])  
		eb=len(newPatternB)

		newPatternB=set(newPatternB)
		newPatternUb=set(newPatternUb)
		dr+=levelFactor(height,level)*mergingFactor(newPatternB,patternB)*adjustmentFactor(eub,eb)
		patternB=newPatternB
		patternUb=newPatternUb
	return dr

if __name__=="__main__":
    
   	print "hello"
        weHave=[]
    	tree = Tree()
    	root=tree.root
    	f=open("sample.cht",'r')
    	for line in f:
    	    if len(line)>2:
    	        line=line.strip('\n\r\t ').split('|')
    	        weHave.append(line[-1])
    	        root=tree.insert(root,line)
    	codeDictUb={}
    	codeDictB={}
	
	
	#tree.printTree(root)
	height=tree.treeHeight(root,0)
	tree.itemToCodeDict(root,codeDictUb)
	root=tree.unbalancedToBalanced(root,height)
	tree.itemToCodeDict(root,codeDictB)	
