from treeLib import * 

def diverseRank(pattern,codeDictB,codeDictUb,height):
        
        """pattern in a list of items whose diverserank is to be calculated"""
        """codeDictB,codeDictUb are the dict which stores the unique code for each item in Balanced and Unbalanced Concept Hierarchy Tree"""
        """height is the height of the Balanced  Concept Hierarchy Tree"""

	patternUb=[]
	patternB=[]
	for i in pattern:
                patternUb.append(codeDictUb[i][2:])
                patternB.append(codeDictB[i][2:])
        height=height-2  # adjustment for extra root
	patternB=set(patternB)
	patternUb=set(patternUb)
        
    	tree = Tree()
    	root=tree.root
        tree.count=0
        for i in patternB:
            i=i.split('+')
    	    root=tree.insert(root,i)
        
        E=height*len(patternB)
        e=height+len(patternB)-1
        eDash=tree.count-1 # adjustment for extra root
	#tree.printTree(root)

        #print E,e,eDash
        dr=(eDash-e)*1.0/(E-e)*1.0
        #print dr
        #print patternB
        #print patternUb
        #print height
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
        
        diverseRank(['a','d','f'],codeDictB,codeDictUb,height)

