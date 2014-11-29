class Node:
	"""simple class for nodes of tree """
	def __init__(self,name,code,level):
        	# initializes the data members
        	self.name=str(name)
        	self.childCount=0
        	self.code=code
        	self.level=level
        	self.children=[]


class Tree:
	""" class for tree """
	def __init__(self):
		# initializes the root member
		self.root = self.addNode('root','0',1)
                self.count=0

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
			if root.children[i].name==str(data[0]):
				root.children[i]=self.insert(root.children[i],data[1:])
				flag=1
				break
		if flag==0:
                        self.count+=1
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

	def unbalancedToBalanced(self,root,height):
		if root.childCount==0:
			if height>root.level:
				root.children.append(self.addNode(root.name,root.code+'+'+str(root.childCount),root.level+1))
				root.childCount+=1

		for i in range(root.childCount):
			root.children[i]=self.unbalancedToBalanced(root.children[i],height)

		return root
	
if __name__ == "__main__":
	# create the tree
	tree = Tree()
	root=tree.root
	root=tree.insert(root,[1,2,3,5])
	root=tree.insert(root,[1,3,3,3])
	root=tree.insert(root,[1,3,3,4])
	root=tree.insert(root,[1,2,4,1])
	root=tree.insert(root,[1,2,5,2])
	root=tree.insert(root,[1,2,5,5,7])
	root=tree.insert(root,[1,2,5,5,8])
	root=tree.insert(root,[1,2,5,5,6])
	root=tree.insert(root,[1,2,5,5,9])
	root=tree.insert(root,[1,2,5,5,0])
	tree.printTree(root)
	codeDict=[]
	height=0
	tree.itemToCodeDict(root,codeDict)
	print codeDict
	height=tree.treeHeight(root,height)
	print height
	codeDict1=[]
	root=tree.unbalancedToBalanced(root,height)
	tree.printTree(root)
	tree.itemToCodeDict(root,codeDict1)
	print codeDict1
