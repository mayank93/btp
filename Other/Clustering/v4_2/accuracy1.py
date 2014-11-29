import sys
actual=open(sys.argv[1])
predicted=open(sys.argv[2])

actualData=[]
predictedData=[]

length=0
for i in actual:
    i=i.strip('\n\r\t ').split()
    i=[int(j) for j in i]
    length+=len(i)
    actualData.append(i)


for i in predicted:
    i=i.strip('\n\r\t ').split()
    i=[int(j) for j in i]
    predictedData.append(i)

#[acc,pre]
#tp=1,1
#tn=0,0
#fp=0,1
#fn=1,0

result=[[0,0],[0,0]]
total=0
for i in range(1,length+1):
    for j in range(1,length+1):
        acc=0
        pre=0
        total+=1
        for k in actualData:
            #print k
            if i in k and j in k:
                #print "actual", i,j,k
                acc=1
                break

        for k in predictedData:
            if i in k and j in k:
                #print "predicted",i,j,k
                pre=1
                break
        
        result[acc][pre]+=1

print result
print "accuracy",((result[1][1]+result[0][0])*1.0)/total
print "precison",(result[1][1]*1.0)/(result[1][1]+result[0][1])
print "recall",(result[1][1]*1.0)/(result[1][1]+result[1][0])
