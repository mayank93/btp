import sys

f=open(sys.argv[1])

for i in f:
    #print i
    i=i.strip('\n\r\t ').split(',')
    k=[]
    for j in range(0,len(i)-1):
        k.append(i[j]+'_'+str(j))
    k=','.join(k)
    print k
