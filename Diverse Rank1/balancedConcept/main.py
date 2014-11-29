from treeLib import *
from diverseRankLib import *

if __name__ == "__main__":
    # create the tree
    tree = Tree()
    root=tree.root
    f=open('test.csv','r')
    for line in f:
        if len(line)>2:
            line=line.strip('\n\r\t ').split('|')
            root=tree.insert(root,line)
    tree.printTree(root)
#    print diverseRank(['0+9+16+2','0+9+3+1','0+9+1+1','0+9+14+3'],4)
    print diverseRank(['0+0+0+0','0+0+1+0','0+0+2+0'],3)
