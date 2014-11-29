import sys

f=open(sys.argv[1])
dic={}
count=0
for i in f:
    i=i.strip('\n\r\t ').split(',')
    count+=1
    label=i[-1]
    if label not in dic:
        dic[label]=[]
    dic[label].append(str(count))

for i in dic:
    x=' '.join(dic[i])
    print x
