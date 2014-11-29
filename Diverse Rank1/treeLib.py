class Node:
    """simple class from nodes of tree """
    def __init__(self,name,code,level):
        # initializes the data members
        self.name=name
        self.childCount=0
        self.code=code
        self.level=level
        self.children=[]



class Tree:
    """ class from tree """
    def __init__(self):
        # initializes the root member
        self.root = self.addNode('root','0',1)

    def addNode(self,name, code,level):
        # creates a new node and returns it
        return Node(name,code,level)

    def insert(self, root, data):
        """data is a list containing elelment in hirearcy """
        # inserts a new data
        if data==[]:
            return root

        flag=0
        
        for i in range(root.childCount):
            if root.children[i].name==data[0]:
                root.children[i]=self.insert(root.children[i],data[1:])
                flag=1
                break

        if flag==0:
            root.children.append(self.addNode(data[0],root.code+'+'+str(root.childCount),root.level+1))
            root.childCount+=1
            root.children[-1]=self.insert(root.children[-1],data[1:])

        return root

    def printTree(self, root):
        # prints the tree path
        print root.name,root.code,root.level
        for i in range(root.childCount):
            self.printTree(root.children[i])

    def itemToCodeDict(self, root,codeDict):
        if root.childCount==0:
            codeDict[root.name]=root.code

        for i in range(root.childCount):
            self.itemToCodeDict(root.children[i],codeDict)

    def treeHeight(self,root,height):
        if root.childCount==0:
            if height<root.level:
                height=root.level
            return height

        for i in range(root.childCount):
            height=self.treeHeight(root.children[i],height)

        return height

    def unbalancedToBalanced(self):
        pass

if __name__ == "__main__":
    # create the tree
    tree = Tree()
    root=tree.root
    root=tree.insert(root,[1,2,3,4])
    root=tree.insert(root,[1,3,3,3])
    root=tree.insert(root,[1,3,3,4])
    root=tree.insert(root,[1,2,4,4])
    root=tree.insert(root,[1,2,5,4])
    tree.printTree(root)
    codeDict={}
    height=0
    tree.itemToCodeDict(root,codeDict)
    height=tree.treeHeight(root,height)
    print codeDict
    print height
